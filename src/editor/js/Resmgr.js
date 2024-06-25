/* Resmgr.js
 * Maintains the list of resources.
 * But it's actually owned by Bus: Bus.toc: {path,type,qual,rid,name,format,serial}[]
 */
 
import { Comm } from "./Comm.js";
import { Bus } from "./Bus.js";
import { HexEditor } from "./HexEditor.js";
import { TextEditor } from "./TextEditor.js";
import { StringEditor } from "./StringEditor.js";
import { ImageEditor } from "./ImageEditor.js";
import { MetadataEditor } from "./MetadataEditor.js";
import { SfgEditor } from "./audio/SfgEditor.js";
import { SongEditor } from "./song/SongEditor.js";
import { selectCustomEditor, listCustomEditors } from "./selectCustomEditor.js";

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
    this.metadata = null; // {k:v} strings, decoded as needed
    this.images = []; // Sparse, indexed by rid. DOM Image objects. One can assume they are constant.
  }
  
  /* Get the full resource list and sanitize it.
   * Returns the format expected by Bus.toc, but does not install there.
   */
  fetchAll() {
    return this.comm.httpJson("GET", "/res-all")
      .then(rsp => this.sanitizeToc(rsp))
      .then(toc => this.loadAllImages(toc));
  }
  
  /* Editors should call this immediately when something changes.
   * It operates on a debounce, should be safe to spam it.
   * (path) is the full resource path as provided by RootUi.
   * (encode) is a no-arg function that returns the encoded resource as Uint8Array, just like you received it.
   */
  dirty(path, encode) {
    if (!path || !encode) throw new Error("Resmgr.dirty with null path or encode");
    this.bus.setStatus("pending");
    if (path.endsWith("metadata")) this.metadata = null;
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
  
  resByPath(path) {
    return this.bus.toc.find(r => r.path === path);
  }
  
  // Name, ID, basename, path. If it starts with "TYPE:", we assert that as type. Otherwise the one you provide, if you do.
  resByString(q, type) {
    if (!q || (typeof(q) !== "string")) return null;
    const match = q.match(/^([a-zA-Z0-9_]+):/);
    if (match) {
      type = match[1];
      q = q.substring(type.length + 1);
    }
    const rid = +q;
    if (type && rid) { // If the string is a nonzero integer, and a type was provided, it can only be rid.
      return this.bus.toc.find(res => (res.rid === rid) && (res.type === type));
    }
    for (const res of this.bus.toc) {
      if (type && (type !== res.type)) continue;
      if (q === res.name) return res;
      if (q === res.path) return res;
      if (res.path.endsWith(q) && (res.path[res.path.length - q.length - 1] === "/")) return res;
    }
    return null;
  }
  
  resById(type, rid) {
    return this.bus.toc.find(r => ((r.type === type) && (r.rid === rid)));
  }
  
  getImage(name) {
    const res = this.resByString(name, "image");
    if (!res) return null;
    return res.image;
  }
  
  getMetadata(key) {
    this.requireMetadata();
    return this.metadata[key] || "";
  }
  
  requireMetadata() {
    if (this.metadata) return;
    const res = this.bus.toc.find(r => r.path === "metadata");
    const serial = res?.serial || new Uint8Array(0);
    const src = new this.window.TextDecoder("utf8").decode(serial);
    this.metadata = {};
    for (let srcp=0; srcp<src.length; ) {
      let nlp = src.indexOf("\n", srcp);
      if (nlp < 0) nlp = src.length;
      const line = src.substring(srcp, nlp);
      srcp = nlp + 1;
      let eqp = line.indexOf('='); if (eqp < 0) eqp = line.length;
      let clp = line.indexOf(':'); if (clp < 0) clp = line.length;
      const sepp = (eqp < clp) ? eqp : clp;
      const k = line.substring(0, sepp).trim();
      const v = line.substring(sepp + 1).trim();
      this.metadata[k] = v;
    }
  }
  
  flushDirties() {
    const old = this.dirties;
    this.dirties = [];
    const promises = [];
    for (const { path, encode } of old) {
      try {
        const serial = encode();
        const res = this.bus.toc.find(r => r.path === path);
        if (res) res.serial = serial;
        promises.push(this.comm.httpText("PUT", "/res/" + path, null, null, serial));
      } catch (error) {
        this.bus.broadcast({ type: "error", error });
      }
    }
    return Promise.all(promises).catch(error => this.bus.broadcast({ type: "error", error }));
  }
  
  sanitizeToc(input) {
    const dst = [];
    if (input?.name) input = {...input, name: ""}; // Eliminate the root node name (typically "data").
    this.sanitizeToc1(dst, "", input);
    return dst;
  }
  
  sanitizeToc1(dst, pfx, file) { 
    if (!file) return;
    if (file.files) {
      if (file.name) pfx += file.name + "/";
      const files = [...file.files];
      files.sort((a, b) => this.tocCmp(a, b));
      for (const child of files) {
        this.sanitizeToc1(dst, pfx, child);
      }
    } else {
      const path = pfx + file.name;
      const { type, qual, rid, name, format } = this.parsePath(path);
      const serial = ((typeof(file.serial) === "string") ? this.decodeBase64(file.serial) : file.serial) || new Uint8Array(0);
      dst.push({ path, type, qual, rid, name, format, serial });
    }
  }
  
  parsePath(path) {
    /* Be careful to apply the same rules as src/eggdev/eggdev_pack.c:eggdev_pack_analyze_path():
     *   .../metadata
     *   .../string/QUAL[-NAME]
     *   .../TYPE/ID
     *   .../TYPE/ID-NAME
     *   .../TYPE/ID-QUAL-NAME
     *   .../TYPE/NAME
     *   .../TYPE/QUAL-NAME
     * And any of those generic ones may be followed by [.COMMENT][.FORMAT].
     */
    const components = path.split("/");
    let base = components[components.length - 1];
    
    // metadata is special.
    if (base === "metadata") return { type: "metadata", qual: "", rid: 0, name: "metadata", format: "" };
    
    // Everything else has TYPE at the beginning and .COMMENT.FORMAT at the end.
    if (components.length < 2) throw new Error(`${path}: Resource paths must have at least two components.`);
    const type = components[components.length - 2];
    const dotbits = base.split(".");
    base = dotbits[0];
    const format = (dotbits.length > 1) ? dotbits[dotbits.length - 1].toLowerCase() : "";
    
    // string is a little bit special; it requires QUAL.
    if (type === "string") {
      const split = base.split("-");
      if (split.length > 2) throw new Error(`${path}: String basenames must be "QUAL" or "QUAL-NAME" with no dashes in NAME.`);
      const qual = split[0];
      const name = split[1] || "";
      return { type, qual, rid: 0, name, format };
    }
    
    // Everything else is generic.
    const dashbits = base.split("-");
    if (dashbits.length > 3) {
      throw new Error(`${path}: Too many dashes in basename. Expected "ID", "ID-NAME", or "ID-QUAL-NAME".`);
    } else if (dashbits.length === 3) {
      const rid = +dashbits[0];
      if (isNaN(rid) || (rid < 0) || (rid > 0xffff)) throw new Error(`${path}: Invalid ID`);
      const qual = dashbits[1];
      const name = dashbits[2];
      return { type, qual, rid, name, format };
    } else if (dashbits.length === 2) { 
      // "ID-NAME" or "QUAL-NAME", ID wins ties.
      let rid = +dashbits[0];
      let qual = "";
      if (isNaN(rid) || (rid < 0) || (rid > 0xffff)) {
        rid = 0;
        qual = dashbits[0];
      }
      const name = dashbits[1];
      return { type, qual, rid, name, format };
    } else {
      // "ID" or "NAME"
      let rid = +dashbits[0];
      let name = "";
      if (isNaN(rid) || (rid < 0) || (rid > 0xffff)) {
        rid = 0;
        name = dashbits[0];
      }
      return { type, qual: "", rid, name, format };
    }
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
  
  /* This happens during fetchAll().
   * Given a finalish TOC, locate all of the image resources, create an Image object for each, and wait for those to load.
   * We modify (toc) in place and resolve with the same.
   */
  loadAllImages(toc) {
    const promises = [];
    for (const res of toc) {
      if (res.type !== "image") continue;
      if (!res.serial.length) continue;
      promises.push(this.loadImage(res.serial).then(img => res.image = img));
    }
    return Promise.all(promises).then(() => toc);
  }
  
  loadImage(serial) {
    const image = new Image();
    const blob = new Blob([serial]);
    const url = URL.createObjectURL(blob);
    return new Promise((resolve, reject) => {
      image.addEventListener("load", () => {
        URL.revokeObjectURL(url);
        resolve(image);
      }, { once: true });
      image.addEventListener("error", error => {
        URL.revokeObjectURL(url);
        reject(error);
      }, { once: true });
      image.src = url;
    });
  }
  
  createFile(path) {
    // Insert in TOC just before the longest prefix match.
    let insp = this.bus.toc.length;
    let matchlen = 0;
    const pfxcnt = (a, b) => {
      let c = 0;
      for (; a[c] === b[c]; c++) ;
      return c;
    };
    for (let i=0; i<this.bus.toc.length; i++) {
      const res = this.bus.toc[i];
      const q = pfxcnt(res.path, path);
      if (q > matchlen) {
        matchlen = q;
        insp = i;
      }
    }
    const file = this.parsePath(path);
    file.path = path;
    file.serial = new Uint8Array(0);
    this.bus.toc.splice(insp, 0, file);
    this.bus.setToc(this.bus.toc);
    this.dirty(path, () => file.serial);
    return file;
  }
  
  deleteFile(path) {
    const p = this.bus.toc.findIndex(r => r.path === path);
    if (p < 0) return this.bus.broadcast({ type: "error", error: `File ${JSON.stringify(path)} not found` });
    this.bus.toc.splice(p, 1);
    this.bus.setToc(this.bus.toc);
    this.comm.httpBinary("DELETE", "/res/" + path).then(() => {
    }).catch(error => this.bus.broadcast({ type: "error", error }));
  }
  
  editorClassesForResource(res) {
    // We could eliminate editor types that we know will be incompatible, but I'm not sure that's really our place.
    return [
      HexEditor,
      TextEditor,
      SfgEditor,
      SongEditor,
      MetadataEditor,
      ImageEditor,
      StringEditor,
      ...listCustomEditors(res.path, res.serial, res.type, res.qual, res.rid, res.name, res.format),
    ];
  }
  
  editorClassForResource(res) {
    
    // First, let the client override any editor selection.
    let clazz = selectCustomEditor(res.path, res.serial, res.type, res.qual, res.rid, res.name, res.format);
    if (clazz) return clazz;

    // Sound effects in our text format use SfgEditor.
    // Check for WAV and SFG-binary signatures; let those fall thru to HexEditor.
    if (res.type === "sound") {
      const s = res.serial;
      if ((s.length >= 4) && (s[0] === 0x52) && (s[1] === 0x49) && (s[2] === 0x46) && (s[3] === 0x46)) ;
      else if ((s.length >= 2) && (s[0] === 0xeb) && (s[1] === 0xeb)) ;
      else return SfgEditor;
    }
    
    // MIDI files use SongEditor.
    // Type should be "song", but MIDI has an unambiguous binary signature, so use that instead.
    if (res.serial.length >= 8) {
      const s = res.serial;
      if (
        (s[0] === 0x4d) && (s[1] === 0x54) && (s[2] === 0x68) && (s[3] === 0x64) &&
        (s[4] === 0x00) && (s[5] === 0x00) && (s[6] === 0x00) && (s[7] === 0x06)
      ) return SongEditor;
    } else if ((res.serial.length === 0) && (res.type === "song")) {
      return SongEditor;
    }
    
    // TextEditor would suffice for metadata, but we can do better.
    if (res.type === "metadata") return MetadataEditor;
    
    // We have a read-only viewer for image resources.
    if (res.type === "image") return ImageEditor;
    
    // Strings have a very special side-by-side deal to aid translation.
    if (res.type === "string") return StringEditor;
     
    // If the whole thing is UTF-8 and not empty, use the text editor.
    if (res.serial.length) {
      const s = res.serial;
      let extc = 0;
      let ok = true;
      for (let srcp=0; srcp<s.length; srcp++) {
        if (extc) {
          extc--;
          if ((s[srcp] & 0xc0) !== 0x80) { ok = false; break; }
        } else if (!(s[srcp] & 0x80)) {
          // Forbid C0 bytes except HT, LF, CR.
          if (s[srcp] === 0x09) ;
          else if (s[srcp] === 0x0a) ;
          else if (s[srcp] === 0x0d) ;
          else if (s[srcp] < 0x20) { ok = false; break; }
        } else if (!(s[srcp] & 0x40)) { ok = false; break; }
        else if (!(s[srcp] & 0x20)) extc = 1;
        else if (!(s[srcp] & 0x10)) extc = 2;
        else if (!(s[srcp] & 0x08)) extc = 3;
        else { ok = false; break; }
      }
      if (extc) ok = false;
      if (ok) return TextEditor;
    }
    
    // Finally, the hex editor can open anything at all.
    return HexEditor;
  }
}

Resmgr.singleton = true;
