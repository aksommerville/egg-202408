/* SidebarUi.js
 * Left side of the screen at all times.
 */
 
import { Dom } from "./Dom.js";
import { Bus } from "./Bus.js";
import { Comm } from "./Comm.js";
import { Launcher } from "./Launcher.js";

export class SidebarUi {
  static getDependencies() {
    return [HTMLElement, Dom, Bus, Comm, Launcher];
  }
  constructor(element, dom, bus, comm, launcher) {
    this.element = element;
    this.dom = dom;
    this.bus = bus;
    this.comm = comm;
    this.launcher = launcher;
    this.statusListener = this.bus.listen(["status"], e => this.onStatusChanged(e));
    this.tocListener = this.bus.listen(["toc"], e => this.onTocChanged(e));
    this.romsListener = this.bus.listen(["roms"], e => this.onRomsChanged(e));
    this.idNext = 1;
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
    element.innerHTML = "";
    if (event.toc?.files) {
      for (const file of event.toc.files) this.addTocFile(element, file, "");
    }
  }
  
  addTocFile(ul, file, pfx) {
    if (!file || !file.name) return;
    if (file.serial) { // Regular file, ie leaf node.
      const li = this.dom.spawn(ul, "LI", ["file"], file.name);
      li.addEventListener("click", () => this.chooseFile(file, pfx, li));
    } else if (file.files) { // Directory.
      const id = `dir-${this.idNext++}`;
      const li = this.dom.spawn(ul, "LI", ["dir"], { id }); // <-- "open" here to start opened. Not sure which is better.
      this.dom.spawn(li, "DIV", ["dirlabel"], file.name, { "on-click": () => this.toggleDir(id) });
      const subul = this.dom.spawn(li, "UL", ["dir"]);
      for (const child of file.files) this.addTocFile(subul, child, pfx + file.name + "/");
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
  
  chooseFile(file, pfx, li) {
    for (const elem of this.element.querySelectorAll(".file.open")) elem.classList.remove("open");
    li.classList.add("open");
    this.bus.broadcast({ type: "open", path: pfx + file.name, serial: file.serial });
  }
  
  onRomsChanged(event) {
    const button = this.element.querySelector("input[type='button'][value='Play']");
    if (event.roms.length) {
      button.disabled = false;
    } else {
      button.disabled = true;
    }
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
