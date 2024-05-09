/* Render.js
 */
 
export class Render {
  constructor(canvas, data) {
    this.canvas = canvas;
    this.data = data;
    this.context = this.canvas.getContext("2d");//TODO webgl
  }
  
  // Notify that the runtime is shutting down.
  stop() {
    this.context.fillStyle = "#888";
    this.context.fillRect(0, 0, this.canvas.width, this.canvas.height);
  }
  
  begin() {
    this.context.fillStyle = "#00f";
    this.context.fillRect(0, 0, this.canvas.width, this.canvas.height);
  }
  
  end() {
  }
  
  /*------------------------ Public API entry points ---------------------------------*/
  
  egg_texture_del(texid) {
    console.log(`TODO egg_texture_del`, { texid });
  }
  
  egg_texture_new() {
    console.log(`TODO egg_texture_new`);
    return 0;
  }
  
  egg_texture_get_header(wp, hp, fmtp, texid) {
    console.log(`TODO egg_texture_get_header`, { wp, hp, fmtp, texid });
  }
  
  egg_texture_load_image(texid, qual, rid) {
    console.log(`TODO egg_texture_load_image`, { texid, qual, rid });
    return -1;
  }
  
  egg_texture_upload(texid, w, h, stride, fmt, v, c) {
    console.log(`TODO egg_texture_upload`, { texid, w, h, stride, fmt, v, c });
    return -1;
  }
  
  egg_texture_clear(texid) {
    console.log(`TODO egg_texture_clear`);
  }
  
  egg_render_tint(rgba) {
    console.log(`TODO egg_render_tint`, { rgba });
  }
  
  egg_render_alpha(a) {
    console.log(`TODO egg_render_alpha`, { a });
  }
  
  egg_draw_rect(dsttexid, x, y, w, h, rgba) {
    console.log(`TODO egg_draw_rect`, { dsttexid, x, y, w, h, rgba });
  }
  
  egg_draw_line(dsttexid, v, c) {
    console.log(`TODO egg_draw_line`, { dsttexid, v, c });
  }
  
  egg_draw_decal(dsttexid, srctexid, dstx, dsty, srcx, srcy, w, h, xform) {
    console.log(`TODO egg_draw_decal`, { dsttexid, srctexid, dstx, dsty, srcx, srcy, w, h, xform });
  }
  
  egg_draw_tile(dsttexid, srctexid, v, c) {
    console.log(`TODO egg_draw_tile`, { dsttexid, srctexid, v, c });
  }
}
