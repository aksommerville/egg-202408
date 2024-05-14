/* Render.js
 */
 
import { Rom } from "./Rom.js";
 
export class Render {
  constructor(egg) {
    this.canvas = egg.canvas;
    this.egg = egg;
    this.gl = this.canvas.getContext("webgl");
    
    // (texid) exposed to client is the index in this array, plus one.
    this.textures = []; // {texid,fbid,w,h,fmt}
  
    this.tint = 0;
    this.alpha = 1;
    this.tr = 0;
    this.tg = 0;
    this.tb = 0;
    this.ta = 0;
  
    this.pgm_raw = null;
    this.pgm_decal = null;
    this.pgm_tile = null;
  
    this.u_raw_screensize = 0;
    this.u_raw_alpha = 0;
    this.u_decal_screensize = 0;
    this.u_decal_sampler = 0;
    this.u_decal_alpha = 0;
    this.u_decal_tint = 0;
    this.u_tile_screensize = 0;
    this.u_tile_sampler = 0;
    this.u_tile_alpha = 0;
    this.u_tile_tint = 0;
    this.u_tile_pointsize = 0;
    
    // Storage for draw_rect, draw_decal, and draw_to_main.
    // Decal is larger, 4 vertices * 12 bytes each.
    this.vbuf = new ArrayBuffer(12 * 4);
    this.vbufu8 = new Uint8Array(this.vbuf);
    this.vbufs16 = new Int16Array(this.vbuf);
    this.vbuff32 = new Float32Array(this.vbuf);
    
    this.gl.blendFunc(this.gl.SRC_ALPHA,this.gl.ONE_MINUS_SRC_ALPHA);
    this.gl.enable(this.gl.BLEND);
    if (!(this.buffer = this.gl.createBuffer())) throw new Error(`Failed to create WebGL vertex buffer.`);
    
    if (
      (this.egg_texture_new() !== 1) ||
      (this.egg_texture_upload(1, this.canvas.width, this.canvas.height, this.canvas.width << 2, 1, 0, 0) < 0)
    ) throw new Error(`Failed to create main framebuffer.`);
    
    this.compileShaders();
  }
  
  /*---------------------------- Entry points for Egg platform ------------------------------*/
  
  // Notify that the runtime is shutting down.
  stop() {
    this.egg_draw_rect(1, 0, 0, this.canvas.width, this.canvas.height, 0x808080ff);
    this.end();
  }
  
  begin() {
    this.egg_render_tint(0x00000000);
    this.alpha = 1;
  }
  
  end() {
    const srctex = this.textures[0];
    if (!srctex) return;

    const dstx = 0, dsty = 0, dstw = this.canvas.width, dsth = this.canvas.height;
    const aposv = this.vbufs16;
    const tcv = this.vbuff32;
    aposv[ 0] = dstx;      aposv[ 1] = dsty;      tcv[ 1] = 0.0; tcv[ 2] = 1.0;
    aposv[ 6] = dstx;      aposv[ 7] = dsty+dsth; tcv[ 4] = 0.0; tcv[ 5] = 0.0;
    aposv[12] = dstx+dstw; aposv[13] = dsty;      tcv[ 7] = 1.0; tcv[ 8] = 1.0;
    aposv[18] = dstx+dstw; aposv[19] = dsty+dsth; tcv[10] = 1.0; tcv[11] = 0.0;
    this.gl.bindBuffer(this.gl.ARRAY_BUFFER, this.buffer);
    this.gl.bufferData(this.gl.ARRAY_BUFFER, this.vbuf, this.gl.STREAM_DRAW);
    
    this.gl.bindFramebuffer(this.gl.FRAMEBUFFER, null);
    this.gl.viewport(0, 0, this.canvas.width, this.canvas.height);
    this.gl.useProgram(this.pgm_decal);
    this.gl.uniform2f(this.u_decal_screensize, this.canvas.width, this.canvas.height);
    this.gl.bindTexture(this.gl.TEXTURE_2D, srctex.texid);
    this.gl.uniform4f(this.u_decal_tint, 0.0, 0.0, 0.0, 0.0);
    this.gl.uniform1f(this.u_decal_alpha, 1.0);
    this.gl.enableVertexAttribArray(0);
    this.gl.enableVertexAttribArray(1);
    this.gl.vertexAttribPointer(0, 2, this.gl.SHORT, false, 12, 0);
    this.gl.vertexAttribPointer(1, 2, this.gl.FLOAT, false, 12, 4);
    this.gl.drawArrays(this.gl.TRIANGLE_STRIP, 0, 4);
    this.gl.disableVertexAttribArray(0);
    this.gl.disableVertexAttribArray(1);
  }
  
  /*------------------------ Public API entry points ---------------------------------*/
  
  egg_texture_del(texid) {
    if ((texid < 2) || (texid > this.textures.length)) return; // sic "<2". You can't delete the main framebuffer.
    const texture = this.textures[texid - 1];
    if (!texture) return;
    if (texture.texid) this.gl.deleteTexture(texture.texid);
    if (texture.fbid) this.gl.deleteFramebuffer(texture.fbid);
    this.textures[texid - 1] = null;
  }
  
  egg_texture_new() {
    const texture = {
      texid: this.gl.createTexture(),
      fbid: null,
      w: 0,
      h: 0,
      fmt: 0,
    };
    if (!texture.texid) return 0;
    this.gl.bindTexture(this.gl.TEXTURE_2D, texture.texid);
    this.gl.texParameteri(this.gl.TEXTURE_2D, this.gl.TEXTURE_WRAP_S, this.gl.CLAMP_TO_EDGE);
    this.gl.texParameteri(this.gl.TEXTURE_2D, this.gl.TEXTURE_WRAP_T, this.gl.CLAMP_TO_EDGE);
    this.gl.texParameteri(this.gl.TEXTURE_2D, this.gl.TEXTURE_MAG_FILTER, this.gl.NEAREST);
    this.gl.texParameteri(this.gl.TEXTURE_2D, this.gl.TEXTURE_MIN_FILTER, this.gl.NEAREST);
    const p = this.textures.indexOf(null);
    if (p >= 0) {
      this.textures[p] = texture;
      return p + 1;
    } else {
      this.textures.push(texture);
      return this.textures.length;
    }
  }
  
  egg_texture_get_header(wp, hp, fmtp, texid) {
    const texture = this.textures[texid - 1];
    if (!texture) return;
    if (wp) this.egg.exec.mem32[wp >> 2] = texture.w;
    if (hp) this.egg.exec.mem32[hp >> 2] = texture.h;
    if (fmtp) this.egg.exec.mem32[fmtp >> 2] = texture.fmt;
  }
  
  egg_texture_load_image(texid, qual, rid) {
    if ((texid < 2) || (texid > this.textures.length)) return -1;
    const texture = this.textures[texid - 1];
    if (!texture) return -1;
    const serial = this.egg.rom.getRes(Rom.RESTYPE_image, qual, rid);
    if (!serial || !serial.length) return -1;
    const image = this.egg.imageDecoder.decode(serial);
    if (!image) return -1;
    return this.loadTexture(texture, image);
  }
  
  egg_texture_upload(texid, w, h, stride, fmt, v, c) {
    if ((texid < 1) || (texid > this.textures.length)) return -1;
    const texture = this.textures[texid - 1];
    if (!texture) return -1;
    if (texid === 1) { // Allowed to upload to texid 1, but not allowed to resize it.
      if (texture.w && texture.h) {
        if ((w !== texture.w) || (h !== texture.h)) return -1;
      }
    }
    let mem = null;
    if (c) {
      if (!(mem = this.egg.exec.getView(v, c))) return -1;
    }
    return this.loadTexture(texture, {
      v: mem,
      w, h, stride, fmt,
    });
  }
  
  egg_texture_clear(texid) {
    const texture = this.textures[texid - 1];
    if (!texture) return;
    this.requireFramebuffer(texture);
    this.gl.bindFramebuffer(this.gl.FRAMEBUFFER, texture.fbid);
    this.gl.clearColor(0.0, 0.0, 0.0, 0.0);
    this.gl.clear(this.gl.COLOR_BUFFER_BIT);
  }
  
  egg_render_tint(tint) {
    this.tint = tint;
    this.tr = ((tint >> 24) & 0xff) / 255.0;
    this.tg = ((tint >> 16) & 0xff) / 255.0;
    this.tb = ((tint >> 8) & 0xff) / 255.0;
    this.ta = (tint & 0xff) / 255.0;
  }
  
  egg_render_alpha(a) {
    this.alpha = a / 255.0;
  }
  
  egg_draw_rect(dsttexid, x, y, w, h, rgba) {
    const texture = this.textures[dsttexid - 1];
    if (!texture) return;
    const r = (rgba >> 24) & 0xff;
    const g = (rgba >> 16) & 0xff;
    const b = (rgba >> 8) & 0xff;
    const a = rgba & 0xff;
    this.requireFramebuffer(texture);
    this.gl.bindFramebuffer(this.gl.FRAMEBUFFER, texture.fbid);
    this.gl.useProgram(this.pgm_raw);
    this.gl.viewport(0, 0, texture.w, texture.h);
    this.gl.uniform2f(this.u_raw_screensize, texture.w, texture.h);
    this.gl.uniform1f(this.u_raw_alpha, this.alpha);
    
    const aposv = this.vbufs16;
    const colorv = this.vbufu8;
    aposv[ 0] = x;   aposv[ 1] = y;   colorv[ 4] = r; colorv[ 5] = g; colorv[ 6] = b; colorv[ 7] = a;
    aposv[ 4] = x;   aposv[ 5] = y+h; colorv[12] = r; colorv[13] = g; colorv[14] = b; colorv[15] = a;
    aposv[ 8] = x+w; aposv[ 9] = y;   colorv[20] = r; colorv[21] = g; colorv[22] = b; colorv[23] = a;
    aposv[12] = x+w; aposv[13] = y+h; colorv[28] = r; colorv[29] = g; colorv[30] = b; colorv[31] = a;
    this.gl.bindBuffer(this.gl.ARRAY_BUFFER, this.buffer);
    this.gl.bufferData(this.gl.ARRAY_BUFFER, this.vbuf, this.gl.STREAM_DRAW);
    
    this.gl.enableVertexAttribArray(0);
    this.gl.enableVertexAttribArray(1);
    this.gl.vertexAttribPointer(0, 2, this.gl.SHORT, false, 8, 0);
    this.gl.vertexAttribPointer(1, 4, this.gl.UNSIGNED_BYTE, true, 8, 4);
    this.gl.drawArrays(this.gl.TRIANGLE_STRIP, 0, 4);
    this.gl.disableVertexAttribArray(0);
    this.gl.disableVertexAttribArray(1);
  }
  
  drawRaw(dsttexid, mode, v, c) {
    const texture = this.textures[dsttexid - 1];
    if (!texture) return;
    this.requireFramebuffer(texture);
    this.gl.bindFramebuffer(this.gl.FRAMEBUFFER, texture.fbid);
    this.gl.useProgram(this.pgm_raw);
    this.gl.viewport(0, 0, texture.w, texture.h);
    this.gl.uniform2f(this.u_raw_screensize, texture.w, texture.h);
    this.gl.uniform1f(this.u_raw_alpha, this.alpha);
    
    const len = c * 8;
    const srcview = this.egg.exec.getView(v, len);
    if (!srcview) return;
    this.gl.bindBuffer(this.gl.ARRAY_BUFFER, this.buffer);
    this.gl.bufferData(this.gl.ARRAY_BUFFER, srcview, this.gl.STREAM_DRAW);
    
    this.gl.enableVertexAttribArray(0);
    this.gl.enableVertexAttribArray(1);
    this.gl.vertexAttribPointer(0, 2, this.gl.SHORT, false, 8, 0);
    this.gl.vertexAttribPointer(1, 4, this.gl.UNSIGNED_BYTE, true, 8, 4);
    this.gl.drawArrays(mode, 0, c);
    this.gl.disableVertexAttribArray(0);
    this.gl.disableVertexAttribArray(1);
  }
  
  egg_draw_line(dsttexid, v, c) {
    this.drawRaw(dsttexid, this.gl.LINE_STRIP, v, c);
  }
  
  egg_draw_trig(dsttexid, v, c) {
    this.drawRaw(dsttexid, this.gl.TRIANGLE_STRIP, v, c);
  }
  
  egg_draw_decal(dsttexid, srctexid, dstx, dsty, srcx, srcy, w, h, xform) {
    if (dsttexid === srctexid) return;
    const dsttex = this.textures[dsttexid - 1];
    const srctex = this.textures[srctexid - 1];
    if (!dsttex || !srctex) return;
    this.requireFramebuffer(dsttex);
    
    let dstw = w, dsth = h;
    if (xform & 4) { // SWAP
      dstw = h;
      dsth = w;
    }
    const aposv = this.vbufs16;
    const tcv = this.vbuff32;
    aposv[ 0] = dstx;      aposv[ 1] = dsty;      tcv[ 1] = 0.0; tcv[ 2] = 0.0;
    aposv[ 6] = dstx;      aposv[ 7] = dsty+dsth; tcv[ 4] = 0.0; tcv[ 5] = 1.0;
    aposv[12] = dstx+dstw; aposv[13] = dsty;      tcv[ 7] = 1.0; tcv[ 8] = 0.0;
    aposv[18] = dstx+dstw; aposv[19] = dsty+dsth; tcv[10] = 1.0; tcv[11] = 1.0;
    if (xform & 4) { // SWAP
      for (let i=1; i<12; i+=3) {
        const tmp = tcv[i];
        tcv[i] = tcv[i + 1];
        tcv[i + 1] = tmp;
      }
    }
    if (xform & 1) { // XREV
      for (let i=1; i<12; i+=3) {
        tcv[i] = 1.0 - tcv[i];
      }
    }
    if (xform & 2) { // YREV
      for (let i=2; i<12; i+=3) {
        tcv[i] = 1.0 - tcv[i];
      }
    }
    const tx0 = srcx / srctex.w;
    const tx1 = w / srctex.w;
    const ty0 = srcy / srctex.h;
    const ty1 = h / srctex.h;
    for (let i=1; i<12; i+=3) {
      tcv[i] = tx0 + tx1 * tcv[i];
      tcv[i+1] = ty0 + ty1 * tcv[i+1];
    }
    this.gl.bindBuffer(this.gl.ARRAY_BUFFER, this.buffer);
    this.gl.bufferData(this.gl.ARRAY_BUFFER, this.vbuf, this.gl.STREAM_DRAW);
    
    this.gl.bindFramebuffer(this.gl.FRAMEBUFFER, dsttex.fbid);
    this.gl.viewport(0, 0, dsttex.w, dsttex.h);
    this.gl.useProgram(this.pgm_decal);
    this.gl.uniform2f(this.u_decal_screensize, dsttex.w, dsttex.h);
    this.gl.bindTexture(this.gl.TEXTURE_2D, srctex.texid);
    this.gl.uniform4f(this.u_decal_tint, this.tr, this.tg, this.tb, this.ta);
    this.gl.uniform1f(this.u_decal_alpha, this.alpha);
    this.gl.enableVertexAttribArray(0);
    this.gl.enableVertexAttribArray(1);
    this.gl.vertexAttribPointer(0, 2, this.gl.SHORT, false, 12, 0);
    this.gl.vertexAttribPointer(1, 2, this.gl.FLOAT, false, 12, 4);
    this.gl.drawArrays(this.gl.TRIANGLE_STRIP, 0, 4);
    this.gl.disableVertexAttribArray(0);
    this.gl.disableVertexAttribArray(1);
  }
  
  egg_draw_decal_mode7(dsttexid, srctexid, dstx, dsty, srcx, srcy, w, h, rotation, xscale, yscale) {
    if (dsttexid === srctexid) return;
    const dsttex = this.textures[dsttexid - 1];
    const srctex = this.textures[srctexid - 1];
    if (!dsttex || !srctex) return;
    this.requireFramebuffer(dsttex);
    rotation /= 65536;
    xscale /= 65536;
    yscale /= 65536;
    
    const aposv = this.vbufs16;
    const tcv = this.vbuff32;
    const cost = Math.cos(-rotation);
    const sint = Math.sin(-rotation);
    const halfw = w * xscale * 0.5;
    const halfh = h * yscale * 0.5;
    const nwx = Math.round( cost * halfw + sint * halfh);
    const nwy = Math.round(-sint * halfw + cost * halfh);
    const swx = Math.round( cost * halfw - sint * halfh);
    const swy = Math.round(-sint * halfw - cost * halfh);
    aposv[ 0] = dstx - nwx; aposv[ 1] = dsty - nwy; tcv[ 1] = 0.0; tcv[ 2] = 0.0;
    aposv[ 6] = dstx - swx; aposv[ 7] = dsty - swy; tcv[ 4] = 0.0; tcv[ 5] = 1.0;
    aposv[12] = dstx + swx; aposv[13] = dsty + swy; tcv[ 7] = 1.0; tcv[ 8] = 0.0;
    aposv[18] = dstx + nwx; aposv[19] = dsty + nwy; tcv[10] = 1.0; tcv[11] = 1.0;
    const tx0 = (srcx + 0.5) / srctex.w;
    const tx1 = (w - 1) / srctex.w;
    const ty0 = (srcy + 0.5) / srctex.h;
    const ty1 = (h - 1) / srctex.h;
    for (let i=1; i<12; i+=3) {
      tcv[i] = tx0 + tx1 * tcv[i];
      tcv[i+1] = ty0 + ty1 * tcv[i+1];
    }
    this.gl.bindBuffer(this.gl.ARRAY_BUFFER, this.buffer);
    this.gl.bufferData(this.gl.ARRAY_BUFFER, this.vbuf, this.gl.STREAM_DRAW);
    
    this.gl.bindFramebuffer(this.gl.FRAMEBUFFER, dsttex.fbid);
    this.gl.viewport(0, 0, dsttex.w, dsttex.h);
    this.gl.useProgram(this.pgm_decal);
    this.gl.uniform2f(this.u_decal_screensize, dsttex.w, dsttex.h);
    this.gl.bindTexture(this.gl.TEXTURE_2D, srctex.texid);
    this.gl.uniform4f(this.u_decal_tint, this.tr, this.tg, this.tb, this.ta);
    this.gl.uniform1f(this.u_decal_alpha, this.alpha);
    this.gl.enableVertexAttribArray(0);
    this.gl.enableVertexAttribArray(1);
    this.gl.vertexAttribPointer(0, 2, this.gl.SHORT, false, 12, 0);
    this.gl.vertexAttribPointer(1, 2, this.gl.FLOAT, false, 12, 4);
    this.gl.drawArrays(this.gl.TRIANGLE_STRIP, 0, 4);
    this.gl.disableVertexAttribArray(0);
    this.gl.disableVertexAttribArray(1);
  }
  
  egg_draw_tile(dsttexid, srctexid, v, c) {
    if (!v || (c < 1)) return;
    if (dsttexid === srctexid) return;
    const dsttex = this.textures[dsttexid - 1];
    const srctex = this.textures[srctexid - 1];
    if (!dsttex || !srctex) return;
    this.requireFramebuffer(dsttex);
    this.gl.bindFramebuffer(this.gl.FRAMEBUFFER, dsttex.fbid);
    this.gl.useProgram(this.pgm_tile);
    this.gl.viewport(0, 0, dsttex.w, dsttex.h);
    this.gl.uniform2f(this.u_tile_screensize, dsttex.w, dsttex.h);
    this.gl.bindTexture(this.gl.TEXTURE_2D, srctex.texid);
    
    const len = c * 6;
    const srcview = this.egg.exec.getView(v, len);
    if (!srcview) return;
    this.gl.bindBuffer(this.gl.ARRAY_BUFFER, this.buffer);
    this.gl.bufferData(this.gl.ARRAY_BUFFER, srcview, this.gl.STREAM_DRAW);
    
    this.gl.uniform4f(this.u_tile_tint, this.tr, this.tg, this.tb, this.ta);
    this.gl.uniform1f(this.u_tile_alpha, this.alpha);
    this.gl.uniform1f(this.u_tile_pointsize, srctex.w >> 4);
    this.gl.enableVertexAttribArray(0);
    this.gl.enableVertexAttribArray(1);
    this.gl.enableVertexAttribArray(2);
    this.gl.vertexAttribPointer(0, 2, this.gl.SHORT, false, 6, 0);
    this.gl.vertexAttribPointer(1, 1, this.gl.UNSIGNED_BYTE, false, 6, 4);
    this.gl.vertexAttribPointer(2, 1, this.gl.UNSIGNED_BYTE, false, 6, 5);
    this.gl.drawArrays(this.gl.POINTS, 0, c);
    this.gl.disableVertexAttribArray(0);
    this.gl.disableVertexAttribArray(1);
    this.gl.disableVertexAttribArray(2);
  }
  
  /*------------------------------ Private -----------------------------------*/
   
  /* (texture) is from our list.
   * (image) is {v,w,h,fmt,stride} where (v) is null or Uint8Array.
   */
  loadTexture(texture, image) {
    this.gl.bindTexture(this.gl.TEXTURE_2D, texture.texid);
    let ifmt = this.gl.RGBA, fmt = this.gl.RGBA, type = this.gl.UNSIGNED_BYTE;
    switch (image.fmt) {
      case 1: break; // RGBA, already initted like that
      case 2: ifmt = this.gl.ALPHA; fmt = this.gl.ALPHA; break;
      case 3: image = this.expand1(image, 0x00000000, 0x000000ff); break; // a1
      default: return -1;
    }
    this.gl.texImage2D(this.gl.TEXTURE_2D, 0, ifmt, image.w, image.h, 0, fmt, type, image.v);
    texture.w = image.w;
    texture.h = image.h;
    texture.fmt = image.fmt;
    return 0;
  }
  
  // Return an RGBA image from something 1-bit.
  expand1(image, zero, one) {
    const dststride = image.w << 2;
    const dst = new Uint8Array(dststride * image.h);
    for (let dstp=0, srcp=0, yi=image.h; yi-->0; srcp+=image.stride) {
      for (let xi=image.w, srcmask=0x80, srcpp=srcp; xi-->0; ) {
        if (image.v[srcpp] & srcmask) {
          dst[dstp++] = one >> 24;
          dst[dstp++] = one >> 16;
          dst[dstp++] = one >> 8;
          dst[dstp++] = one;
        } else {
          dst[dstp++] = zero >> 24;
          dst[dstp++] = zero >> 16;
          dst[dstp++] = zero >> 8;
          dst[dstp++] = zero;
        }
        if (srcmask === 1) { srcmask = 0x80; srcpp++; }
        else srcmask >>= 1;
      }
    }
    return {
      v: dst,
      w: image.w,
      h: image.h,
      fmt: 1, // RGBA
      stride: dststride,
    };
  }
  
  requireFramebuffer(texture) {
    if (texture.fbid) return;
    if (!(texture.fbid = this.gl.createFramebuffer())) throw new Error(`Failed to create WebGL framebuffer object.`);
    this.gl.bindFramebuffer(this.gl.FRAMEBUFFER, texture.fbid);
    this.gl.framebufferTexture2D(this.gl.FRAMEBUFFER, this.gl.COLOR_ATTACHMENT0, this.gl.TEXTURE_2D, texture.texid, 0);
    this.gl.bindFramebuffer(this.gl.FRAMEBUFFER, null);
  }
  
  compileShaders() {
  
    this.pgm_raw = this.compileShader("raw", Render.vsrc_raw, Render.fsrc_raw);
    this.gl.useProgram(this.pgm_raw);
    this.u_raw_screensize = this.gl.getUniformLocation(this.pgm_raw, "screensize");
    this.u_raw_alpha = this.gl.getUniformLocation(this.pgm_raw, "alpha");
    this.gl.bindAttribLocation(this.pgm_raw, 0, "apos");
    this.gl.bindAttribLocation(this.pgm_raw, 1, "acolor");
  
    this.pgm_decal = this.compileShader("decal", Render.vsrc_decal, Render.fsrc_decal);
    this.gl.useProgram(this.pgm_decal);
    this.u_decal_screensize = this.gl.getUniformLocation(this.pgm_decal, "screensize");
    this.u_decal_sampler = this.gl.getUniformLocation(this.pgm_decal, "sampler");
    this.u_decal_alpha = this.gl.getUniformLocation(this.pgm_decal, "alpha");
    this.u_decal_tint = this.gl.getUniformLocation(this.pgm_decal, "tint");
    this.gl.bindAttribLocation(this.pgm_decal, 0, "apos");
    this.gl.bindAttribLocation(this.pgm_decal, 1, "atexcoord");
  
    this.pgm_tile = this.compileShader("tile", Render.vsrc_tile, Render.fsrc_tile);
    this.gl.useProgram(this.pgm_tile);
    this.u_tile_screensize = this.gl.getUniformLocation(this.pgm_tile, "screensize");
    this.u_tile_sampler = this.gl.getUniformLocation(this.pgm_tile, "sampler");
    this.u_tile_alpha = this.gl.getUniformLocation(this.pgm_tile, "alpha");
    this.u_tile_tint = this.gl.getUniformLocation(this.pgm_tile, "tint");
    this.u_tile_pointsize = this.gl.getUniformLocation(this.pgm_tile, "pointsize");
    this.gl.bindAttribLocation(this.pgm_tile, 0, "apos");
    this.gl.bindAttribLocation(this.pgm_tile, 1, "atileid");
    this.gl.bindAttribLocation(this.pgm_tile, 2, "axform");
  }
  
  compileShader(name, vsrc, fsrc) {
    const pid = this.gl.createProgram();
    if (!pid) throw new Error(`Failed to create new WebGL program for ${JSON.stringify(name)}`);
    try {
      this.compileShader1(name, pid, this.gl.VERTEX_SHADER, vsrc);
      this.compileShader1(name, pid, this.gl.FRAGMENT_SHADER, fsrc);
      this.gl.linkProgram(pid);
      if (!this.gl.getProgramParameter(pid, this.gl.LINK_STATUS)) {
        const log = this.gl.getProgramInfoLog(pid);
        throw new Error(`Failed to link program ${JSON.stringify(name)}:\n${log}`);
      }
    } catch (e) {
      this.gl.deleteProgram(pid);
      throw e;
    }
    return pid;
  }
  
  compileShader1(name, pid, type, src) {
    const sid = this.gl.createShader(type);
    if (!sid) throw new Error(`Failed to create new WebGL shader for ${JSON.stringify(name)}`);
    try {
      this.gl.shaderSource(sid, src);
      this.gl.compileShader(sid);
      if (!this.gl.getShaderParameter(sid, this.gl.COMPILE_STATUS)) {
        const log = this.gl.getShaderInfoLog(sid);
        throw new Error(`Failed to link ${(type === this.gl.VERTEX_SHADER) ? "vertex" : "fragment"} shader for ${JSON.stringify(name)}:\n${log}`);
      }
      this.gl.attachShader(pid, sid);
    } finally {
      this.gl.deleteShader(sid);
    }
  }
}

/* GLSL
 * TODO These need a minification strategy separate from the general Javascript one.
 ***********************************************************/
 
Render.vsrc_raw = `
  #version 100
  precision mediump float;
  uniform vec2 screensize;
  attribute vec2 apos;
  attribute vec4 acolor;
  varying vec4 vcolor;
  void main() {
    vec2 npos=(apos*2.0)/screensize-1.0;
    gl_Position=vec4(npos,0.0,1.0);
    vcolor=acolor;
  }
`;

Render.fsrc_raw = `
  #version 100
  precision mediump float;
  uniform float alpha;
  varying vec4 vcolor;
  void main() {
    gl_FragColor=vec4(vcolor.rgb,vcolor.a*alpha);
  }
`;
 
Render.vsrc_decal = `
  #version 100
  precision mediump float;
  uniform vec2 screensize;
  attribute vec2 apos;
  attribute vec2 atexcoord;
  varying vec2 vtexcoord;
  void main() {
    vec2 npos=(apos*2.0)/screensize-1.0;
    gl_Position=vec4(npos,0.0,1.0);
    vtexcoord=atexcoord;
  }
`;

Render.fsrc_decal = `
  #version 100
  precision mediump float;
  uniform sampler2D sampler;
  uniform float alpha;
  uniform vec4 tint;
  varying vec2 vtexcoord;
  void main() {
    gl_FragColor=texture2D(sampler,vtexcoord);
    gl_FragColor=vec4(mix(gl_FragColor.rgb,tint.rgb,tint.a),gl_FragColor.a*alpha);
  }
`;
 
Render.vsrc_tile = `
  #version 100
  precision mediump float;
  uniform vec2 screensize;
  uniform float pointsize;
  attribute vec2 apos;
  attribute float atileid;
  attribute float axform;
  varying vec2 vsrcp;
  varying mat2 vmat;
  void main() {
    vec2 npos=(apos*2.0)/screensize-1.0;
    gl_Position=vec4(npos,0.0,1.0);
    vsrcp=vec2(
      mod(atileid,16.0),
      floor(atileid/16.0)
    )/16.0;
         if (axform<0.5) vmat=mat2( 1.0, 0.0, 0.0, 1.0); // no xform
    else if (axform<1.5) vmat=mat2(-1.0, 0.0, 0.0, 1.0); // XREV
    else if (axform<2.5) vmat=mat2( 1.0, 0.0, 0.0,-1.0); // YREV
    else if (axform<3.5) vmat=mat2(-1.0, 0.0, 0.0,-1.0); // XREV|YREV
    else if (axform<4.5) vmat=mat2( 0.0, 1.0, 1.0, 0.0); // SWAP
    else if (axform<5.5) vmat=mat2( 0.0, 1.0,-1.0, 0.0); // SWAP|XREV
    else if (axform<6.5) vmat=mat2( 0.0,-1.0, 1.0, 0.0); // SWAP|YREV
    else if (axform<7.5) vmat=mat2( 0.0,-1.0,-1.0, 0.0); // SWAP|XREV|YREV
                    else vmat=mat2( 1.0, 0.0, 0.0, 1.0); // invalid; use identity
    gl_PointSize=pointsize;
  }
`;

Render.fsrc_tile = `
  #version 100
  precision mediump float;
  uniform sampler2D sampler;
  uniform float alpha;
  uniform vec4 tint;
  varying vec2 vsrcp;
  varying mat2 vmat;
  void main() {
    vec2 texcoord=gl_PointCoord;
    texcoord.y=1.0-texcoord.y;
    texcoord=vmat*(texcoord-0.5)+0.5;
    texcoord=vsrcp+texcoord/16.0;
    gl_FragColor=texture2D(sampler,texcoord);
    gl_FragColor=vec4(mix(gl_FragColor.rgb,tint.rgb,tint.a),gl_FragColor.a*alpha);
  }
`;
