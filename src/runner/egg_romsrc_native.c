/* egg_romsrc_native.c
 * Only one of the egg_romsrc_*.c files actually get used in a given target.
 * This one uses a bundled ROM file for assets, and native functions for code.
 */

#include "egg_runner_internal.h"
#include <stdarg.h>

const int egg_romsrc=EGG_ROMSRC_NATIVE;

extern const char egg_rom_bundled[];
extern const int egg_rom_bundled_length;

//XXX TEMP stubs for public API
double egg_time_real() { return 0.0; }
void egg_time_local(int *v,int a) {}
void egg_request_termination() {}
int egg_get_user_languages(int *v,int a) { return 0; }
void egg_texture_del(int texid) {}
int egg_texture_new() { return 0; }
void egg_texture_get_header(int *w,int *h,int *fmt,int texid) {}
int egg_texture_load_image(int texid,int qual,int rid) { return -1; }
int egg_texture_upload(int texid,int w,int h,int stride,int fmt,const void *v,int c) { return -1; }
void egg_texture_clear(int texid) {}
void egg_render_tint(uint32_t rgba) {}
void egg_render_alpha(uint8_t a) {}
void egg_draw_rect(int dsttexid,int x,int y,int w,int h,int rgba) {}
void egg_draw_line(int dsttexid,const struct egg_draw_line *v,int c) {}
void egg_draw_decal(int dsttexid,int srctexid,int dstx,int dsty,int srcx,int srcy,int w,int h,int xform) {}
void egg_draw_tile(int dsttexid,int srctexid,const struct egg_draw_tile *v,int c) {}
void egg_image_get_header(int *w,int *h,int *stride,int *fmt,int qual,int rid) {}
int egg_image_decode(void *dst,int dsta,int qual,int rid) {}
int egg_res_get(void *dst,int dsta,int tid,int qual,int rid) {}
int egg_res_for_each(int (*cb)(int tid,int qual,int rid,int len,void *userdata),void *userdata) { return 0; }
int egg_store_get(char *dst,int dsta,const char *k,int kc) { return 0; }
int egg_store_set(const char *k,int kc,const char *v,int vc) { return -1; }
int egg_store_key_by_index(char *dst,int dsta,int p) { return 0; }
int egg_event_get(union egg_event *v,int a) { return 0; }
int egg_event_enable(int type,int enable) { return 0; }
void egg_show_cursor(int show) {}
int egg_lock_cursor(int lock) { return 0; }
int egg_joystick_devid_by_index(int p) { return 0; }
void egg_joystick_get_ids(int *vid,int *pid,int *version,int devid) {}
int egg_joystick_get_name(char *dst,int dsta,int devid) { return 0; }
void egg_audio_play_song(int qual,int rid,int force,int repeat) {}
void egg_audio_play_sound(int qual,int rid,double trim,double pan) {}
void egg_audio_event(int chid,int opcode,int a,int b) {}
double egg_audio_get_playhead() { return 0.0; }
void egg_audio_set_playhead(double beat) {}

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

/* Load.
 */
 
int egg_romsrc_load() {
  if (rom_init_borrow(&egg.rom,egg_rom_bundled,egg_rom_bundled_length)<0) {
    fprintf(stderr,"%s: Failed to decode built-in ROM.\n",egg.exename);
    return -2;
  }
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
