/* Exec.js
 * Owns the WebAssembly context.
 */
 
import { Rom } from "./Rom.js";
 
export class Exec {
  constructor(egg) {
    this.egg = egg;
    if (!window.WebAssembly) throw new Error("WebAssembly not supported");
    this.textDecoder = new TextDecoder("utf8");
    this.textEncoder = new TextEncoder("utf8");
    this.memory = null;
    this.egg_client_quit = () => {};
    this.egg_client_init = () => -1;
    this.egg_client_update = () => {};
    this.egg_client_render = () => {};
    this.mem8 = null;
    this.mem32 = null;
    this.memf64 = null;
    this.fntab = null;
  }
  
  load() {
    const serial = this.egg.rom.getRes(Rom.RESTYPE_wasm, 0, 1);
    const options = { env: {
      egg_log: (f, v) => this.egg.egg_log(f, v),
      egg_time_real: () => Date.now() / 1000,
      egg_time_local: (v, a) => this.egg.egg_time_local(v, a),
      egg_request_termination: () => this.egg.stop(),
      egg_get_user_languages: (v, a) => this.egg.egg_get_user_languages(v, a),
      egg_video_get_size: (wp, hp) => this.egg.render.egg_video_get_size(wp, hp),
      egg_texture_del: (texid) => this.egg.render.egg_texture_del(texid),
      egg_texture_new: () => this.egg.render.egg_texture_new(),
      egg_texture_get_header: (w, h, fmt, texid) => this.egg.render.egg_texture_get_header(w, h, fmt, texid),
      egg_texture_load_image: (texid, qual, rid) => this.egg.render.egg_texture_load_image(texid, qual, rid),
      egg_texture_upload: (texid, w, h, stride, fmt, v, c) => this.egg.render.egg_texture_upload(texid, w, h, stride, fmt, v, c),
      egg_texture_clear: (texid) => this.egg.render.egg_texture_clear(texid),
      egg_render_tint: (rgba) => this.egg.render.egg_render_tint(rgba),
      egg_render_alpha: (a) => this.egg.render.egg_render_alpha(a),
      egg_draw_rect: (dt, x, y, w, h, c) => this.egg.render.egg_draw_rect(dt, x, y, w, h, c),
      egg_draw_line: (dt, v, c) => this.egg.render.egg_draw_line(dt, v, c),
      egg_draw_trig: (dt, v, c) => this.egg.render.egg_draw_trig(dt, v, c),
      egg_draw_decal: (dt, st, dx, dy, sx, sy, w, h, xf) => this.egg.render.egg_draw_decal(dt, st, dx, dy, sx, sy, w, h, xf),
      egg_draw_decal_mode7: (dt, st, dx, dy, sx, sy, w, h, r, xs, ys) => this.egg.render.egg_draw_decal_mode7(dt, st, dx, dy, sx, sy, w, h, r, xs, ys),
      egg_draw_tile: (dt, st, v, c) => this.egg.render.egg_draw_tile(dt, st, v, c),
      egg_image_get_header: (wp, hp, sp, fp, qual, rid) => this.egg.data.egg_image_get_header(wp, hp, sp, fp, qual, rid),
      egg_image_decode: (v, a, qual, rid) => this.egg.data.egg_image_decode(v, a, qual, rid),
      egg_res_get: (v, a, tid, qual, rid) => this.egg.egg_res_get(v, a, tid, qual, rid),
      egg_res_for_each: (cb, ctx) => this.egg.egg_res_for_each(cb, ctx),
      egg_store_get: (v, a, k, kc) => this.egg.data.egg_store_get(v, a, k, kc),
      egg_store_set: (k, kc, v, vc) => this.egg.data.egg_store_set(k, kc, v, vc),
      egg_store_key_by_index: (v, a, p) => this.egg.data.egg_store_key_by_index(v, a, p),
      egg_event_get: (v, a) => this.egg.input.egg_event_get(v, a),
      egg_event_enable: (t, e) => this.egg.input.egg_event_enable(t, e),
      egg_show_cursor: (s) => this.egg.input.egg_show_cursor(s),
      egg_lock_cursor: (l) => this.egg.input.egg_lock_cursor(l),
      egg_joystick_devid_by_index: (p) => this.egg.input.egg_joystick_devid_by_index(p),
      egg_joystick_get_ids: (vid, pid, ver, devid) => this.egg.input.egg_joystick_get_ids(vid, pid, ver, devid),
      egg_joystick_get_name: (v, a, devid) => this.egg.input.egg_joystick_get_name(v, a, devid),
      egg_joystick_for_each_button: (devid, cb, ctx) => this.egg.input.egg_joystick_for_each_button(devid, cb, ctx),
      egg_audio_play_song: (qual, rid, f, r) => this.egg.audio.egg_audio_play_song(qual, rid, f, r),
      egg_audio_play_sound: (qual, rid, t, p) => this.egg.audio.egg_audio_play_sound(qual, rid, t, p),
      egg_audio_event: (c, o, a, b) => this.egg.audio.egg_audio_event(c, o, a, b),
      egg_audio_get_playhead: () => this.egg.audio.egg_audio_get_playhead(),
      egg_audio_set_playhead: (b) => this.egg.audio.egg_audio_set_playhead(b),
    }};
    return WebAssembly.instantiate(serial, options).then(result => {
      const yoink = name => {
        if (!result.instance.exports[name]) {
          throw new Error(`ROM does not export required symbol '${name}'`);
        }
        this[name] = result.instance.exports[name];
      };
      yoink("memory");
      yoink("egg_client_quit");
      yoink("egg_client_init");
      yoink("egg_client_update");
      yoink("egg_client_render");
      this.mem8 = new Uint8Array(this.memory.buffer);
      this.mem32 = new Uint32Array(this.memory.buffer);
      this.memf64 = new Float64Array(this.memory.buffer);
      this.fntab = result.instance.exports.__indirect_function_table;
    });
  }
  
  readCString(p) {
    let z = p;
    while (this.mem8[z]) z++;
    return this.textDecoder.decode(this.mem8.slice(p, z));
  }
  
  readLimitedString(p, limit) {
    let z = p;
    while (this.mem8[z] && (limit-- > 0)) z++;
    return this.textDecoder.decode(this.mem8.slice(p, z));
  }
  
  // (src) must be string or Uint8Array
  safeWrite(dst, dsta, src) {
    if (typeof(src) === "string") {
      src = this.textEncoder.encode(src);
    }
    const cpc = Math.min(dsta, src.length);
    if (cpc === src.length) {
      const dstview = new Uint8Array(this.memory.buffer, dst, cpc);
      dstview.set(src);
    } else if (cpc > 0) {
      const dstview = new Uint8Array(this.memory.buffer, dst, cpc);
      const srcview = new Uint8Array(src.buffer, src.byteOffset, cpc);
      dstview.set(srcview);
    }
    return src.length;
  }
  
  // Offset Uint8Array, or null if OOB
  getView(p, c) {
    if ((p < 0) || (c < 0) || (p > this.memory.buffer.byteLength - c)) return null;
    return new Uint8Array(this.memory.buffer, p, c);
  }
}
