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
    return [HTMLElement, Dom, Bus, Resmgr, Window];
  }
  constructor(element, dom, bus, resmgr, window) {
    this.element = element;
    this.dom = dom;
    this.bus = bus;
    this.resmgr = resmgr;
    this.window = window;
    
    this.sidebarUi = null;
    this.editor = null;
    
    this.buildUi();
    
    this.bus.listen(["open"], e => this.onOpen(e));
    
    this.hashListener = e => this.onHashChange(e.newURL?.split('#')[1] || "");
    this.window.addEventListener("hashchange", this.hashListener);
  }
  
  onRemoveFromDom() {
    this.window.removeEventListener("hashchange", this.hashListener);
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.sidebarUi = this.dom.spawnController(this.element, SidebarUi);
    this.dom.spawn(this.element, "DIV", ["workspace"]);
  }
  
  // Our bootstrap will call this after the TOC is loaded. It is not loaded when our ctor runs.
  openInitialResource() {
    const hash = this.window.location.hash?.substring(1) || "";
    this.onHashChange(hash);
  }
  
  onOpen(event) {
    this.window.location = "#" + event.path;
  }
  
  onHashChange(path) {
    const res = this.resmgr.tocEntryByPath(path);
    if (res && res.serial) {
      const clazz = this.resmgr.editorClassForResource(path, res.serial);
      if (clazz) {
        const workspace = this.element.querySelector(".workspace");
        workspace.innerHTML = "";
        this.editor = this.dom.spawnController(workspace, clazz);
        this.editor.setup(res.serial, path);
        this.sidebarUi.focusPath(path);
        return;
      }
    }
    const workspace = this.element.querySelector(".workspace");
    workspace.innerHTML = "";
    this.editor = null;
    this.sidebarUi.focusPath("");
  }
}
