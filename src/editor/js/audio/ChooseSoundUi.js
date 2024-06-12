/* ChooseSoundUi.js
 * Sits in front of SfgEditor, to choose or create a sound in a multi-resource file.
 */
 
import { Dom } from "../Dom.js";
import { Bus } from "../Bus.js";
import { Resmgr } from "../Resmgr.js";
import { SfgMultiFile } from "./SfgMultiFile.js";

export class ChooseSoundUi {
  static getDependencies() {
    return [HTMLElement, Dom, Bus, Resmgr];
  }
  constructor(element, dom, bus, resmgr) {
    this.element = element;
    this.dom = dom;
    this.bus = bus;
    this.resmgr = resmgr;
    this.path = "";
    this.sfgMultiFile = null;
    this.buildUi();
  }
  
  setup(serial, path) {
    this.path = path;
    this.sfgMultiFile = new SfgMultiFile(serial);
    this.element.classList.remove("hidden");
    this.rebuildCards();
  }
  
  rebuildCards() {
    for (const card of this.element.querySelectorAll(".card.sound")) card.remove();
    for (const sound of this.sfgMultiFile.sounds) {
      const card = this.dom.spawn(this.element, "DIV", ["card", "sound"]);
      const buttonRow = this.dom.spawn(card, "DIV", ["buttonRow"]);
      this.dom.spawn(buttonRow, "INPUT", { type: "button", value: sound.id, "on-click": () => this.onEditSound(sound.id) });
      this.dom.spawn(buttonRow, "INPUT", { type: "button", value: "X", "on-click": () => this.onDeleteSound(sound.id) });
      const alias = this.aliasForSoundId(sound.id);
      if (alias) this.dom.spawn(card, "DIV", ["alias"], alias);
    }
  }
  
  buildUi() {
    this.element.innerHTML = "";
    const card = this.dom.spawn(this.element, "DIV", ["card", "new"]);
    this.dom.spawn(card, "INPUT", { type: "button", value: "New", "on-click": () => this.onNewSound() });
  }
  
  aliasForSoundId(id) {
    const n = +id;
    if (n >= 35) {
      const alias = ChooseSoundUi.MIDI_DRUM_NAMES[n - 35];
      if (alias) return alias;
    }
    return "";
  }
  
  onNewSound() {
    if (!this.sfgMultiFile) return;
    this.dom.modalInput("New sound ID:", this.sfgMultiFile.getUnusedId()).then(id => {
      if (!id) return;
      if (!id.match(/^[0-9a-zA-Z_]+$/)) return;
      const sound = { id, text: "" };
      this.sfgMultiFile.sounds.push(sound);
      this.rebuildCards();
      this.resmgr.dirty(this.path, () => this.sfgMultiFile.encode());
      this.bus.broadcast({ type: "editSound", id, text: sound.text });
    }).catch(() => {});
  }
  
  onDeleteSound(id) {
    if (!this.sfgMultiFile) return;
    this.dom.modalPickOne(["Cancel, keep it", "Yes, delete it"], `Really delete sound ${JSON.stringify(id)}?`).then(choice => {
      if (!choice.startsWith("Yes")) return;
      const p = this.sfgMultiFile.sounds.findIndex(s => s.id === id);
      if (p < 0) return;
      this.sfgMultiFile.sounds.splice(p, 1);
      this.rebuildCards();
      this.resmgr.dirty(this.path, () => this.sfgMultiFile.encode());
    }).catch(() => {});
  }
  
  onEditSound(id) {
    if (!this.sfgMultiFile) return;
    const sound = this.sfgMultiFile.sounds.find(s => s.id === id);
    if (!sound) return;
    this.bus.broadcast({ type: "editSound", id, text: sound.text });
  }
}

ChooseSoundUi.MIDI_DRUM_NAMES = [ // First is sound 35.
  "Acoustic Bass Drum",
  "Bass Drum 1",
  "Side Stick",
  "Acoustic Snare",
  "Hand Clap",
  "Electric Snare",
  "Low Floor Tom",
  "Closed Hi Hat",
  "High Floor Tom",
  "Pedal Hi-Hat",
  "Low Tom",
  "Open Hi-Hat",
  "Low-Mid Tom",
  "Hi Mid Tom",
  "Crash Cymbal 1",
  "High Tom",
  "Ride Cymbal 1",
  "Chinese Cymbal",
  "Ride Bell",
  "Tambourine",
  "Splash Cymbal",
  "Cowbell",
  "Crash Cymbal 2",
  "Vibraslap",
  "Ride Cymbal 2",
  "Hi Bongo",
  "Low Bongo",
  "Mute Hi Conga",
  "Open Hi Conga",
  "Low Conga",
  "High Timbale",
  "Low Timbale",
  "High Agogo",
  "Low Agogo",
  "Cabasa",
  "Maracas",
  "Short Whistle",
  "Long Whistle",
  "Short Guiro",
  "Long Guiro",
  "Claves",
  "Hi Wood Block",
  "Low Wood Block",
  "Mute Cuica",
  "Open Cuica",
  "Mute Triangle",
  "Open Triangle",
];
