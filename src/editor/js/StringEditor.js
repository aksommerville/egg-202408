/* StringEditor.js
 * Show two string files side by side.
 */
 
import { Dom } from "./Dom.js";
import { Resmgr } from "./Resmgr.js";

export class StringEditor {
  static getDependencies() {
    return [HTMLElement, Dom, Resmgr, Window];
  }
  constructor(element, dom, resmgr, window) {
    this.element = element;
    this.dom = dom;
    this.resmgr = resmgr;
    this.window = window;
    
    this.pathPrefix = "";
    this.files = [];
    this.names = [null, "(l)", "(r)"]; // indexed by column id, so 1 and 2.
    this.rows = []; // [id,left,right], strings are decoded if they were stored JSON.
    this.buildUi();
    this.element.addEventListener("input", () => this.onDirty());
  }
  
  setup(serial, path) {
    this.pathPrefix = path.replace(/[^\/]*$/, "");
    this.files = this.resmgr.bus.toc.filter(res => res.type === "string");
    if (this.files.length < 1) return;
    if (this.files.length === 1) {
      this.chooseFiles(this.files[0], null);
    } else {
      const main = this.files.find(f => path === f.path);
      if (!main) return;
      let ref;
      const lang = this.resmgr.getMetadata("language").trim().substring(0, 2) || "en";
      if (main.name !== lang) ref = this.files.find(f => f.qual === lang);
      if (!ref) ref = this.files.find(f => f !== main);
      this.chooseFiles(ref, main);
    }
  }
  
  chooseFiles(l, r) {
    this.element.querySelector("th.left").innerText = this.names[1] = l?.name || "";
    this.element.querySelector("th.right").innerText = this.names[2] = r?.name || "";
    for (const row of this.element.querySelectorAll("tr.res")) row.remove();
    this.rows = [];
    if (l) this.decodeFile(1, l.serial);
    if (r) this.decodeFile(2, r.serial);
    // It's tempting to sort (this.rows) but don't. There might be some value in preserving the original order.
    const table = this.element.querySelector("table");
    for (const [id, lstr, rstr] of this.rows) {
      const tr = this.dom.spawn(table, "TR", ["res"]);
      this.dom.spawn(tr, "TD", ["id"], this.dom.spawn(null, "INPUT", { type: "text", value: id }));
      this.dom.spawn(tr, "TD", ["left"], this.dom.spawn(null, "INPUT", { type: "text", value: lstr }));
      this.dom.spawn(tr, "TD", ["right"], this.dom.spawn(null, "INPUT", { type: "text", value: rstr }));
    }
  }
  
  decodeFile(col, serial) {
    const src = new this.window.TextDecoder("utf8").decode(serial);
    for (let srcp=0, lineno=1; srcp<src.length; lineno++) {
      let nlp = src.indexOf("\n", srcp);
      if (nlp < 0) nlp = src.length;
      const line = src.substring(srcp, nlp).trim();
      srcp = nlp + 1;
      if (!line) continue;
      if (line.startsWith("#")) continue;
      const match = line.match(/^([^\s]*)\s+(.*)$/);
      if (!match) continue;
      const id = match[1];
      const v = match[2].match(/^".*"$/) ? JSON.parse(match[2]) : match[2];
      const p = this.rows.findIndex(r => r[0] === id);
      if (p >= 0) this.rows[p][col] = v;
      else {
        const row = [id, "", ""];
        row[col] = v;
        this.rows.push(row);
      }
    }
  }
  
  buildUi() {
    this.element.innerHTML = "";
    const table = this.dom.spawn(this.element, "TABLE");
    const tr = this.dom.spawn(table, "TR");
    this.dom.spawn(tr, "TH", ["id"], "ID");
    this.dom.spawn(tr, "TH", ["left"], this.names[1], { "on-click": () => this.chooseLanguage(1) });
    this.dom.spawn(tr, "TH", ["right"], this.names[2], { "on-click": () => this.chooseLanguage(2) });
  }
  
  chooseLanguage(col) {
    this.dom.modalPickOne(this.files.map(f => f.name), `Currently '${this.names[col]}'`).then(lang => {
      this.names[col] = lang;
      const file = this.files.find(f => f.name === lang);
      if (col === 1) {
        const other = this.files.find(f => f.name === this.names[2]);
        this.chooseFiles(file, other);
      } else {
        const other = this.files.find(f => f.name === this.names[1]);
        this.chooseFiles(other, file);
      }
    }).catch(() => {});
  }
  
  onDirty() {
    const lfile = this.files.find(f => f.name === this.names[1]);
    const rfile = this.files.find(f => f.name === this.names[2]);
    if (lfile) this.resmgr.dirty(this.pathPrefix + lfile.name, () => this.encode("left"));
    if (rfile) this.resmgr.dirty(this.pathPrefix + rfile.name, () => this.encode("right"));
  }
  
  encode(colname) {
    let text = "";
    for (const row of this.element.querySelectorAll("tr.res")) {
      const id = row.querySelector("td.id input").value.trim();
      if (!id) continue;
      const v = row.querySelector("td." + colname + " input").value.trim();
      text += id + " ";
      if (v.startsWith('"') || (v.indexOf("\n") >= 0) || v.match(/^\s/)) {
        text += JSON.stringify(v);
      } else {
        text += v;
      }
      text += "\n";
    }
    return  new this.window.TextEncoder("utf8").encode(text);
  }
}
