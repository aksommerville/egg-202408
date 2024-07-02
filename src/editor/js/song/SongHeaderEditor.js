/* SongHeaderEditor.js
 * Editor for beeeeeP or certain MIDI files, showing only global header and channel config.
 */
 
import { Dom } from "../Dom.js";
import { Comm } from "../Comm.js";
import { Song } from "./Song.js";
import { GM_PROGRAM_NAMES } from "./SongEventModal.js";
import { Resmgr } from "../Resmgr.js";

const DIRTY_DEBOUNCE_MS = 100;
const RENDER_DEBOUNCE_MS = 50;

export class SongHeaderEditor {
  static getDependencies() {
    return [HTMLElement, Dom, Comm, Window, Resmgr];
  }
  constructor(element, dom, comm, window, resmgr) {
    this.element = element;
    this.dom = dom;
    this.comm = comm;
    this.window = window;
    this.resmgr = resmgr;
    
    this.model = null;
    this.song = null;
    this.path = "";
    this.dirtyDebounce = null;
    this.playheadListener = this.comm.listenPlayhead(v => this.onPlayhead(v));
    this.renderDebounce = null;
    this.playhead = 0;
  }
  
  onRemoveFromDom() {
    if (this.dirtyDebounce) {
      this.window.clearTimeout(this.dirtyDebounce);
      this.dirtyDebounce = null;
    }
    if (this.renderDebounce) {
      this.window.clearTimeout(this.renderDebounce);
      this.renderDebounce = null;
    }
    this.comm.unlistenPlayhead(this.playheadListener);
    this.onStop();
  }
  
  setup(serial, path) {
    this.song = new Song(serial);
    this.path = path;
    this.model = this.song.getHeaders();
    this.buildUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    
    const panlist = this.dom.spawn(this.element, "DATALIST", { id: "SongHeaderEditor-panlist" });
    this.dom.spawn(panlist, "OPTION", { value: -1 }, "Left");
    this.dom.spawn(panlist, "OPTION", { value: 0 }, "Center");
    this.dom.spawn(panlist, "OPTION", { value: 1 }, "Right");
    
    const playback = this.dom.spawn(this.element, "DIV", ["playback"]);
    this.dom.spawn(playback, "CANVAS", ["playhead"], { "on-click": e => this.onPlayheadClick(e) });
    this.dom.spawn(playback, "INPUT", { type: "button", value: "Play", "on-click": () => this.onPlay() });
    this.dom.spawn(playback, "INPUT", { type: "button", value: "Stop", "on-click": () => this.onStop() });
    
    const global = this.dom.spawn(this.element, "TABLE", ["global"]);
    const tempoTd = this.spawnKvRow(global, "tempo");
    this.dom.spawn(tempoTd, "INPUT", { name: "tempo", value: this.model.tempo, type: "number", "on-input": () => this.onDirty() });
    this.dom.spawn(tempoTd, "SPAN", "bpm. Restart playback to apply tempo changes.");
    
    const channels = this.dom.spawn(this.element, "TABLE", ["channels"], { "on-input": () => this.onDirty() });
    const chanhdr = this.dom.spawn(channels, "TR");
    this.dom.spawn(chanhdr, "TH", ["label"], "Channel");
    this.dom.spawn(chanhdr, "TH", ["pid"], "Program");
    this.dom.spawn(chanhdr, "TH", ["volume"], "Volume");
    this.dom.spawn(chanhdr, "TH", ["pan"], "Pan");
    for (const ch of this.model.channels) {
      const tr = this.dom.spawn(channels, "TR");
      this.dom.spawn(tr, "TD", ["key"], `${ch.chid}: ${ch.label}`);
      this.spawnPidTd(tr, ch.chid, ch.pid);
      this.spawnVolumeTd(tr, ch.chid, ch.volume);
      this.spawnPanTd(tr, ch.chid, ch.pan);
    }
    
    this.renderSoon();
  }
  
  spawnKvRow(table, key) {
    const tr = this.dom.spawn(table, "TR", { "data-key": key });
    this.dom.spawn(tr, "TD", ["key"], key);
    return this.dom.spawn(tr, "TD", ["value"]);
  }
  
  spawnPidTd(tr, chid, value) {
    const td = this.dom.spawn(tr, "TD");
    const select = this.dom.spawn(td, "SELECT", { name: `${chid}-pid`, value });
    for (let pid=0; pid<256; pid++) {
      let label;
      if (pid >= 128) label = `Drums ${(pid - 128) * 128}..${(pid - 128) * 128 + 127}`;
      else if (!(label = GM_PROGRAM_NAMES[pid])) label = "";
      label = "0x" + pid.toString(16).padStart(2, '0') + " " + label;
      this.dom.spawn(select, "OPTION", { value: pid }, label);
    }
    select.value = value;
  }
  
  spawnVolumeTd(tr, chid, value) {
    const td = this.dom.spawn(tr, "TD");
    const input = this.dom.spawn(td, "INPUT", { name: `${chid}-volume`, type: "range", min: 0, max: 1, step: 0.01, value });
  }
  
  spawnPanTd(tr, chid, value) {
    const td = this.dom.spawn(tr, "TD");
    const list = `SongHeaderEditor-panlist`;
    const input = this.dom.spawn(td, "INPUT", { name: `${chid}-pan`, type: "range", min: -1, max: 1, step: 0.01, value, list });
  }
  
  updateSongFromUi() {
    for (const input of this.element.querySelectorAll("input,select")) {
    
      if (input.name === "tempo") {
        const bpm = +this.element.querySelector("input[name='tempo']").value || 1;
        const usPerQnote = Math.max(1, Math.min(0xffffff, Math.round(60000000 / bpm)));
        this.song.setTempo(usPerQnote);
        
      } else {
        const split = input.name.split('-');
        if (split.length !== 2) continue;
        const chid = +split[0] || 0;
        if ((chid < 0) || (chid > 7)) continue;
        const key = split[1];
        switch (key) {
          case "pid": {
              this.song.setHeaderEvent(chid, 0xb0, 0x20, input.value >> 7);
              this.song.setHeaderEvent(chid, 0xc0, input.value & 0x7f, 0);
            } break;
          case "volume": {
              const v = Math.max(0, Math.min(0x7f, Math.round(input.value * 0x7f)));
              this.song.setHeaderEvent(chid, 0xb0, 0x07, v);
            } break;
          case "pan": {
              const v = Math.max(0, Math.min(0x7f, Math.round(input.value * 0x40 + 0x40)));
              this.song.setHeaderEvent(chid, 0xb0, 0x0a, v);
            } break;
        }
      }
    }
  }
  
  gatherAdjustBodyFromUi() {
    const serial = new Uint8Array(42);
    for (const input of this.element.querySelectorAll("input,select")) {
    
      if (input.name === "tempo") {
        // Backend can't change tempo on the fly. Note timing is directly in milliseconds, they'd have to visit every note and risk heavy rounding error.
        // So live adjustment of the tempo is not possible without reencoding the song. Let's skip it for now.
        
      } else {
        const split = input.name.split('-');
        if (split.length !== 2) continue;
        const chid = +split[0] || 0;
        if ((chid < 0) || (chid > 7)) continue;
        const key = split[1];
        switch (key) {
          case "pid": {
              serial[10 + chid * 4] = +input.value;
            } break;
          case "volume": {
              const v = Math.max(0, Math.min(0xff, +input.value * 0xff));
              serial[10 + chid * 4 + 1] = v;
            } break;
          case "pan": {
              const v = Math.max(0, Math.min(0xff, (+input.value + 1.0) * 0x80));
              serial[10 + chid * 4 + 2] = v;
            } break;
        }
      }
    }
    return serial;
  }
  
  renderSoon() {
    if (this.renderDebounce) return;
    this.renderDebounce = this.window.setTimeout(() => {
      this.renderDebounce = null;
      this.renderNow();
    }, RENDER_DEBOUNCE_MS);
  }
  
  renderNow() {
    const canvas = this.element.querySelector("canvas.playhead");
    const bounds = canvas.getBoundingClientRect();
    canvas.width = bounds.width;
    canvas.height = bounds.height;
    const ctx = canvas.getContext("2d");
    ctx.fillStyle = "#020";
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    ctx.fillStyle = "#440";
    ctx.fillRect(this.playhead * canvas.width - 8, 0, 16, canvas.height);
    ctx.fillStyle = "#ff0";
    ctx.fillRect(this.playhead * canvas.width - 1, 0, 2, canvas.height);
  }
  
  onPlayheadClick(event) {
    const canvas = this.element.querySelector("canvas.playhead");
    const bounds = canvas.getBoundingClientRect();
    const v = Math.max(0, Math.min(1, (event.x - bounds.x) / bounds.width));
    this.comm.setPlayhead(v);
  }
  
  onPlay() {
    const serial = this.song.encode();
    this.comm.httpText("POST", "/api/song", null, null, serial).then(rsp => {
      // ok!
    }).catch(error => {
      console.log(`Play song failed`, error);
    });
  }
  
  onStop() {
    this.comm.httpText("POST", "/api/song", null, null, null).catch(() => {});
  }
  
  /* Changes to globals or channels -- anything that changes the song data.
   * Resmgr et al do their own saving debounce, but we do another one gating playback calls.
   */
  onDirty() {
    this.resmgr.dirty(this.path, () => {
      this.updateSongFromUi();
      return this.song.encode();
    });
    if (this.dirtyDebounce) this.window.clearTimeout(this.dirtyDebounce);
    this.dirtyDebounce = this.window.setTimeout(() => {
      this.dirtyDebounce = null;
      if (!this.playhead) return;
      const serial = this.gatherAdjustBodyFromUi();
      this.comm.httpBinary("POST", "/api/song/adjust", null, null, serial).then(rsp => {
      }).catch(error => {
        console.log(`Adjust song failed`, error);
      });
    }, DIRTY_DEBOUNCE_MS);
  }
  
  onPlayhead(v) {
    this.playhead = v;
    this.renderSoon();
  }
}
