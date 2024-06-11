/* TextEditor.js
 */
 
import { Dom } from "./Dom.js";
import { Resmgr } from "./Resmgr.js";

export class TextEditor {
  static getDependencies() {
    return [HTMLElement, Dom, Resmgr, Window];
  }
  constructor(element, dom, resmgr, window) {
    this.element = element;
    this.dom = dom;
    this.resmgr = resmgr;
    this.window = window;
    
    this.path = "";
    this.serial = [];
    
    this.buildUi();
  }
  
  setup(serial, path) {
    this.serial = serial;
    this.path = path;
    this.populateUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.dom.spawn(this.element, "TEXTAREA", { "on-input": () => this.onInput() });
  }
  
  populateUi() {
    const textarea = this.element.querySelector("textarea");
    textarea.value = new this.window.TextDecoder("utf8").decode(this.serial);
  }
  
  onInput() {
    this.resmgr.dirty(this.path, () => new this.window.TextEncoder("utf8").encode(this.element.querySelector("textarea").value));
  }
}
