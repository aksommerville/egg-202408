/* RootUi.js
 * Top of the view hierarchy, lives forever.
 * We'll be in charge of high-level coordination.
 * Actual UI stuff, keep it to a minimum. Defer to SidebarUi whenever possible.
 */
 
import { Dom } from "./Dom.js";
import { SidebarUi } from "./SidebarUi.js";
import { Bus } from "./Bus.js";
import { Resmgr } from "./Resmgr.js";

export class RootUi {
  static getDependencies() {
    return [HTMLElement, Dom, Bus, Resmgr];
  }
  constructor(element, dom, bus, resmgr) {
    this.element = element;
    this.dom = dom;
    this.bus = bus;
    this.resmgr = resmgr;
    
    this.sidebarUi = null;
    this.editor = null;
    
    this.buildUi();
    
    this.bus.listen(["open"], e => this.onOpen(e));
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.sidebarUi = this.dom.spawnController(this.element, SidebarUi);
    this.dom.spawn(this.element, "DIV", ["workspace"]);
  }
  
  onOpen(event) {
    const path = event.path;
    const serial = event.serial;
    const clazz = this.resmgr.editorClassForResource(path, serial);
    if (!clazz) {
      this.dom.modalError(`Unable to determine editor class for resource ${JSON.stringify(path)}`);
      return;
    }
    if (this.editor) {
      //TODO Cleanup for outgoing editor?
    }
    const workspace = this.element.querySelector(".workspace");
    workspace.innerHTML = "";
    this.editor = this.dom.spawnController(workspace, clazz);
    this.editor.setup(serial, path);
  }
}
