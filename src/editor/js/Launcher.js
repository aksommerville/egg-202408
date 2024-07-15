/* Launcher.js
 * Manages loading the real Egg Runtime and starting it up in a modal.
 */
 
import { Dom } from "./Dom.js";

export class Launcher {
  static getDependencies() {
    return [Dom];
  }
  constructor(dom) {
    this.dom = dom;
    this.running = false;
  }
  
  launchSerial(serial) {
    if (this.running) return;
    const controller = this.dom.modalIframe("/rt/index.html?delivery=message");
    const iframe = controller.element.querySelector("iframe");
    this.running = true;
    
    // Read framebuffer size from the ROM and let that guide the iframe's size.
    const meta = Launcher.readMetadata(serial);
    const fbsize = (meta.framebuffer || "").match(/^(\d+)x(\d+)/) || [320, 180];
    const fbw = +fbsize[1];
    const fbh = +fbsize[2];
    const bounds = document.body.getBoundingClientRect();
    // Assume we want 100px clearance on both axes, try not to approach the edges.
    const availw = bounds.width - 100;
    const availh = bounds.height - 100;
    const scale = Math.max(1, Math.floor(Math.min(availw / fbw, availh / fbh)));
    // Add a little to the iframe's size to account for padding and extra chrome (eg the temporaryish "Terminate" button).
    // I hope that before we ship 1.0, these constants can go zero.
    iframe.width = fbw * scale + 20;
    iframe.height = fbh * scale + 50;
    
    iframe.addEventListener("load", () => {
      try {
        iframe.focus();
        iframe.contentWindow.postMessage(serial);
      } catch (error) {
        console.log(`iframe.contentWindow.postMessage failed`, error);
      }
    });
    controller.result.catch(() => {}).then(() => {
      this.running = false;
    });
  }
  
  /* ROM files are carefully designed such that resource metadata:0:1 can be extracted without touching the TOC.
   * It must be the first resource (it has to be; they're sorted by ID), and it must be terminated by a double-zero.
   */
  static readMetadata(s) {
    if (s instanceof ArrayBuffer) s = new Uint8Array(s);
    if (!s || (s.length < 12)) throw new Error(`Invalid ROM`);
    const signature = (s[0] << 24) | (s[1] << 16) | (s[2] << 8) | s[3];
    const hdrlen = (s[4] << 24) | (s[5] << 16) | (s[6] << 8) | s[7];
    const toclen = (s[8] << 24) | (s[9] << 16) | (s[10] << 8) | s[11];
    // One could argue that we should assert the heap length too, but it shouldn't matter.
    if ((signature !== ~~0xea00ffff) || (hdrlen < 16) || (hdrlen + toclen >= s.length)) throw new Error(`Invalid ROM`);
    let sp = hdrlen + toclen; // Start of heap, which must also be the start of metadata:0:1.
    if (s[sp++] !== 0xee) throw new Error(`Invalid ROM`);
    if (s[sp++] !== 0x4d) throw new Error(`Invalid ROM`);
    const dst = {};
    const decoder = new TextDecoder("utf8");
    while (sp < s.length) {
      const kc = s[sp++];
      if (!kc) break;
      const vc = s[sp++];
      if (sp + kc + vc > s.length) throw new Error(`Invalid ROM`);
      const k = decoder.decode(s.slice(sp, sp + kc));
      sp += kc;
      const v = decoder.decode(s.slice(sp, sp + vc));
      sp += vc;
      dst[k] = v;
    }
    return dst;
  }
}

Launcher.singleton = true;
