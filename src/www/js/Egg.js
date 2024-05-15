/* Egg.js
 * Top level coordinator for the Egg Web Runtime.
 * You must provide a valid Rom at construction.
 */
 
import { DataService } from "./DataService.js";
import { Render } from "./Render.js";
import { Exec } from "./Exec.js";
import { SysExtra } from "./SysExtra.js";
import { Input } from "./Input.js";
import { Audio } from "./Audio.js";
import { ImageDecoder } from "./ImageDecoder.js";
import { Rom } from "./Rom.js";
 
export class Egg {
  constructor(rom) {
    this.rom = rom;
    this.data = new DataService(this);
    this.sysExtra = new SysExtra(window);
    this.input = new Input(this);
    this.audio = new Audio(this);
    this.imageDecoder = new ImageDecoder();
    this.canvas = null;
    this.render = null;
    this.exec = null;
    this.running = false;
    this.loaded = false;
    this.pvtime = 0;
    this.pendingFrame = null;
    this.restoreTitle = document.title;
  }
  
  attachToDom() {
    const body = document.body;
    body.innerHTML = "";
    
    // Optional "terminate" button. XXX This shouldn't be in the final product; whoever embeds us should provide this.
    const terminate = document.createElement("INPUT");
    terminate.setAttribute("type", "button");
    terminate.setAttribute("value", "Terminate");
    terminate.addEventListener("click", () => this.stop());
    terminate.style.display = "block";
    body.appendChild(terminate);
    
    this.canvas = document.createElement("CANVAS");
    this.setCanvasSize();
    body.appendChild(this.canvas);
    this.render = new Render(this);
    this.exec = new Exec(this);
    this.retitlePerRom();
  }
  
  setCanvasSize() {
    const match = this.data.getMetadata("framebuffer").match(/^(\d+)x(\d+)/);
    if (!match) throw new Error("ROM does not declare its framebuffer size.");
    this.canvas.width = +match[1];
    this.canvas.height = +match[2];
    this.input.canvasChanged();
  }
  
  retitlePerRom() {
    document.title = this.data.getMetadata("title");
    let url = "";
    const iconImageId = +this.data.getMetadata("iconImage");
    if (iconImageId) {
      const serial = this.rom.getRes(Rom.RESTYPE_image, 0, iconImageId);
      if (serial.length > 0) {
        const crop = new Uint8Array(serial.length);
        crop.set(serial);
        const blob = new Blob([crop.buffer], { type: "image/png" });
        url = URL.createObjectURL(blob);
      }
    }
    for (const link of window.document.querySelectorAll("link[rel='icon']")) link.remove();
    if (url) {
      const link = window.document.createElement("LINK");
      link.setAttribute("data-egg-favicon", "");
      link.setAttribute("rel", "icon");
      link.setAttribute("type", "image/png");
      link.setAttribute("href", url);
      window.document.head.appendChild(link);
    }
  }
  
  retitleDefault() {
    document.title = this.restoreTitle;
    for (const link of window.document.querySelectorAll("link[rel='icon']")) link.remove();
  }
  
  start() {
    if (this.running) return;
    this.running = true;
    this.loaded = false;
    this.pvtime = 0;
    this.audio.start();
    return this.exec.load().then(() => {
      this.loaded = true;
      if (this.exec.egg_client_init() < 0) {
        throw new Error("Game aborted during startup.");
      }
      this.exec.egg_client_init = () => -1;
      this.pendingFrame = window.requestAnimationFrame(() => this.update());
    });
  }
  
  stop() {
    if (!this.running) return;
    this.audio.stop();
    this.input.detach();
    this.running = false;
    if (this.loaded) {
      this.exec.egg_client_quit();
      this.exec.egg_client_quit = () => {};
      this.loaded = false;
    }
    if (this.pendingFrame) {
      window.cancelAnimationFrame(this.pendingFrame);
      this.pendingFrame = null;
    }
    if (this.render) this.render.stop();
    this.retitleDefault();
  }
  
  update() {
    this.pendingFrame = null;
    if (!this.running) return;
    this.audio.update();
    this.input.update();
    const elapsed = this.tick();
    if (elapsed >= 0) {
      this.exec.egg_client_update(elapsed);
      if (!this.running) return;
      this.render.begin();
      this.exec.egg_client_render();
      this.render.end();
    }
    this.pendingFrame = window.requestAnimationFrame(() => this.update());
  }
  
  tick() {
    if (!this.pvtime) { // Very first update only, we update with zero time elapsed.
      this.pvtime = Date.now();
      return 0;
    }
    const now = Date.now();
    let elapsed = (now - this.pvtime) / 1000;
    if (elapsed < 0.015) return -1; // skip a frame (eg high-frequency monitors, don't let us run at 200 Hz or whatever)
    this.pvtime = now;
    return Math.min(0.050, elapsed); // Very long elapsed time, clamp at 50 ms.
  }
  
  /*------------------------ Public API entry points ------------------------*/
  
  egg_log(fmt, vargs) {
    const msg = this.sysExtra.applyLogFormat(
      this.exec.readCString(fmt),
      (spec, prec) => { // udxplfsc
        switch (spec) {
          case 'u': case 'd': case 'x': case 'c': case 'p': {
              const v = this.exec.mem32[vargs >> 2] || 0;
              vargs += 4;
              return v;
            }
          case 'l': {
              const v = this.exec.mem32[vargs >> 2] | (this.exec.mem32[(vargs+4) >> 2] * 4294967296);
              vargs += 8;
              return v;
            }
          case 'f': {
              if (vargs & 7) vargs = (vargs + 8) & ~7;
              const v = this.exec.memf64[vargs >> 3];
              vargs += 8;
              return v;
            }
          case 's': {
              let v;
              const p = this.exec.mem32[vargs >> 2];
              if (prec >= 0) v = this.exec.readLimitedString(p, prec);
              else v = this.exec.readCString(p);
              vargs += 4;
              return v;
            }
        }
      }
    );
    console.log(`GAME: ${msg}`);
  }
  
  egg_time_local(v, a) {
    if (a < 1) return;
    v >>= 2;
    const now = new Date();
    this.exec.mem32[v++] = now.getFullYear();
    if (a < 2) return;
    this.exec.mem32[v++] = 1 + now.getMonth();
    if (a < 3) return;
    this.exec.mem32[v++] = now.getDate();
    if (a < 4) return;
    this.exec.mem32[v++] = now.getHours();
    if (a < 5) return;
    this.exec.mem32[v++] = now.getMinutes();
    if (a < 6) return;
    this.exec.mem32[v++] = now.getSeconds();
    if (a < 7) return;
    this.exec.mem32[v++] = now.getMilliseconds();
  }
  
  egg_get_user_languages(v, a) {
    const src = this.sysExtra.getUserLanguages();
    const cpc = Math.min(a, src.length);
    for (let i=0, dstp=v>>2; i<cpc; i++, dstp++) {
      this.exec.mem32[dstp] = src[i];
    }
    return src.length;
  }
  
  egg_res_get(dst, dsta, tid, qual, rid) {
    const serial = this.rom.getRes(tid, qual, rid);
    return this.exec.safeWrite(dst, dsta, serial);
  }
  
  egg_res_for_each(cb, ctx) {
    if (!(cb = this.exec.fntab.get(cb))) return 0;
    for (const { tid, qual, rid, v } of this.rom.resv) {
      const err = cb(tid, qual, rid, v.length, ctx);
      if (err) return err;
    }
    return 0;
  }
}
