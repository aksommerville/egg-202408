/* ServerAudioModal.js
 */
 
import { Dom } from "./Dom.js";

export class ServerAudioModal {
  static getDependencies() {
    return [HTMLDialogElement, Dom];
  }
  constructor(element, dom) {
    this.element = element;
    this.dom = dom;
    
    this.buildUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.dom.spawn(this.element, "DIV", ["instructions"], "driver 'none' to turn off, or empty for the default.");
    const form = this.dom.spawn(this.element, "FORM", { "on-submit": e => e.preventDefault() });
    const table = this.dom.spawn(form, "TABLE");
    this.spawnInput(table, "text", "driver");
    this.spawnInput(table, "text", "device");
    this.spawnInput(table, "number", "rate");
    this.spawnInput(table, "number", "chanc");
    this.spawnInput(table, "number", "buffer");
    this.dom.spawn(form, "INPUT", { type: "submit", value: "OK", "on-click": e => {
      e.preventDefault();
      e.stopPropagation();
      this.resolve(this.readValueFromUi());
    }});
  }
  
  spawnInput(table, type, key) {
    const tr = this.dom.spawn(table, "TR");
    this.dom.spawn(tr, "TD", ["key"], key);
    const tdv = this.dom.spawn(tr, "TD", ["value"]);
    this.dom.spawn(tdv, "INPUT", { type, name: key });
  }
  
  readValueFromUi() {
    const result = {};
    for (const input of this.element.querySelectorAll("input")) {
      const key = input.name;
      const value = input.value;
      if (!key || !value) continue;
      result[key] = value;
    }
    return result;
  }
}
