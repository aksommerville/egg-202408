/* render.h
 * Egg's default renderer, using GLES2.
 */
 
#ifndef RENDER_H
#define RENDER_H

#include <stdint.h>

struct render;
struct egg_draw_line;
struct egg_draw_tile;

void render_del(struct render *render);
struct render *render_new();

void render_texture_del(struct render *render,int texid);
int render_texture_new(struct render *render);

/* (w,h,stride,fmt) zero to decode.
 * (stride,src,srcc) zero to allocate, content initially undefined.
 */
int render_texture_load(struct render *render,int texid,int w,int h,int stride,int fmt,const void *src,int srcc);

void render_texture_get_header(int *w,int *h,int *fmt,const struct render *render,int texid);

// Caller frees, if not null.
void *render_texture_get_pixels(int *w,int *h,int *fmt,struct render *render,int texid);

void render_texture_clear(struct render *render,int texid);

void render_tint(struct render *render,uint32_t rgba);
void render_alpha(struct render *render,uint8_t a);

void render_draw_rect(struct render *render,int texid,int x,int y,int w,int h,uint32_t pixel);

void render_draw_line(struct render *render,int texid,const struct egg_draw_line *v,int c);

// Triangle strip.
void render_draw_trig(struct render *render,int texid,const struct egg_draw_line *v,int c);

void render_draw_decal(
  struct render *render,
  int dsttexid,int srctexid,
  int dstx,int dsty,
  int srcx,int srcy,
  int w,int h,
  int xform
);

void render_draw_decal_mode7(
  struct render *render,
  int dsttexid,int srctexid,
  int dstx,int dsty, // center
  int srcx,int srcy,
  int w,int h,
  double rotation,double xscale,double yscale
);

void render_draw_tile(
  struct render *render,
  int dsttexid,int srctexid,
  const struct egg_draw_tile *v,int c
);

void render_draw_to_main(struct render *render,int mainw,int mainh,int texid);

void render_coords_fb_from_screen(struct render *render,int *x,int *y);
void render_coords_screen_from_fb(struct render *render,int *x,int *y);

#endif
