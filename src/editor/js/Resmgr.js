/* Resmgr.js
 * Maintains the list of resources.
 */
 
import { Comm } from "./Comm.js";
import { Bus } from "./Bus.js";
import { HexEditor } from "./HexEditor.js";
import { TextEditor } from "./TextEditor.js";

export class Resmgr {
  static getDependencies() {
    return [Comm, Bus, Window];
  }
  constructor(comm, bus, window) {
    this.comm = comm;
    this.bus = bus;
    this.window = window;
    
    this.DIRTY_TIMEOUT_MS = 2000;
    this.dirties = []; // {path,encode}
    this.dirtyTimeout = null;
  }
  
  /* Get the full resource list and sanitize it.
   * This is a tree of entries {name, files} or {name, serial}.
   * serial are Uint8Array.
   */
  fetchAll() {
    return this.comm.httpJson("GET", "/res-all").then(rsp => this.sanitizeToc(rsp));
  }
  
  /* Editors should call this immediately when something changes.
   * It operates on a debounce, should be safe to spam it.
   * (path) is the full resource path as provided by RootUi.
   * (encode) is a no-arg function that returns the encoded resource as Uint8Array, just like you received it.
   */
  dirty(path, encode) {
    this.bus.setStatus("pending");
    const rec = this.dirties.find(d => d.path === path);
    if (rec) rec.encode = encode;
    else this.dirties.push({ path, encode });
    if (this.dirtyTimeout) {
      this.window.clearTimeout(this.dirtyTimeout);
    }
    this.dirtyTimeout = this.window.setTimeout(() => {
      this.dirtyTimeout = null;
      this.flushDirties().then(() => {
        this.bus.setStatus("ok");
      });
    }, this.DIRTY_TIMEOUT_MS);
  }
  
  flushDirties() {
    const old = this.dirties;
    this.dirties = [];
    const promises = [];
    for (const { path, encode } of old) {
      try {
        const serial = encode();
        this.replaceSerialInToc(path, serial);
        promises.push(this.comm.httpText("PUT", "/res/" + path, null, null, serial));
      } catch (error) {
        this.bus.broadcast({ type: "error", error });
      }
    }
    return Promise.all(promises).catch(error => this.bus.broadcast({ type: "error", error }));
  }
  
  replaceSerialInToc(path, serial) {
    const names = path.split("/");
    let node = this.bus.toc;
    while (names.length) {
      if (!node || !node.files) return;
      const name = names[0];
      names.splice(0, 1);
      if (!(node = node.files.find(f => f.name === name))) return;
    }
    node.serial = serial;
  }
  
  sanitizeToc(input) {
    if (!input) return { name: "", files: [] };
    if (!input.files) return { name: "", files: [] };
    return {
      name: "",
      files: input.files.map(f => this._sanitizeToc(f)).filter(v => v).sort((a, b) => this.tocCmp(a, b)),
    };
  }
  
  _sanitizeToc(file) {
    if (!file) return null;
    const name = (file.name || "").toString();
    if (file.files) return {
      name,
      files: file.files.map(f => this._sanitizeToc(f)).filter(v => v).sort((a, b) => this.tocCmp(a, b)),
    };
    if (file.serial) return {
      name,
      serial: this.decodeBase64(file.serial),
    };
    return {
      name,
      serial: "",
    };
  }
  
  tocCmp(a, b) {
    // Regular files before directories.
    if (!a.files && b.files) return -1;
    if (a.files && !b.files) return 1;
    // If both basenames start with a nonzero decimal integer, sort ascending. Just one, put numbers first. Neither, sort by string order.
    const an = +a.name.match(/^(\d+)/)?.[1] || 0;
    const bn = +b.name.match(/^(\d+)/)?.[1] || 0;
    if (an && bn) {
      if (an < bn) return -1;
      if (an > bn) return 1;
    }
    if (an) return -1;
    if (bn) return 1;
    if (a.name < b.name) return -1;
    if (a.name > b.name) return 1;
    return 0;
  }

  decodeBase64(src) {
    let dst = new Uint8Array(65536);
    let dstc = 0;
    const intake = []; // (0..63) up to 4
    for (let i=0; i<src.length; i++) {
      const srcch = src.charCodeAt(i);
      if (srcch <= 0x20) continue;
      if (srcch === 0x3d) continue; // =
      if ((srcch >= 0x41) && (srcch <= 0x5a)) intake.push(srcch - 0x41);
      else if ((srcch >= 0x61) && (srcch <= 0x7a)) intake.push(srcch - 0x61 + 26);
      else if ((srcch >= 0x30) && (srcch <= 0x39)) intake.push(srcch - 0x30 + 52);
      else if (srcch === 0x2b) intake.push(62); // +
      else if (srcch === 0x2f) intake.push(63); // /
      else throw new Error(`Unexpected byte ${srcch} in base64 rom`);
      if (intake.length >= 4) {
        if (dstc >= dst.length - 3) {
          const ndst = new Uint8Array(dst.length << 1);
          ndst.set(dst);
          dst = ndst;
        }
        dst[dstc++] = (intake[0] << 2) | (intake[1] >> 4);
        dst[dstc++] = (intake[1] << 4) | (intake[2] >> 2);
        dst[dstc++] = (intake[2] << 6) | intake[3];
        intake.splice(0, 4);
      }
    }
    if (intake.length > 0) {
      if (dstc >= dst.length - 2) {
        const ndst = new Uint8Array(dst.length + 2);
        ndst.set(dst);
        dst = ndst;
      }
      switch (intake.length) {
        case 1: dst[dstc++] = intake[0] << 2; break;
        case 2: dst[dstc++] = (intake[0] << 2) | (intake[1] >> 4); break;
        case 3: dst[dstc++] = (intake[0] << 2) | (intake[1] >> 4); dst[dstc++] = (intake[1] << 4) | (intake[2] >> 2); break;
      }
    }
    if (dstc < dst.length) {
      const ndst = new Uint8Array(dstc);
      const srcview = new Uint8Array(dst.buffer, 0, dstc);
      ndst.set(srcview);
      dst = ndst;
    }
    return dst;
  }
  
  editorClassForResource(path, serial) {
    console.log(`Resmgr.editorClassForResource path=${JSON.stringify(path)} len=${serial.length}`);

    //TODO User-supplied custom classes. How is that going to work?
    //TODO metadata. TextEditor is OK, but it would be much nicer with validation and suggestions.
    //TODO sfg. Very complicated and very necessary. TextEditor is hard to work with, and there needs to be instant feedback too.
    //TODO midi. Or maybe don't.
    //TODO png read-only. Easy.
    //TODO string. Need a side-by-side view of two languages.
     
    // If the whole thing is UTF-8 and not empty, use the text editor.
    if (serial.length) {
      let extc = 0;
      let ok = true;
      for (let srcp=0; srcp<serial.length; srcp++) {
        if (extc) {
          extc--;
          if ((serial[srcp] & 0xc0) !== 0x80) { ok = false; break; }
        } else if (!(serial[srcp] & 0x80)) {
          // Forbid C0 bytes except HT, LF, CR.
          if (serial[srcp] === 0x09) ;
          else if (serial[srcp] === 0x0a) ;
          else if (serial[srcp] === 0x0d) ;
          else if (serial[srcp] < 0x20) { ok = false; break; }
        } else if (!(serial[srcp] & 0x40)) { ok = false; break; }
        else if (!(serial[srcp] & 0x20)) extc = 1;
        else if (!(serial[srcp] & 0x10)) extc = 2;
        else if (!(serial[srcp] & 0x08)) extc = 3;
        else { ok = false; break; }
      }
      if (extc) ok = false;
      if (ok) return TextEditor;
    }
    
    // Finally, the hex editor can open anything at all:
    return HexEditor;
  }
}

Resmgr.singleton = true;
