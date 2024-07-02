/* Song.js
 * Model for a MIDI file.
 */
 
export class Song {
  constructor(src) {
    this.nextId = 1;
    if (!src?.length) {
      this._initDefault();
    } else if (src instanceof Uint8Array) {
      this._initDecode(src);
    } else {
      throw new Error(`Unexpected input for new Song`);
    }
  }
  
  _initDefault() {
    this.tracks = []; // {events:{id,when:ticks,chid,opcode,a,b,v,duration:ticks,offVelocity}[]}
    this.division = 24; // ticks/qnote: 1..32767
  }
  
  /* Sets division (ticks/qnote) to this new value and updates all timestamps with a best fit.
   * Returns false if unchanged -- invalid input, or already at this value.
   * Beware that this is very destructive.
   * Increasing division by a multiple of the prior division is completely safe,
   * but any other case will likely produce some rounding error.
   */
  changeDivision(division) {
    if (!division || (typeof(division) !== "number") || (division < 1) || (division > 0x7fff)) return false;
    if (division === this.division) return false;
    const scale = division / this.division;
    for (const track of this.tracks) {
      for (const event of track.events) {
        event.when = Math.round(event.when * scale);
        if (event.duration) event.duration = Math.round(event.when * scale);
      }
    }
    this.division = division;
    return true;
  }
  
  addTrack() {
    const track = { events: [] };
    this.tracks.push(track);
    return track;
  }
  
  deleteTrack(ix) {
    if ((ix < 0) || (ix >= this.tracks.length)) return false;
    this.tracks.splice(ix, 1);
    return true;
  }
  
  getEventById(id) {
    let trid = 0;
    for (const track of this.tracks) {
      const event = track.events.find(e => e.id === id);
      if (event) return { ...event, trid };
      trid++;
    }
    return null;
  }
  
  replaceEvent(id, nevent) {
    for (const track of this.tracks) {
      const p = track.events.findIndex(e => e.id === id);
      if (p < 0) continue;
      track.events[p] = { ...nevent, id };
      return true;
    }
    return false;
  }
  
  /* Generate a sequence of {when,events:{...event,trid}}, containing every event, bucketted by time.
   * (trid) is the zero-based track index.
   */
  *eventsByTime() {
    for (let pv=this.tracks.map(() => 0); ; ) {
      let nextTime, finished=true;
      for (let i=pv.length; i-->0; ) {
        const track = this.tracks[i];
        if (pv[i] >= track.events.length) continue;
        const event = track.events[pv[i]];
        if (finished) {
          finished = false;
          nextTime = event.when;
        } else if (event.when < nextTime) {
          nextTime = event.when;
        }
      }
      if (finished) break;
      const result = {
        when: nextTime,
        events: [],
      };
      for (let i=pv.length; i-->0; ) {
        const track = this.tracks[i];
        while ((pv[i] < track.events.length) && (track.events[pv[i]].when === nextTime)) {
          result.events.push({
            ...track.events[pv[i]],
            trid: i,
          });
          pv[i]++;
        }
      }
      yield result;
    }
  }
  
  /* Decode.
   *********************************************************************************************/
  
  _initDecode(src) {
    this.tracks = [];
    this.division = 0;
    const textDecoder = new TextDecoder("utf8");
    for (let srcp=0; srcp<src.length; ) {
      let chunkid = textDecoder.decode(src.slice(srcp, srcp + 4));
      srcp += 4;
      let len = src[srcp++] << 24;
      len |= src[srcp++] << 16;
      len |= src[srcp++] << 8;
      len |= src[srcp++];
      if ((len < 0) || (srcp > src.length - len)) {
        throw new Error(`Invalid chunk around ${srcp - 8}/${src.length}`);
      }
      switch (chunkid) {
        case "MThd": this._decodeMThd(src.slice(srcp, srcp + len)); break;
        case "MTrk": this._decodeMTrk(src.slice(srcp, srcp + len)); break;
      }
      srcp += len;
    }
    if (!this.division || !this.tracks.length) throw new Error(`Invalid MIDI file`);
  }
  
  _decodeMThd(src) {
    const format = (src[0] << 8) | src[1];
    const trackc = (src[2] << 8) | src[3];
    const division = (src[4] << 8) | src[5];
    if (!division || (division & 0x8000)) throw new Error(`Invalid division ${division} in MThd`);
    this.division = division;
  }
  
  _decodeMTrk(src) {
    const track = {
      events: [],
    };
    let status=0, when=0;
    for (let srcp=0; srcp<src.length; ) {
    
      const [delay, dlen] = this.decodeVlq(src, srcp);
      srcp += dlen;
      when += delay;
      
      if (src[srcp] & 0x80) {
        status = src[srcp++];
      } else if (status & 0x80) {
      } else {
        throw new Error(`Invalid leading byte ${src[srcp]} at ${srcp}/${src.length} in MTrk ${this.tracks.length}`);
      }
      
      let opcode = status & 0xf0;
      let chid = status & 0x0f;
      switch (opcode) {
      
        case 0x80: { // Note Off.
            // We don't record these. Find the corresponding Note On and fill in its duration and offVelocity.
            // Note Off must happen on the same track as Note On, for us to detect it.
            // I'm not sure that the spec requires that, though it would certainly be weird not to.
            const noteid = src[srcp++];
            const velocity = src[srcp++];
            this._endNote(track, chid, noteid, velocity, when);
          } break;
          
        case 0x90: { // Note On.
            const noteid = src[srcp++];
            const velocity = src[srcp++];
            if (!velocity) { // Note On with velocity zero means Note Off with velocity 0x40.
              this._endNote(track, chid, noteid, 0x40, when);
            } else {
              track.events.push({ id: this.nextId++, when, chid, opcode, a: noteid, b: velocity, duration: 0, offVelocity: 0 });
            }
          } break;
          
        case 0xa0: // Note Adjust.
        case 0xb0: // Control Change.
        case 0xe0: // Pitch Wheel.
          {
            const a = src[srcp++];
            const b = src[srcp++];
            track.events.push({ id: this.nextId++, when, chid, opcode, a, b });
          } break;
          
        case 0xc0: // Program Change.
        case 0xd0: // Channel Pressure.
          {
            const a = src[srcp++];
            track.events.push({ id: this.nextId++, when, chid, opcode, a, b: 0 });
          } break;
          
        case 0xf0: { // Meta or Sysex.
            opcode = status;
            status = 0;
            chid = 0xff;
            let a = 0;
            if (opcode === 0xff) { // Meta has a leading "type" byte, which we'll report as "a".
              a = src[srcp++];
            }
            // Opcode must be 0xff=Meta, 0xf0=Sysex, or 0xf7=Sysex, but we'll allow 0xf-anything.
            const [paylen, paylenlen] = this.decodeVlq(src, srcp);
            srcp += paylenlen;
            const v = src.slice(srcp, srcp + paylen);
            srcp += paylen;
            track.events.push({ id: this.nextId++, when, chid, opcode, a, b: 0, v });
          } break;
          
        default: throw new Error("oh no this isnt possible");
      }
    }
    this.tracks.push(track);
  }
  
  // => [value, length]
  decodeVlq(src, srcp) {
    let v=0, c=0;
    while (src[srcp + c] & 0x80) {
      v <<= 7;
      v |= src[srcp + c] & 0x7f;
      c++;
      if (c > 4) throw new Error(`Invalid VLQ`);
    }
    if (srcp >= src.length) throw new Error(`Invalid VLQ`);
    v <<= 7;
    v |= src[srcp + c];
    c++;
    return [v, c];
  }
  
  _endNote(track, chid, noteid, velocity, when) {
    for (let i=track.events.length; i-->0; ) {
      const event = track.events[i];
      if (event.chid !== chid) continue;
      if (event.opcode !== 0x90) continue;
      if (event.a !== noteid) continue;
      if (event.duration) return; // Note not currently held.
      event.duration = when - event.when;
      event.offVelocity = velocity;
      return;
    }
    // Note not currently held.
  }
  
  /* Encode.
   **********************************************************************************/
  
  encode() {
    const encoder = new Encoder();
    
    encoder.string("MThd");
    encoder.u32(6);
    encoder.u16(1); // Format
    encoder.u16(this.tracks.length);
    encoder.u16(this.division);
    
    for (const track of this.tracks) {
      encoder.string("MTrk");
      const lenp = encoder.placeholderU32();
      const events = this._flattenEventsForEncode(track.events);
      let status = 0;
      for (const event of events) {
        
        encoder.vlq(event.when);
        
        if (event.opcode >= 0xf0) {
          encoder.u8(event.opcode);
          if (event.opcode === 0xff) encoder.u8(event.a);
          encoder.vlq(event.v.length);
          encoder.raw(event.v);
          status = 0;
          
        } else {
          const nstatus = event.opcode | event.chid;
          if (nstatus !== status) {
            encoder.u8(nstatus);
            status = nstatus;
          }
          encoder.u8(event.a);
          switch (event.opcode) {
            case 0x80: case 0x90: case 0xa0: case 0xb0: case 0xe0: encoder.u8(event.b); break;
          }
        }
      }
      encoder.lenU32(lenp);
    }
    
    return encoder.finish();
  }
  
  /* Return this events list with some adjustments:
   *  - (when) are relative to the prior event. (they're absolute in storage).
   *  - Note Offs are inserted for each Note On.
   * So the only non-LTI thing you need to worry about is Running Status.
   */
  _flattenEventsForEncode(events) {
    const dst = [];
    const sorted = [...events];
    for (let i=sorted.length; i-->0; ) {
      const event = sorted[i];
      if (event.opcode === 0x90) {
        sorted.push({
          when: event.when + event.duration,
          chid: event.chid,
          opcode: 0x80,
          a: event.a,
          b: event.offVelocity,
        });
      }
    }
    sorted.sort((a, b) => a.when - b.when);
    let when = 0;
    for (const event of sorted) {
      dst.push({
        ...event,
        when: event.when - when,
      });
      when = event.when;
    }
    return dst;
  }
}

/* Generic Encoder.
 **********************************************************************/
 
class Encoder {
  constructor() {
    this.v = new Uint8Array(4096);
    this.c = 0;
  }
  
  finish() {
    const dst = new Uint8Array(this.c);
    const srcview = new Uint8Array(this.v.buffer, 0, this.c);
    dst.set(srcview);
    return dst;
  }
  
  require(addc) {
    if (addc < 1) return;
    if (this.c + addc <= this.v.length) return;
    let na = (this.c + addc + 1024) & ~1023;
    const nv = new Uint8Array(na);
    nv.set(this.v);
    this.v = nv;
  }
  
  u8(src) {
    this.require(1);
    this.v[this.c++] = src;
  }
  
  u16(src) {
    this.require(2);
    this.v[this.c++] = src >> 8;
    this.v[this.c++] = src;
  }
  
  u32(src) {
    this.require(4);
    this.v[this.c++] = src >> 24;
    this.v[this.c++] = src >> 16;
    this.v[this.c++] = src >> 8;
    this.v[this.c++] = src;
  }
  
  vlq(src) {
    this.require(4);
    if (src >= 0x10000000) {
      throw new Error(`Integer ${src} not representable in 4 bytes of VLQ`);
    } if (src >= 0x200000) {
      this.v[this.c++] = 0x80 | (src >> 21);
      this.v[this.c++] = 0x80 | (src >> 14);
      this.v[this.c++] = 0x80 | (src >> 7);
      this.v[this.c++] = src & 0x7f;
    } else if (src >= 0x4000) {
      this.v[this.c++] = 0x80 | (src >> 14);
      this.v[this.c++] = 0x80 | (src >> 7);
      this.v[this.c++] = src & 0x7f;
    } else if (src >= 0x80) {
      this.v[this.c++] = 0x80 | (src >> 7);
      this.v[this.c++] = src & 0x7f;
    } else if (src >= 0) {
      this.v[this.c++] = src;
    } else {
      throw new Error(`${src} not representable as VLQ`);
    }
  }
  
  string(src) {
    this.require(src.length);
    for (let i=0; i<src.length; i++) this.v[this.c++] = src.charCodeAt(i);
  }
  
  raw(src) {
    this.require(src.length);
    const dstview = new Uint8Array(this.v.buffer, this.c, src.length);
    dstview.set(src);
    this.c += src.length;
  }
  
  /* Helper for chunk lengths.
   * Call placeholderU32() and record its result.
   * Then encode the chunk, and call lenU32() with that result when finished.
   */
  placeholderU32() {
    const p = this.c;
    this.u32(0);
    return p;
  }
  lenU32(p) {
    if ((p < 0) || (p + 4 > this.c)) throw new Error(`Invalid placeholder ${p}`);
    const len = this.c - p - 4;
    this.v[p++] = len >> 24;
    this.v[p++] = len >> 16;
    this.v[p++] = len >> 8;
    this.v[p] = len;
  }
}
