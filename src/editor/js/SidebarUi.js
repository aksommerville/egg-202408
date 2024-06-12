/* SidebarUi.js
 * Left side of the screen at all times.
 */
 
import { Dom } from "./Dom.js";
import { Bus } from "./Bus.js";
import { Comm } from "./Comm.js";
import { Launcher } from "./Launcher.js";
import { Resmgr } from "./Resmgr.js";
import { customGlobalActions } from "./customGlobalActions.js";

export class SidebarUi {
  static getDependencies() {
    return [HTMLElement, Dom, Bus, Comm, Launcher, Resmgr];
  }
  constructor(element, dom, bus, comm, launcher, resmgr) {
    this.element = element;
    this.dom = dom;
    this.bus = bus;
    this.comm = comm;
    this.launcher = launcher;
    this.resmgr = resmgr;
    this.statusListener = this.bus.listen(["status"], e => this.onStatusChanged(e));
    this.tocListener = this.bus.listen(["toc"], e => this.onTocChanged(e));
    this.romsListener = this.bus.listen(["roms"], e => this.onRomsChanged(e));
    this.idNext = 1;
    this.currentPath = "";
    this.actions = {}; // value:action
    this.buildUi();
    this.onStatusChanged({ status: this.bus.status });
    this.onTocChanged({ toc: this.bus.toc });
    this.onRomsChanged({ roms: this.bus.roms });
  }
  
  onRemoveFromDom() {
    this.bus.unlisten(this.statusListener);
    this.statusListener = null;
    this.bus.unlisten(this.tocListener);
    this.tocListener = null;
    this.bus.unlisten(this.romsListener);
    this.romsListener = null;
  }
  
  buildUi() {
    this.element.innerHTML = "";
    
    const controls = this.dom.spawn(this.element, "DIV", ["controls"]);
    this.dom.spawn(controls, "DIV", ["status"]);
    this.dom.spawn(controls, "INPUT", { type: "button", value: "Play", "on-click": () => this.onPlay() });
    this.dom.spawn(controls, "INPUT", { type: "button", value: "New", "on-click": () => this.onNew() });
    
    const ops = this.dom.spawn(controls, "SELECT", { "on-change": e => {
      const action = this.actions[e.target?.value];
      if (action) {
        try {
          action();
        } catch (error) {
          this.bus.broadcast({ type: "error", error });
        }
      }
      ops.value = "";
    }});
    this.dom.spawn(ops, "OPTION", { value: "" }, "Ops...");
    // Opportunity to define standard actions here.
    this.actions = {
    };
    for (const { value, name, action } of customGlobalActions) {
      this.actions[value] = action;
      this.dom.spawn(ops, "OPTION", { value }, name);
    }
    
    const toc = this.dom.spawn(this.element, "UL", ["toc", "dir"]);
  }
  
  onStatusChanged(event) {
    const validStatuses = ["ok", "error", "pending", "dead"];
    if (validStatuses.includes(event.status)) {
      const element = this.element.querySelector(".status");
      for (const status of validStatuses) element.classList.remove(status);
      element.classList.add(event.status);
    }
  }
  
  onTocChanged(event) {
    const element = this.element.querySelector(".toc");
    const openDirectories = Array.from(element.querySelectorAll("li.dir.open")).map(e => e.getAttribute("data-path"));
    element.innerHTML = "";
    if (event.toc?.files) {
      for (const file of event.toc.files) this.addTocFile(element, file, "", openDirectories);
    }
  }
  
  addTocFile(ul, file, pfx, openDirectories) {
    if (!file || !file.name) return;
    const path = pfx + file.name;
    if (file.hasOwnProperty("serial")) { // Regular file, ie leaf node.
      const li = this.dom.spawn(ul, "LI", ["file"], { "data-path": path }, file.name);
      li.addEventListener("click", (e) => this.chooseFile(e, file, pfx, li));
      if (this.currentPath === path) li.classList.add("open");
    } else if (file.files) { // Directory.
      const id = `dir-${this.idNext++}`;
      const li = this.dom.spawn(ul, "LI", ["dir"], { id, "data-path": path });
      if (openDirectories.includes(path)) li.classList.add("open");
      this.dom.spawn(li, "DIV", ["dirlabel"], file.name, { "on-click": () => this.toggleDir(id) });
      const subul = this.dom.spawn(li, "UL", ["dir"]);
      for (const child of file.files) this.addTocFile(subul, child, path + "/", openDirectories);
    }
  }
  
  toggleDir(id) {
    const li = this.element.querySelector("#" + id);
    if (!li) return;
    if (li.classList.contains("open")) {
      li.classList.remove("open");
    } else {
      li.classList.add("open");
    }
  }
  
  chooseFile(event, file, pfx, li) {
    if (event.ctrlKey) {
      this.dom.modalPickOne(["Cancel, keep it", "Yes, delete it"], `Really delete file '${pfx}${file.name}'?`)
        .catch(() => {})
        .then(rsp => {
          if (!rsp || !rsp.startsWith("Yes")) return;
          const path = pfx + file.name;
          this.resmgr.deleteFile(path);
          this.onTocChanged({ toc: this.bus.toc });
          if (path === this.currentPath) {
            this.currentPath = "";
            this.bus.broadcast({ type: "open", path: "", serial: [] });
          }
        }).catch(error => this.bus.broadcast({ type: "error", error }));
    } else {
      for (const elem of this.element.querySelectorAll(".file.open")) elem.classList.remove("open");
      li.classList.add("open");
      this.currentPath = pfx + file.name;
      this.bus.broadcast({ type: "open", path: pfx + file.name, serial: file.serial });
    }
  }
  
  // Opens directories if needed, and simulates clicking on the element associated with this path.
  chooseFileByPath(path) {
    const li = this.element.querySelector(`li.file[data-path='${path}']`);
    if (!li) return;
    for (let parent=li.parentNode; parent; parent=parent.parentNode) {
      if ((parent.tagName === "LI") && parent.classList.contains("dir")) {
        parent.classList.add("open");
      }
    }
    li.click();
  }
  
  onRomsChanged(event) {
    const button = this.element.querySelector("input[type='button'][value='Play']");
    if (event.roms.length) {
      button.disabled = false;
    } else {
      button.disabled = true;
    }
  }
  
  onNew() {
    this.dom.modalInput("File name, relative to inside data directory:", "").then(subpath => {
      if (!subpath) return;
      this.resmgr.createFile(subpath);
      this.onTocChanged({ toc: this.bus.toc });
      this.chooseFileByPath(subpath);
    }).catch(error => {
      if (error) this.bus.broadcast({ type: "error", error });
    });
  }
  
  onPlay() {
    const roms = this.bus.roms;
    if (roms.length < 1) return;
    if (roms.length > 1) {
      this.dom.modalPickOne(roms).then(rom => this.launchRom(rom)).catch(() => {});
    } else {
      this.launchRom(roms[0]);
    }
  }
  
  launchRom(name) {
    this.comm.httpBinary("GET", name).then(rsp => {
      this.launcher.launchSerial(rsp);
    }).catch(error => {
      this.dom.modalError(error);
    });
  }
}
