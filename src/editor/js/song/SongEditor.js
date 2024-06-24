/* SongEditor.js
 * Top level editor for MIDI files.
 */
 
import { Dom } from "../Dom.js";
import { Resmgr } from "../Resmgr.js";
import { Bus } from "../Bus.js";
import { Song } from "./Song.js";
import { SongEventModal } from "./SongEventModal.js";

export class SongEditor {
  static getDependencies() {
    return [HTMLElement, Dom, Resmgr, Bus];
  }
  constructor(element, dom, resmgr, bus) {
    this.element = element;
    this.dom = dom;
    this.resmgr = resmgr;
    this.bus = bus;
    
    this.song = null;
    this.path = "";
    
    this.buildUi();
  }
  
  setup(serial, path) {
    this.path = path;
    this.song = new Song(serial);
    this.populateUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    const controls = this.dom.spawn(this.element, "DIV", ["controls"]);
    this.dom.spawn(controls, "INPUT", { type: "button", value: "+Event", "on-click": () => this.addEvent() });
    this.dom.spawn(controls, "INPUT", { type: "button", value: "+Track", "on-click": () => this.addTrack() });
    this.dom.spawn(controls, "INPUT", { type: "button", name: "division", value: "Division", "on-click": () => this.editDivision() });
    this.dom.spawn(this.element, "TABLE", ["main"]);
  }
  
  populateUi() {
    this.element.querySelector("input[name='division']").value = this.song ? `${this.song.division} ticks/qnote` : "Division";
    const table = this.element.querySelector("table.main");
    table.innerHTML = "";
    if (!this.song) return;
    let tr = this.dom.spawn(table, "TR", ["header"]);
    this.dom.spawn(tr, "TH", "Time");
    for (let i=0; i<this.song.tracks.length; i++) {
      this.dom.spawn(tr, "TH", `#${i}`, { "on-click": () => this.editTrack(i) });
    }
    for (const { when, events } of this.song.eventsByTime()) {
      tr = this.dom.spawn(table, "TR");
      this.dom.spawn(tr, "TD", when.toString());
      for (let trid=0; trid<this.song.tracks.length; trid++) {
        const td = this.dom.spawn(tr, "TD");
        for (const event of events) {
          if (event.trid !== trid) continue;
          const input = this.dom.spawn(td, "INPUT", ["event"], { type: "button", value: this.buttonLabelForEvent(event) });
          input.addEventListener("click", () => this.editEvent(event.id, input));
        }
      }
    }
  }
  
  buttonLabelForEvent(event) {
    switch (event.opcode) {
      case 0x90: return `${event.chid} #${event.a} ..${event.duration}`;
      case 0xa0: return `${event.chid} Adjust`;
      case 0xb0: return `${event.chid}.${event.a}=${event.b}`;
      case 0xc0: return `${event.chid} pgm ${event.a}`;
      case 0xd0: return `${event.chid} Pressure`;
      case 0xe0: return `${event.chid} Wheel`;
      case 0xf0: return "Sysex";
      case 0xf7: return "Sysex";
      case 0xff: switch (event.a) {
          case 0x2f: return "EOT";
          case 0x51: return `${(event.v[0] << 16) | (event.v[1] << 8) | event.v[2]} us/qnote`;
          default: return `Meta 0x${event.a.toString(16).padStart(2, '0')}`;
        }
      default: return event.opcode.toString();
    }
  }
  
  addEvent() {
    if (!this.song) return;
    const modal = this.dom.spawnModal(SongEventModal);
    modal.setup(null);
    modal.result.then(rsp => {
      console.log(`Edited event: ${JSON.stringify(rsp)}`);
    }).catch(() => {});
  }
  
  addTrack() {
    if (!this.song) return;
    this.song.addTrack();
    this.resmgr.dirty(this.path, () => this.song.encode());
    this.populateUi();
  }
  
  editDivision() {
    if (!this.song) return;
    this.dom.modal("Ticks/qnote:", this.song.division.toString()).then(rsp => {
      const division = +rsp;
      if (isNaN(division) || (division < 1) || (division >= 0x8000)) return;
      this.song.division = division;
      this.element.querySelector("input[name='division']").value = `${division} ticks/qnote`;
      this.resmgr.dirty(this.path, () => this.song.encode());
    }).catch(() => {});
  }
  
  editTrack(ix) {
    const evtc = this.song.tracks[ix].events.length;
    this.dom.modalPickOne(["Keep", "Delete"], `Really delete track ${ix}? Contains ${evtc} events.`).then(rsp => {
      if (rsp !== "Delete") return;
      this.song.deleteTrack(ix);
      this.resmgr.dirty(this.path, () => this.song.encode());
      this.populateUi();
    }).catch(() => {});
  }
  
  editEvent(id, button) {
    const before = this.song.getEventById(id);
    if (!before) return;
    const modal = this.dom.spawnModal(SongEventModal);
    modal.setup(before);
    modal.result.then(after => {
      this.song.replaceEvent(id, after);
      button.value = this.buttonLabelForEvent(after);
      this.resmgr.dirty(this.path, () => this.song.encode());
    }).catch(() => {});
  }
}
