/* Bus.js
 * State cache and event dispatcher.
 *
 * Events:
 *   {type:"status",status:"ok"|"error"|"pending"|"dead"}
 *   {type:"toc",toc}
 *   {type:"roms",roms:string[]}
 *   {type:"error",error:any}
 *   {type:"open",path,serial:Uint8Array}
 *
 * More events, from the older audio-only implementation:
 * { type:"play" }
 *   Request to play the current sound. From click in WaveVisualUi.
 *
 * { type:"newSound" }
 *   Request to delete the current model and start a fresh one.
 *   Replaces the global model stored here.
 *
 * { type:"editSound", id:string, text:string }
 * { type:"closeSound" }
 *
 * { type:"soundDirty", sender }
 *   Only Bus should dispatch. Clients, call Bus.soundDirty().
 *   Drops cached text, binary, and wave. Notify all that sound has changed.
 *
 * { type:"setTimeRange", timeRange:number }
 * { type:"setMaster", master:number } -- no associated soundDirty
 */
 
import { Sound } from "./audio/Sound.js";
import { SfgCompiler } from "./audio/SfgCompiler.js";
import { SfgPrinter } from "./audio/SfgPrinter.js";
 
export class Bus {
  static getDependencies() {
    return [];
  }
  constructor() {
    this.listeners = [];
    this.nextId = 1;
    
    // General state.
    this.status = "pending";
    this.toc = {}; // {name,files:*[]} or {name,serial}
    this.roms = []; // string
    // For audio:
    this.sound = new Sound();
    this.text = null; // string
    this.bin = null; // Uint8Array
    this.wave = null; // Float32Array
    this.timeRange = 1000; // ms, horizontal range of all envelopes, and effectively the duration limit.
    this.rate = 44100;
    this.autoPlay = false; // Must start false, to force some interaction before launching the WebAudio context.
  }
  
  listen(events, cb) {
    const id = this.nextId++;
    this.listeners.push({ events, cb, id });
    return id;
  }
  
  unlisten(id) {
    const p = this.listeners.findIndex(l => l.id === id);
    if (p >= 0) this.listeners.splice(p, 1);
  }
  
  broadcast(event) {
    if (!event?.type) throw new Error(`Events for broadcast must have a string 'type': ${JSON.stringify(event)}`);
    switch (event.type) {
      case "newSound": this.newSound(); break;
      case "setTimeRange": {
          if (!event.timeRange || (event.timeRange === this.timeRange)) return;
          this.timeRange = event.timeRange;
        } break;
      case "setMaster": {
          if (!event.master || (event.master === this.sound.master)) return;
          this.sound.master = event.master;
          this.text = null;
          this.bin = null;
          this.wave = null;
        } break;
    }
    for (const { cb, events } of this.listeners) {
      if (events.includes(event.type)) cb(event);
    }
  }
  
  setStatus(status) {
    if (status === this.status) return;
    this.status = status;
    this.broadcast({ type: "status", status });
  }
  
  setToc(toc) {
    this.toc = toc;
    this.broadcast({ type: "toc", toc });
  }
  
  setRoms(roms) {
    this.roms = roms;
    this.broadcast({ type: "roms", roms });
  }
  
  /* Global store, specific to audio.
   *************************************************************/
   
  newSound() {
    this.sound.clear();
    this.text = null;
    this.bin = null;
    this.wave = null;
    this.timeRange = 1000;
  }
  
  soundDirty(sender) {
    this.text = null;
    this.bin = null;
    this.wave = null;
    this.broadcast({ type: "soundDirty", sender });
  }
  
  replaceText(text, sender) {
    this.sound.decodeText(text);
    this.text = text;
    this.bin = null;
    this.wave = null;
    this.broadcast({ type: "soundDirty", sender });
  }
  
  replaceBin(bin, sender) { // XXX Not sure we're going to support this.
    this.sound.decodeBinary(bin);
    this.text = null;
    this.wave = null;
    this.broadcast({ type: "soundDirty", sender });
  }
  
  requireText() {
    try {
      if (!this.text) this.text = this.sound.encodeText();
    } catch (e) {
      console.error(e);
    }
    return this.text;
  }
  
  requireBin() {
    if (!this.bin) {
      try {
        this.bin = new SfgCompiler(this.requireText()).compile();
      } catch (e) {
        console.error(e);
      }
    }
    return this.bin;
  }
  
  requireWave() {
    if (!this.wave) {
      try {
        this.wave = new SfgPrinter(this.requireBin(), this.rate).print();
      } catch (e) {
        console.error(e);
      }
    }
    return this.wave;
  }
  
  setAutoPlay(autoPlay) {
    autoPlay = !!autoPlay;
    if (autoPlay === this.autoPlay) return;
    this.autoPlay = autoPlay;
    // I think no need for an event.
  }
}

Bus.singleton = true;
