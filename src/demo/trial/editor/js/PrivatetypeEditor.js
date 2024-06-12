/* PrivatetypeEditor.js
 * Testing game-specific editor overrides.
 * These resources would otherwise launch with the generic TextEditor.
 * A realistic editor would of course be more robust and more useful, but this should give you the basic idea.
 */
 
import { Dom } from "./Dom.js";
import { Resmgr } from "./Resmgr.js";

export class PrivatetypeEditor {
  static getDependencies() {
    return [HTMLElement, Dom, Resmgr];
  }
  constructor(element, dom, resmgr) {
    this.element = element;
    this.dom = dom;
    this.resmgr = resmgr;
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
    this.dom.spawn(this.element, "DIV", "This is an example of custom data editors living within the game project.");
    this.dom.spawn(this.element, "INPUT", { type: "text", "on-input": () => this.onInput() });
  }
  
  populateUi() {
    this.element.querySelector("input").value = new TextDecoder("utf8").decode(this.serial);
  }
  
  onInput() {
    this.resmgr.dirty(this.path, () => new TextEncoder("utf8").encode(this.element.querySelector("input").value));
  }
}
