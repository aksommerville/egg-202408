/* DataService.js
 * Coordinates higher-level services around resources,
 * and the persistent storage.
 */
 
import { Rom } from "./Rom.js";
 
export class DataService {
  constructor(egg) {
    this.egg = egg;
    this.rom = egg.rom;
    this.metadata = null; // {k:v} strings
    this.masterKey = "store-" + this.getMetadata("title").substring(0, 32);
    try {
      this.store = JSON.parse(window.localStorage.getItem(this.masterKey));
      if (!this.store || (typeof(this.store) !== "object") || (this.store instanceof Array)) this.store = {};
    } catch (e) {
      this.store = {};
    }
    this.storeSaveTimeout = null;
  }
  
  getMetadata(k) {
    if (!this.metadata) this.metadata = this.decodeMetadata(this.rom.getRes(Rom.RESTYPE_metadata, 0, 1));
    // TODO Look for "*String", if we know the language.
    return this.metadata[k] || "";
  }
  
  decodeMetadata(src) {
    const decoder = new TextDecoder("utf8");
    const dst = {};
    for (let srcp=0; srcp<src.length; ) {
      const kc = src[srcp++] || 0;
      const vc = src[srcp++] || 0;
      if (srcp > src.length - vc - kc) break;
      const k = decoder.decode(src.slice(srcp, srcp + kc));
      srcp += kc;
      const v = decoder.decode(src.slice(srcp, srcp + vc));
      srcp += vc;
      dst[k] = v;
    }
    return dst;
  }
  
  storeDirty() {
    /* This debounce shouldn't be long, in fact zero would probably be OK.
     * The idea is that if the game sets a whole bunch of fields in one update cycle, don't write them out more than once.
     * No matter what we do here, fields written by the game available to it for immediate readback.
     */
    const saveDebounceTimeMs = 500;
    if (!this.storeSaveTimeout) this.storeSaveTimeout = window.setTimeout(() => {
      this.storeSaveTimeout = null;
      window.localStorage.setItem(this.masterKey, JSON.stringify(this.store));
    }, saveDebounceTimeMs);
  }
  
  /*--------------------------- Public API entry points ---------------------------------*/
  
  egg_image_get_header(wp, hp, stridep, fmtp, qual, rid) {
    const serial = this.rom.getRes(Rom.RESTYPE_image, qual, rid);
    if (serial.length < 1) return;
    try {
      const header = this.egg.imageDecoder.decodeHeader(serial);
      this.egg.exec.mem32[wp >> 2] = header.w;
      this.egg.exec.mem32[hp >> 2] = header.h;
      this.egg.exec.mem32[stridep >> 2] = header.stride;
      this.egg.exec.mem32[fmtp >> 2] = header.fmt;
    } catch (e) {}
  }
  
  egg_image_decode(dst, dsta, qual, rid) {
    const serial = this.rom.getRes(Rom.RESTYPE_image, qual, rid);
    if (serial.length < 1) return -1;
    try {
      const image = this.egg.imageDecoder.decode(serial);
      return this.egg.exec.safeWrite(dst, dsta, image.v);
    } catch (e) {
      console.error(e);
      return -1;
    }
  }
  
  egg_store_get(dst, dsta, k, kc) {
    k = this.egg.exec.readLimitedString(k, kc);
    const v = this.store[k] || "";
    return this.egg.exec.safeWrite(dst, dsta, v);
  }
  
  egg_store_set(k, kc, v, vc) {
    k = this.egg.exec.readLimitedString(k, kc);
    v = this.egg.exec.readLimitedString(v, vc);
    if (!k) return -1;
    //TODO Access control.
    if (v) {
      if (this.store[k] === v) return 0;
      if (!this.store[k]) {
        //TODO Confirm length<256 when encoded UTF-8.
      }
      this.store[k] = v;
    } else {
      if (!this.store[k]) return 0;
      delete this.store[k];
    }
    this.storeDirty();
    return 0;
  }
  
  egg_store_key_by_index(dst, dsta, p) {
    const keys = Object.keys(this.store);
    return this.egg.exec.safeWrite(dst, dsta, keys[p] || "");
  }
}
