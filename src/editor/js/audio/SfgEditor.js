/* SfgEditor.js
 * Top level of sound effects editor.
 * This file was written for new Egg, and most of the other things it includes were from the 202405 version.
 */
 
import { Dom } from "../Dom.js";
import { Bus } from "../Bus.js";
import { Resmgr } from "../Resmgr.js";
import { InputUi } from "./InputUi.js";
import { OutputUi } from "./OutputUi.js";
import { PlaybackService } from "./PlaybackService.js";
import { ChooseSoundUi } from "./ChooseSoundUi.js";

export class SfgEditor {
  static getDependencies() {
    return [HTMLElement, Dom, Bus, PlaybackService, Resmgr];
  }
  constructor(element, dom, bus, playbackService, resmgr) {
    this.element = element;
    this.dom = dom;
    this.bus = bus;
    this.playbackService = playbackService;
    this.resmgr = resmgr;
    
    this.serial = [];
    this.path = "";
    this.soundId = "";
    this.inputUi = null;
    this.outputUi = null;
    this.chooseSoundUi = null;
    this.busListener = this.bus.listen(["editSound", "closeSound", "soundDirty", "setMaster"], e => this.onBusEvent(e));
    this.playbackService.requireContext();
    
    this.buildUi();
  }
  
  onRemoveFromDom() {
    this.bus.unlisten(this.busListener);
    this.playbackService.shutdown();
  }
  
  setup(serial, path) {
    this.serial = serial;
    this.path = path;
    this.chooseSoundUi.setup(serial, path);
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.inputUi = this.dom.spawnController(this.element, InputUi);
    this.outputUi = this.dom.spawnController(this.element, OutputUi);
    this.chooseSoundUi = this.dom.spawnController(this.element, ChooseSoundUi);
  }
  
  onBusEvent(event) {
    switch (event.type) {
      case "editSound": this.onEditSound(event.id, event.text); break;
      case "closeSound": this.onCloseSound(); break;
      case "soundDirty": this.onDirty(); break;
      case "setMaster": this.onDirty(); break;
    }
  }
  
  onEditSound(id, text) {
    this.soundId = id;
    this.bus.replaceText(text, this);
    this.chooseSoundUi.element.classList.add("hidden");
  }
  
  onCloseSound() {
    this.soundId = "";
    this.chooseSoundUi.element.classList.remove("hidden");
  }
  
  onDirty() {
    if (!this.chooseSoundUi) return;
    if (!this.chooseSoundUi.sfgMultiFile) return;
    if (!this.path) return;
    if (!this.soundId) return;
    const sound = this.chooseSoundUi.sfgMultiFile.sounds.find(s => s.id === this.soundId);
    if (!sound) return;
    sound.text = this.bus.requireText();
    this.resmgr.dirty(this.path, () => this.chooseSoundUi.sfgMultiFile.encode());
  }
}
