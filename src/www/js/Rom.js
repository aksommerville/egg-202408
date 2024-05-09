export class Rom {
  constructor(serial) {
    if (serial instanceof ArrayBuffer) serial = new Uint8Array(serial);
    this.resv = []; // {tid,qual,rid,v:Uint8Array}, sorted
    this.empty = new Uint8Array(0);
    this.decode(serial);
  }
  
  getRes(tid, qual, rid) {
    let lo=0, hi=this.resv.length;
    while (lo < hi) {
      const ck = (lo + hi) >> 1;
      const q = this.resv[ck];
           if (tid < q.tid) hi = ck;
      else if (tid > q.tid) lo = ck + 1;
      else if (qual < q.qual) hi = ck;
      else if (qual > q.qual) lo = ck + 1;
      else if (rid < q.rid) hi = ck;
      else if (rid > q.rid) lo = ck + 1;
      else return q.v;
    }
    return this.empty;
  }
  
  decode(src) {
    
    // Header.
    if (src.length < 16) throw "Invalid ROM";
    if ((src[0] !== 0xea) || (src[1] !== 0x00) || (src[2] !== 0xff) || (src[3] !== 0xff)) throw "Invalid ROM";
    const hdrlen = (src[4] << 24) | (src[5] << 16) | (src[6] << 8) | src[7];
    const toclen = (src[8] << 24) | (src[9] << 16) | (src[10] << 8) | src[11];
    const heaplen = (src[12] << 24) | (src[13] << 16) | (src[14] << 8) | src[15];
    if ((hdrlen < 16) || (hdrlen + toclen + heaplen > src.length)) throw "Invalid ROM";
    const toc0 = hdrlen;
    const heap0 = toc0 + toclen;
    const eof = heap0 + heaplen;
    
    // Walk TOC and heap.
    let tocp=toc0, heapp=heap0, tid=1, qual=0, rid=1;
    const addres = (len) => {
      if ((tid > 0x63) || (qual > 0x3ff) || (rid > 0xffff)) throw "Invalid ROM";
      if (heapp > eof - len) throw "Invalid ROM";
      this.resv.push({ tid, qual, rid, v: new Uint8Array(src.buffer, heapp, len) });
      heapp += len;
      rid += 1;
    };
    while (tocp < heap0) {
      const lead = src[tocp++];
      if (!(lead & 0x80)) { // SMALL
        addres(lead);
      } else switch (lead & 0xe0) {
        case 0x80: { // MEDIUM
            if (tocp > heap0 - 2) throw "Invalid ROM";
            const len = (((lead & 0x1f) << 16) | (src[tocp] << 8) | src[tocp+1]) + 128;
            tocp += 2;
            addres(len);
          } break;
        case 0xa0: { // LARGE
            if (tocp > heap0 - 3) throw "Invalid ROM";
            const len = (((lead & 0x1f) << 24) | (src[tocp] << 16) | (src[tocp+1] << 8) | src[tocp+2]) + 2097279;
            tocp += 3;
            addres(len);
          } break;
        case 0xc0: { // QUAL, RID, or Reserved
            if (lead & 0x10) { // RID
              rid += lead & 0x0f;
            } else if (lead & 0x0c) { // Reserved
              throw "Invalid ROM";
            } else { // QUAL
              if (tocp > heap0 - 1) throw "Invalid ROM";
              const d = ((lead & 3) << 8) | src[tocp];
              tocp += 1;
              qual += d + 1;
              rid = 1;
            }
          } break;
        case 0xe0: { // TYPE
            const d = lead & 0x1f;
            tid += d + 1;
            qual = 0;
            rid = 1;
          } break;
      }
    }
  }
}

Rom.RESTYPE_metadata = 1;
Rom.RESTYPE_wasm = 2;
Rom.RESTYPE_string = 3;
Rom.RESTYPE_image = 4;
Rom.RESTYPE_song = 5;
Rom.RESTYPE_sound = 6;
