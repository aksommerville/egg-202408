/* egg_video.h
 */
 
#ifndef EGG_VIDEO_H
#define EGG_VIDEO_H

/* OpenGL ES 2 helpers.
 * If your metdata resource contains "framebuffer=WIDTHxHEIGHT gl", the platform
 * provides a raw GLES2 context and does not initialize Egg's bespoke render API.
 * Use the helpers in this section, and ignore the rest of this header.
 * Your declared framebuffer size in this case is only a hint for the preferred aspect ratio.
 * Include <GLES2/gl2.h> and work from that.
 *
 * You must use VBOs for vertex data.
 * Passing pointers directly to glVertexAttribPointer() will work for native hosts, but not on the web.
 *****************************************************************/

/* For glGetString(), you must supply a buffer in client memory where we can write the strings.
 */
void egg_video_set_string_buffer(char *v,int c);

/* Direct OpenGL, you are drawing into the main output, which can change size between frames.
 * Egg doesn't send events on video size changes (because our preferred video API wouldn't need them).
 * Recommend to call this at the start of each render.
 */
void egg_video_get_size(int *w,int *h);
 
/* Egg Video API.
 * If you don't want the complexity of coding to GLES2 from scratch,
 * we provide a nice simple API for primitives and blitters.
 * Under this regime, the framebuffer size you declare in metadata is what you get, exactly.
 * This can also be abused as a software framebuffer: Upload pixels to texture 1.
 *********************************************************************/

#define EGG_TEX_FMT_RGBA 1 /* 32-bit RGBA, red first. */
#define EGG_TEX_FMT_A8   2 /* 8-bit alpha. */
#define EGG_TEX_FMT_A1   3 /* 1-bit alpha. Big-endian (ie first pixel is 0x80). */

#define EGG_XFORM_XREV 1
#define EGG_XFORM_YREV 2
#define EGG_XFORM_SWAP 4

/* Texture objects.
 **********************************************************************/

/* Valid texid are >0.
 * There is always a texture ID 1, which represents the main output.
 * You can read its bounds and draw to it, but you can't load images or change its dimensions.
 * DO NOT assume that Egg texid are actual OpenGL texture IDs -- that's up to the platform.
 */
void egg_texture_del(int texid);
int egg_texture_new();

void egg_texture_get_header(int *w,int *h,int *fmt,int texid);

/* Decode an image resource and load it into a texture.
 * <0 on errors, in which case the content of the texture is undefined.
 * For small games, it's normal to load all your images at startup.
 * When you get up to a dozen or so, consider swapping them in and out as needed.
 * There is no well-defined limit to how much texture can be loaded at once (but a limit surely exists).
 * This call is equivalent to egg_res_get() followed by egg_texture_upload() but much more efficient.
 */
int egg_texture_load_image(int texid,int qual,int imageid);

/* Replace a texture with raw content from the client.
 * (stride) may be zero to have us use the minimum based on (w) and (fmt).
 * If (v,c) are both zero, allocate the texture and leave its content undefined.
 * If (w,h,stride,fmt) are all zero, (v,c) may be a full PNG file.
 * Otherwise (c) is in bytes and must be (h*stride).
 * Uploading to texid 1 is legal only for raw pixels, and only if the dimensions stay the same.
 * <0 on errors, 0 on success.
 */
int egg_texture_upload(int texid,int w,int h,int stride,int fmt,const void *v,int c);

/* Wipe content to zeroes.
 */
void egg_texture_clear(int texid);

/* Rendering.
 ********************************************************************/

/* Global tint and alpha.
 * These reset to 0x00000000 and 0xff at the start of each render cycle.
 * The alpha channel of tint controls how much tinting.
 */
void egg_render_tint(uint32_t rgba);
void egg_render_alpha(uint8_t a);

/* Plain axis-aligned rectangle.
 */
void egg_draw_rect(int dsttexid,int x,int y,int w,int h,int rgba);

/* Single-pixel segmented line.
 */
struct egg_draw_line {
  int16_t x,y;
  uint8_t r,g,b,a;
};
void egg_draw_line(int dsttexid,const struct egg_draw_line *v,int c);

/* Untextured triangle strip.
 * You can use this for rectangle gradients, for approximated ovals, or maybe some lo-fi 3d?
 * (3d, you'd have to do all the projection yourself, it's probably not workable).
 */
void egg_draw_trig(int dsttexid,const struct egg_draw_line *v,int c);

/* Copy some portion of one image onto another.
 * (dstx,dsty) is the top-left corner of output, regardless of (xform).
 * Output bounds are (h,w) if EGG_XFORM_SWAP in play. Input bounds are always (w,h).
 */
void egg_draw_decal(
  int dsttexid,int srctexid,
  int dstx,int dsty,
  int srcx,int srcy,
  int w,int h,
  int xform
);

/* Copy from one texture to another with scaling and rotation.
 * (dstx,dsty) is the *center* of output. Not the top-left corner as with egg_draw_decal.
 * No (xform) option, because you can reproduce it exactly using (rotation,xscale,yscale).
 * (rotation) is in radians clockwise.
 */
void egg_draw_decal_mode7(
  int dsttexid,int srctexid,
  int dstx,int dsty,
  int srcx,int srcy,
  int w,int h,
  int rotation,int xscale,int yscale // all fixed-point s16.16
);

/* Draw any number of square axis-aligned tiles.
 * (x,y) is the center of the output.
 * (srctexid) must contain 256 discrete images arranged in a square:
 *   0 1 ... 15
 *   ...
 *   240 ... 255
 */
struct egg_draw_tile {
  int16_t x,y;
  uint8_t tileid;
  uint8_t xform;
};
void egg_draw_tile(int dsttexid,int srctexid,const struct egg_draw_tile *v,int c);

/* Access to image decoder for software rendering.
 * For ordinary rendering, use egg_texture_load_image(), it's much more efficient.
 * We do not provide access for decoding image files in client memory, only for ones stored as resources.
 **********************************************************************/

/* Fetch the geometry of an image resource.
 * Total size required will be (h*stride).
 * Leaves all return vectors unset on any error.
 */
void egg_image_get_header(int *w,int *h,int *stride,int *fmt,int qual,int imageid);

/* Decode an image and write the raw pixels at (dst).
 * Returns total length. If (>dsta), output is undefined.
 * You must call egg_image_get_header() first to determine the buffer size.
 */
int egg_image_decode(void *dst,int dsta,int qual,int imageid);

#endif
