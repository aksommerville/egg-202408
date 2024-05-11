/* egg_romsrc_native.c
 * Only one of the egg_romsrc_*.c files actually get used in a given target.
 * This one uses a bundled ROM file for assets, and native functions for code.
 */

#include "egg_runner_internal.h"
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

const int egg_romsrc=EGG_ROMSRC_NATIVE;

extern const char egg_rom_bundled[];
extern const int egg_rom_bundled_length;

/* egg_log: Not part of the general API because it's variadic, so egg_romsrc_external does its own thing,
 * but we can do a much easier (and better) thing.
 */
 
void egg_log(const char *fmt,...) {
  va_list vargs;
  va_start(vargs,fmt);
  char tmp[1024];
  int tmpc=vsnprintf(tmp,sizeof(tmp),fmt,vargs);
  if ((tmpc<0)||(tmpc>sizeof(tmp))) tmpc=0;
  fprintf(stderr,"%.*s\n",tmpc,tmp);
}

/* Public API, simple features.
 * Anything complex enough that we don't want to duplicate in egg_romsrc_external.c, the native version lives in egg_public_api.c:
 *  - egg_get_user_languages
 *  - egg_store_*
 * And all input-related stuff lives in egg_event.c:
 *  - egg_event-*
 *  - egg_*_cursor
 *  - egg_joystick_*
 */
 
double egg_time_real() {
  struct timeval tv={0};
  gettimeofday(&tv,0);
  return (double)tv.tv_sec+(double)tv.tv_usec/1000000.0;
}

void egg_time_local(int *dst,int a) {
  if (a<1) return;
  if (a>7) a=7;
  if (!dst) return;
  time_t now=time(0);
  struct tm tm={0};
  localtime_r(&now,&tm);
  *(dst++)=1900+tm.tm_year;
  if (a<2) return;
  *(dst++)=1+tm.tm_mon;
  if (a<3) return;
  *(dst++)=tm.tm_mday;
  if (a<4) return;
  *(dst++)=tm.tm_hour;
  if (a<5) return;
  *(dst++)=tm.tm_min;
  if (a<6) return;
  *(dst++)=tm.tm_sec;
  if (a<7) return;
  struct timeval tv={0};
  gettimeofday(&tv,0);
  *dst=tv.tv_usec/1000;
}

void egg_request_termination() {
  fprintf(stderr,"%s: Terminating due to request from game.\n",egg.exename);
  egg.terminate=1;
}

void egg_texture_del(int texid) {
  render_texture_del(egg.render,texid);
}

int egg_texture_new() {
  return render_texture_new(egg.render);
}

void egg_texture_get_header(int *w,int *h,int *fmt,int texid) {
  return render_texture_get_header(w,h,fmt,egg.render,texid);
}

int egg_texture_load_image(int texid,int qual,int rid) {
  const void *serial=0;
  int serialc=rom_get(&serial,&egg.rom,EGG_RESTYPE_image,qual,rid);
  if (serialc<=0) return -1;
  return render_texture_load(egg.render,texid,0,0,0,0,serial,serialc);
}

int egg_texture_upload(int texid,int w,int h,int stride,int fmt,const void *v,int c) {
  return render_texture_load(egg.render,texid,w,h,stride,fmt,v,c);
}

void egg_texture_clear(int texid) {
  render_texture_clear(egg.render,texid);
}

void egg_render_tint(uint32_t rgba) {
  render_tint(egg.render,rgba);
}

void egg_render_alpha(uint8_t a) {
  render_alpha(egg.render,a);
}

void egg_draw_rect(int dsttexid,int x,int y,int w,int h,int rgba) {
  render_draw_rect(egg.render,dsttexid,x,y,w,h,rgba);
}

void egg_draw_line(int dsttexid,const struct egg_draw_line *v,int c) {
  render_draw_line(egg.render,dsttexid,v,c);
}

void egg_draw_decal(int dsttexid,int srctexid,int dstx,int dsty,int srcx,int srcy,int w,int h,int xform) {
  render_draw_decal(egg.render,dsttexid,srctexid,dstx,dsty,srcx,srcy,w,h,xform);
}

void egg_draw_tile(int dsttexid,int srctexid,const struct egg_draw_tile *v,int c) {
  render_draw_tile(egg.render,dsttexid,srctexid,v,c);
}

void egg_image_get_header(int *w,int *h,int *stride,int *fmt,int qual,int rid) {
  const void *serial=0;
  int serialc=rom_get(&serial,&egg.rom,EGG_RESTYPE_image,qual,rid);
  struct png_image image={0};
  if (png_decode_header(&image,serial,serialc)<0) return;
  *w=image.w;
  *h=image.h;
  *stride=image.stride;
  switch (image.pixelsize) {
    case 1: *fmt=EGG_TEX_FMT_A1; break;
    case 8: *fmt=EGG_TEX_FMT_A8; break;
    case 32: *fmt=EGG_TEX_FMT_RGBA; break;
    // Anything else will force to RGBA at decode.
    default: *stride=image.w<<2; *fmt=EGG_TEX_FMT_RGBA;
  }
}

int egg_image_decode(void *dst,int dsta,int qual,int rid) {
  const void *serial=0;
  int serialc=rom_get(&serial,&egg.rom,EGG_RESTYPE_image,qual,rid);
  struct png_image *image=png_decode(serial,serialc);
  if (!image) return -1;
  switch (image->pixelsize) {
    case 1: case 8: case 32: break; // cool
    default: { // force RGBA
        if (png_image_reformat(image,8,6)<0) {
          png_image_del(image);
          return -1;
        }
      }
  }
  int srcc=image->stride*image->h;
  if (srcc<=dsta) memcpy(dst,image->v,srcc);
  png_image_del(image);
  return srcc;
}

int egg_res_get(void *dst,int dsta,int tid,int qual,int rid) {
  const void *src=0;
  int srcc=rom_get(&src,&egg.rom,tid,qual,rid);
  if (srcc<=dsta) memcpy(dst,src,srcc);
  return srcc;
}

int egg_res_for_each(int (*cb)(int tid,int qual,int rid,int len,void *userdata),void *userdata) {
  const struct rom_res *res=egg.rom.resv;
  int i=egg.rom.resc,err;
  for (;i-->0;res++) {
    int tid=0,qual=0,rid=0;
    rom_unpack_fqrid(&tid,&qual,&rid,res->fqrid);
    if (err=cb(tid,qual,rid,res->c,userdata)) return err;
  }
  return 0;
}

void egg_audio_play_song(int qual,int rid,int force,int repeat) {
  if (egg_lock_audio()<0) return;
  synth_play_song(egg.synth,qual,rid,force,repeat);
}

void egg_audio_play_sound(int qual,int rid,double trim,double pan) {
  if (egg_lock_audio()<0) return;
  synth_play_sound(egg.synth,qual,rid,trim,pan);
}

void egg_audio_event(int chid,int opcode,int a,int b) {
  if (egg_lock_audio()<0) return;
  synth_event(egg.synth,chid,opcode,a,b,0);
}

double egg_audio_get_playhead() {
  // No need to lock.
  //TODO Adjust per driver: About how far into the last buffer are we?
  return synth_get_playhead(egg.synth);
}
void egg_audio_set_playhead(double beat) {
  if (egg_lock_audio()<0) return;
  synth_set_playhead(egg.synth,beat);
}

/* Load.
 */
 
int egg_romsrc_load() {
  if (rom_init_borrow(&egg.rom,egg_rom_bundled,egg_rom_bundled_length)<0) {
    fprintf(stderr,"%s: Failed to decode built-in ROM.\n",egg.exename);
    return -2;
  }
  // In theory, we should call egg_rom_assert_required() here. But that seems pointless for a static build.
  return 0;
}

/* Client hooks.
 */
 
void egg_romsrc_call_client_quit() {
  egg_client_quit();
}

int egg_romsrc_call_client_init() {
  return egg_client_init();
}

void egg_romsrc_call_client_update(double elapsed) {
  egg_client_update(elapsed);
}

void egg_romsrc_call_client_render() {
  egg_client_render();
}
