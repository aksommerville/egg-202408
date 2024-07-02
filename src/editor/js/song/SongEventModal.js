/* SongEventModal.js
 * Suitable for "new event" or "edit event".
 * Events:
 *   trid: number
 *   when: number (ticks)
 *   chid: number, 255 if channelless
 *   opcode: number
 *   a: number
 *   b: number (mandatory even if unused)
 *   v?: Uint8Array, mandatory for Meta and Sysex
 *   duration?: number (ticks), mandatory for Note On
 *   offVelocity?: number, mandatory for Note On
 */
 
import { Dom } from "../Dom.js";

export class SongEventModal {
  static getDependencies() {
    return [HTMLDialogElement, Dom];
  }
  constructor(element, dom) {
    this.element = element;
    this.dom = dom;
    
    this.event = null;
  }
  
  setup(event) {
    if (event) {
      this.event = {...event};
    } else {
      this.event = {
        trid: 0,
        when: 0,
        chid: 0xff,
        opcode: 0,
        a: 0,
        b: 0,
      };
    }
    this.buildUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    const form = this.dom.spawn(this.element, "FORM", { action: "#", "on-submit": e => e.preventDefault() });
    const table = this.dom.spawn(form, "TABLE");
    this.spawnNumber(table, "when", "Time");
    this.spawnNumber(table, "trid", "Track");
    this.spawnNumber(table, "chid", "Channel");
    const opcode = this.spawnSelect(table, "opcode", "Opcode");
    this.dom.spawn(opcode, "OPTION", { value: 0x90 }, "0x90 Note On/Off");
    this.dom.spawn(opcode, "OPTION", { value: 0xa0 }, "0xa0 Note Adjust");
    this.dom.spawn(opcode, "OPTION", { value: 0xb0 }, "0xb0 Control Change");
    this.dom.spawn(opcode, "OPTION", { value: 0xc0 }, "0xc0 Program Change");
    this.dom.spawn(opcode, "OPTION", { value: 0xd0 }, "0xd0 Channel Pressure");
    this.dom.spawn(opcode, "OPTION", { value: 0xe0 }, "0xe0 Pitch Wheel");
    this.dom.spawn(opcode, "OPTION", { value: 0xf0 }, "0xf0 Sysex");
    this.dom.spawn(opcode, "OPTION", { value: 0xf7 }, "0xf7 Sysex");
    this.dom.spawn(opcode, "OPTION", { value: 0xff }, "0xff Meta");
    opcode.value = this.event.opcode;
    switch (this.event.opcode) {
      case 0x90: {
          this.spawnNumber(table, "a", "Noteid");
          this.spawnNumber(table, "b", "Velocity");
          this.spawnNumber(table, "duration", "Duration");
          this.spawnNumber(table, "offVelocity", "Off Velocity");
        } break;
      case 0xa0: {
          this.spawnNumber(table, "a", "Noteid");
          this.spawnNumber(table, "b", "Pressure");
        } break;
      case 0xb0: {
          const k = this.spawnSelect(table, "a", "Key");
          for (let i=0; i<0x80; i++) {
            this.dom.spawn(k, "OPTION", { value: i }, `${i}: ${CONTROL_KEYS[i] || ''}`);
          }
          k.value = this.event.a;
          this.spawnNumber(table, "b", "Value");
        } break;
      case 0xc0: {
          const pgm = this.spawnSelect(table, "a", "Program");
          for (let i=0; i<0x80; i++) {
            this.dom.spawn(pgm, "OPTION", { value: i }, `${i}: ${GM_PROGRAM_NAMES[i] || ''}`);
          }
          pgm.value = this.event.a;
        } break;
      case 0xd0: {
          this.spawnNumber(table, "a", "Pressure");
        } break;
      case 0xe0: {
          this.spawnNumber(table, "a", "LSB(7)");
          this.spawnNumber(table, "b", "MSB(7)");
        } break;
      case 0xf0:
      case 0xf7: {
          this.spawnHexDump(table);
        } break;
      case 0xff: {
          // We have a partial list of Meta keys and could do a select. But it's so sparse, it feels wrong.
          this.spawnNumber(table, "a", "Type");
          this.spawnHexDump(table);
        } break;
    }
    this.dom.spawn(form, "INPUT", { type: "submit", value: "OK", "on-click": e => {
      e.stopPropagation();
      e.preventDefault();
      this.resolve(this.gatherValue());
    }});
  }
  
  spawnNumber(table, k, lbl) {
    const tr = this.dom.spawn(table, "TR");
    this.dom.spawn(tr, "TD", ["key"], lbl);
    const tdv = this.dom.spawn(tr, "TD");
    return this.dom.spawn(tdv, "INPUT", { type: "number", name: k, value: this.event[k] });
  }
  
  spawnSelect(table, k, lbl) {
    const tr = this.dom.spawn(table, "TR");
    this.dom.spawn(tr, "TD", ["key"], lbl);
    const tdv = this.dom.spawn(tr, "TD");
    return this.dom.spawn(tdv, "SELECT", { name: k, value: this.event[k] });
  }
  
  spawnHexDump(table) {
    const tr = this.dom.spawn(table, "TR");
    this.dom.spawn(tr, "TD", ["key"], "Payload");
    const tdv = this.dom.spawn(tr, "TD");
    return this.dom.spawn(tdv, "INPUT", { type: "text", name: "v", value: this.reprHex(this.event.v) });
  }
  
  reprHex(src) {
    if (!src) return "";
    let dst = "";
    for (let i=0; i<src.length; i++) {
      dst += "0123456789abcdef"[src[i] >> 4];
      dst += "0123456789abcdef"[src[i] & 15];
      dst += " ";
    }
    return dst;
  }
  
  evalHex(src) {
    src = src.replace(/\s/g, "");
    const dst = new Uint8Array(src.length >> 1);
    let dstc=0;
    for (let srcp=0; srcp<src.length; srcp+=2) {
      dst[dstc++] = parseInt(src.substring(srcp, srcp+2), 16);
    }
    return dst;
  }
  
  gatherValue() {
    const result = {};
    for (const input of this.element.querySelectorAll("input[name],select[name]")) {
      if (input.name === "v") {
        result.v = this.evalHex(input.value);
      } else {
        result[input.name] = +input.value;
      }
    }
    return result;
  }
}

export const CONTROL_KEYS = [
"MSB Bank","MSB Mod","MSB Breath","MSB ?","MSB Foot","MSB Portamento Time","MSB Data Entry","MSB Volume",
"MSB Balance","MSB ?","MSB Pan","MSB Expression","MSB Effect 1","MSB Effect 2","MSB Effect 3","MSB Effect 4",
"MSB GP 1","MSB GP 2","MSB GP 3","MSB GP 4","MSB ?","MSB ?","MSB ?","MSB ?",
"MSB ?","MSB ?","MSB ?","MSB ?","MSB ?","MSB ?","MSB ?","MSB ?",
"LSB Bank","LSB Mod","LSB Breath","LSB ?","LSB Foot","LSB Portamento Time","LSB Data Entry","LSB Volume",
"LSB Balance","LSB ?","LSB Pan","LSB Expression","LSB Effect 1","LSB Effect 2","LSB Effect 3","LSB Effect 4",
"LSB GP 1","LSB GP 2","LSB GP 3","LSB GP 4","LSB ?","LSB ?","LSB ?","LSB ?",
"LSB ?","LSB ?","LSB ?","LSB ?","LSB ?","LSB ?","LSB ?","LSB ?",
"Sustain Switch","Portamento Switch","Sustenuto Switch","Soft Switch","Legato Switch","Hold 2 Switch","Controller 1 (Sound Variation)","Controller 2 (Timbre)",
"Controller 3 (Release Time)","Controller 4 (Attack Time)","Controller 5 (Brightness)","Controller 6","Controller 7","Controller 8","Controller 9","Controller 10",
"GP 5","GP 6","GP 7","GP 8","Portamento Control (noteid)","?","?","?",
"?","?","?","Effect 1 Depth","Effect 2 Depth","Effect 3 Depth","Effect 4 Depth","Effect 5 Depth",
"?","?","?","?","?","?","?","?",
"?","?","?","?","?","?","?","?",
"?","?","?","?","?","?","?","?",
"All Sound Off","Reset Controllers","Local Controller Switch","All Notes Off","Omni Off (+Notes Off)","Omni On (+Notes Off)","Poly Switch (+Notes Off)","Poly On (+Notes Off)",
];

export const GM_PROGRAM_NAMES = [
"Acoustic Grand Piano","Bright Acoustic Piano","Electric Grand Piano","Honky","Electric Piano 1 (Rhodes Piano)","Electric Piano 2 (Chorused Piano)","Harpsichord","Clavinet",
"Celesta","Glockenspiel","Music Box","Vibraphone","Marimba","Xylophone","Tubular Bells","Dulcimer (Santur)",
"Drawbar Organ (Hammond)","Percussive Organ","Rock Organ","Church Organ","Reed Organ","Accordion (French)","Harmonica","Tango Accordion (Band neon)",
"Acoustic Guitar (nylon)","Acoustic Guitar (steel)","Electric Guitar (jazz)","Electric Guitar (clean)","Electric Guitar (muted)","Overdriven Guitar","Distortion Guitar","Guitar harmonics",
"Acoustic Bass","Electric Bass (fingered)","Electric Bass (picked)","Fretless Bass","Slap Bass 1","Slap Bass 2","Synth Bass 1","Synth Bass 2",
"Violin","Viola","Cello","Contrabass","Tremolo Strings","Pizzicato Strings","Orchestral Harp","Timpani",
"String Ensemble 1 (strings)","String Ensemble 2 (slow strings)","SynthStrings 1","SynthStrings 2","Choir Aahs","Voice Oohs","Synth Voice","Orchestra Hit",
"Trumpet","Trombone","Tuba","Muted Trumpet","French Horn","Brass Section","SynthBrass 1","SynthBrass 2",
"Soprano Sax","Alto Sax","Tenor Sax","Baritone Sax","Oboe","English Horn","Bassoon","Clarinet",
"Piccolo","Flute","Recorder","Pan Flute","Blown Bottle","Shakuhachi","Whistle","Ocarina",
"Lead 1 (square wave)","Lead 2 (sawtooth wave)","Lead 3 (calliope)","Lead 4 (chiffer)","Lead 5 (charang)","Lead 6 (voice solo)","Lead 7 (fifths)","Lead 8 (bass + lead)",
"Pad 1 (new age Fantasia)","Pad 2 (warm)","Pad 3 (polysynth)","Pad 4 (choir space voice)","Pad 5 (bowed glass)","Pad 6 (metallic pro)","Pad 7 (halo)","Pad 8 (sweep)",
"FX 1 (rain)","FX 2 (soundtrack)","FX 3 (crystal)","FX 4 (atmosphere)","FX 5 (brightness)","FX 6 (goblins)","FX 7 (echoes, drops)","FX 8 (sci",
"Sitar","Banjo","Shamisen","Koto","Kalimba","Bag pipe","Fiddle","Shanai",
"Tinkle Bell","Agogo","Steel Drums","Woodblock","Taiko Drum","Melodic Tom","Synth Drum","Reverse Cymbal",
"Guitar Fret Noise","Breath Noise","Seashore","Bird Tweet","Telephone Ring","Helicopter","Applause","Gunshot",
];
