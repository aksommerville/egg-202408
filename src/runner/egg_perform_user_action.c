#include "egg_runner_internal.h"
#include "egg_inmap.h"
#include "opt/serial/serial.h"
#include "opt/fs/fs.h"
#include "GLES2/gl2.h"

/* Trivial actions.
 */
 
static void egg_ua_QUIT() {
  fprintf(stderr,"%s: Terminating due to keystroke.\n",egg.exename);
  egg.terminate=1;
}
 
static void egg_ua_FULLSCREEN() {
  hostio_toggle_fullscreen(egg.hostio);
}
 
static void egg_ua_PAUSE() {
  if (egg.hard_pause) {
    fprintf(stderr,"%s: Resume\n",egg.exename);
    egg.hard_pause=0;
  } else {
    fprintf(stderr,"%s: Hard pause\n",egg.exename);
    egg.hard_pause=1;
  }
}

/* Compose path for new screencap.
 * The directory containing it must exist, it's within our writ to create it.
 * For now at least, we're just going to use the working directory.
 * TODO The directory ought to be configurable.
 */
 
static int egg_get_screencap_path(char *dst,int dsta) {
  int t[7]={0};
  egg_time_local(t,7);
  int dstc=snprintf(dst,dsta,"screencap-%04d%02d%02d%02d%02d%02d%03d.png",t[0],t[1],t[2],t[3],t[4],t[5],t[6]);
  if ((dstc<1)||(dstc>=dsta)) return -1;
  return dstc;
}

/* Take and save a screencap.
 */
 
static void egg_ua_SCREENCAP() {
  void *rgba=0;
  int w=0,h=0,fmt=EGG_TEX_FMT_RGBA;
  if (egg.directgl&&!egg.config.configure_input) {
    w=egg.hostio->video->w;
    h=egg.hostio->video->h;
    if (!(rgba=malloc(w*h*4))) return;
    // I was having problems with pixels coming back invalid or partial. Triggering a fresh client render seems to fix it.
    if (egg.hostio->video->type->gx_begin(egg.hostio->video)<0) {
      free(rgba);
      return;
    }
    egg_romsrc_call_client_render();
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,rgba);
    egg.hostio->video->type->gx_end(egg.hostio->video);
  } else {
    if (!(rgba=render_texture_get_pixels(&w,&h,&fmt,egg.render,1))) return;
  }
  struct png_image png={
    .v=rgba,
    .w=w,
    .h=h,
  };
  switch (fmt) {
    case EGG_TEX_FMT_RGBA: {
        png.depth=8;
        png.colortype=6;
        png.stride=w<<2;
        png.pixelsize=32;
      } break;
    case EGG_TEX_FMT_A8: {
        png.depth=8;
        png.colortype=0;
        png.stride=w;
        png.pixelsize=8;
      } break;
    case EGG_TEX_FMT_A1: {
        png.depth=1;
        png.colortype=0;
        png.stride=(w+7)>>3;
        png.pixelsize=1;
      } break;
    default: free(rgba); return;
  }
  struct sr_encoder serial={0};
  int err=png_encode(&serial,&png);
  free(rgba);
  if (err<0) {
    sr_encoder_cleanup(&serial);
    return;
  }
  char path[1024];
  int pathc=egg_get_screencap_path(path,sizeof(path));
  if ((pathc<1)||(pathc>=sizeof(path))) {
    fprintf(stderr,"%s: Failed to determine output path for screencap!\n",egg.exename);
    sr_encoder_cleanup(&serial);
    return;
  }
  err=file_write(path,serial.v,serial.c);
  sr_encoder_cleanup(&serial);
  if (err<0) return;
  fprintf(stderr,"%s: Saved screencap.\n",path);
}

/* Save state.
 */
 
static void egg_ua_SAVE() {
  fprintf(stderr,"%s:%s TODO\n",__FILE__,__func__);
}

/* Load state.
 */
 
static void egg_ua_LOAD() {
  fprintf(stderr,"%s:%s TODO\n",__FILE__,__func__);
}

/* User action, dispatch.
 */
 
int egg_perform_user_action(int inmap_btnid) {
  switch (inmap_btnid) {
    #define _(tag) case EGG_INMAP_BTN_##tag: egg_ua_##tag(); return 1;
    _(QUIT)
    _(SCREENCAP)
    _(SAVE)
    _(LOAD)
    _(PAUSE)
    _(FULLSCREEN)
    #undef _
  }
  return 0;
}
