/* egg_romsrc_external.c
 * Only one of the egg_romsrc_*.c files actually get used in a given target.
 * This one is for the universal runner; ROM file must be provided at runtime.
 */

#include "egg_runner_internal.h"
#include "opt/fs/fs.h"
#include "opt/strfmt/strfmt.h"
#include "wasm_export.h"
#include <sys/time.h>
#include <time.h>

#if EGG_BUNDLE_ROM
const int egg_romsrc=EGG_ROMSRC_BUNDLED;
extern const char egg_rom_bundled[];
extern const int egg_rom_bundled_length;
#else
const int egg_romsrc=EGG_ROMSRC_EXTERNAL;
#endif

/* egg_log: Nontrivial
 */
 
static void egg_wasm_log(wasm_exec_env_t ee,const char *fmt,int vargs) {
  if (!fmt) return;
  char tmp[256];
  int tmpc=0,panic=100;
  struct strfmt strfmt={.fmt=fmt};
  while ((tmpc<sizeof(tmp))&&!strfmt_is_finished(&strfmt)&&(panic-->0)) {
    int err=strfmt_more(tmp+tmpc,sizeof(tmp)-tmpc,&strfmt);
    if (err>0) tmpc+=err;
    switch (strfmt.awaiting) {
      case 'i': {
          int32_t *vp=wamr_validate_pointer(egg.wamr,1,vargs,4);
          vargs+=4;
          strfmt_provide_i(&strfmt,vp?*vp:0);
        } break;
      case 'l': {
          if (vargs&4) vargs+=4;
          int64_t *vp=wamr_validate_pointer(egg.wamr,1,vargs,8);
          vargs+=8;
          strfmt_provide_l(&strfmt,vp?*vp:0); 
        } break;
      case 'f': {
          if (vargs&4) vargs+=4;
          double *vp=wamr_validate_pointer(egg.wamr,1,vargs,8);
          vargs+=8;
          strfmt_provide_f(&strfmt,vp?*vp:0.0);
        } break;
      case 'p': {
          uint32_t *pointer=wamr_validate_pointer(egg.wamr,1,vargs,4);
          vargs+=4;
          const char *string=pointer?wamr_validate_pointer(egg.wamr,1,*pointer,0):0;
          strfmt_provide_p(&strfmt,string);
        } break;
    }
  }
  fprintf(stderr,"GAME: %.*s\n",tmpc,tmp);
}

/* Trivial API wrappers.
 */
 
static double egg_wasm_time_real(wasm_exec_env_t ee) {
  struct timeval tv={0};
  gettimeofday(&tv,0);
  return (double)tv.tv_sec+(double)tv.tv_usec/1000000.0;
}

static void egg_wasm_time_local(wasm_exec_env_t ee,int vp,int a) {
  if (a<1) return;
  if (a>7) a=7;
  int *dst=wamr_validate_pointer(egg.wamr,1,vp,a*sizeof(int));
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

static void egg_wasm_request_termination(wasm_exec_env_t ee) {
  fprintf(stderr,"%s: Terminating due to request from game.\n",egg.exename);
  egg.terminate=1;
}

static int egg_wasm_get_user_languages(wasm_exec_env_t ee,int vp,int a) {
  int tmp[16];
  int tmpc=egg_get_user_languages(tmp,16);
  if (tmpc>16) tmpc=16;
  int cpc=(tmpc<a)?tmpc:a;
  void *dst=wamr_validate_pointer(egg.wamr,1,vp,sizeof(int)*cpc);
  if (!dst) return 0;
  memcpy(dst,tmp,sizeof(int)*cpc);
  return tmpc;
}

static void egg_wasm_video_get_size(wasm_exec_env_t ee,int *w,int *h) {
  *w=egg.hostio->video->w;
  *h=egg.hostio->video->h;
}

static void egg_wasm_texture_del(wasm_exec_env_t ee,int texid) {
  render_texture_del(egg.render,texid);
}

static int egg_wasm_texture_new(wasm_exec_env_t ee) {
  return render_texture_new(egg.render);
}

static void egg_wasm_texture_get_header(wasm_exec_env_t ee,int *w,int *h,int *fmt,int texid) {
  render_texture_get_header(w,h,fmt,egg.render,texid);
}

static int egg_wasm_texture_load_image(wasm_exec_env_t ee,int texid,int qual,int rid) {
  const void *serial=0;
  int serialc=rom_get(&serial,&egg.rom,EGG_RESTYPE_image,qual,rid);
  if (serialc<=0) return -1;
  return render_texture_load(egg.render,texid,0,0,0,0,serial,serialc);
}

static int egg_wasm_texture_upload(wasm_exec_env_t ee,int texid,int w,int h,int stride,int fmt,const void *v,int c) {
  return render_texture_load(egg.render,texid,w,h,stride,fmt,v,c);
}

static void egg_wasm_texture_clear(wasm_exec_env_t ee,int texid) {
  render_texture_clear(egg.render,texid);
}

static void egg_wasm_render_tint(wasm_exec_env_t ee,uint32_t rgba) {
  render_tint(egg.render,rgba);
}

static void egg_wasm_render_alpha(wasm_exec_env_t ee,uint8_t a) {
  render_alpha(egg.render,a);
}

static void egg_wasm_draw_rect(wasm_exec_env_t ee,int dsttexid,int x,int y,int w,int h,int rgba) {
  render_draw_rect(egg.render,dsttexid,x,y,w,h,rgba);
}

static void egg_wasm_draw_line(wasm_exec_env_t ee,int dsttexid,int vp,int c) {
  if (c<1) return;
  const struct egg_draw_line *v=wamr_validate_pointer(egg.wamr,1,vp,sizeof(struct egg_draw_line)*c);
  if (!v) return;
  render_draw_line(egg.render,dsttexid,v,c);
}

static void egg_wasm_draw_trig(wasm_exec_env_t ee,int dsttexid,int vp,int c) {
  if (c<1) return;
  const struct egg_draw_line *v=wamr_validate_pointer(egg.wamr,1,vp,sizeof(struct egg_draw_line)*c);
  if (!v) return;
  render_draw_trig(egg.render,dsttexid,v,c);
}

static void egg_wasm_draw_decal(wasm_exec_env_t ee,int dsttexid,int srctexid,int dstx,int dsty,int srcx,int srcy,int w,int h,int xform) {
  render_draw_decal(egg.render,dsttexid,srctexid,dstx,dsty,srcx,srcy,w,h,xform);
}

static void egg_wasm_draw_decal_mode7(wasm_exec_env_t ee,int dsttexid,int srctexid,int dstx,int dsty,int srcx,int srcy,int w,int h,int r,int xs,int ys) {
  render_draw_decal_mode7(egg.render,dsttexid,srctexid,dstx,dsty,srcx,srcy,w,h,r/65536.0,xs/65536.0,ys/65536.0);
}

static void egg_wasm_draw_tile(wasm_exec_env_t ee,int dsttexid,int srctexid,int vp,int c) {
  if (c<1) return;
  const struct egg_draw_tile *v=wamr_validate_pointer(egg.wamr,1,vp,sizeof(struct egg_draw_tile)*c);
  if (!v) return;
  render_draw_tile(egg.render,dsttexid,srctexid,v,c);
}

static void egg_wasm_image_get_header(wasm_exec_env_t ee,int *w,int *h,int *stride,int *fmt,int qual,int rid) {
  const void *serial=0;
  int serialc=rom_get(&serial,&egg.rom,EGG_RESTYPE_image,qual,rid);
  struct png_image image={0};
  if (png_decode_header(&image,serial,serialc)<0) return;
  if (w) *w=image.w;
  if (h) *h=image.h;
  if (stride) *stride=image.stride;
  switch (image.pixelsize) {
    case 1: *fmt=EGG_TEX_FMT_A1; break;
    case 8: *fmt=EGG_TEX_FMT_A8; break;
    case 32: *fmt=EGG_TEX_FMT_RGBA; break;
    // Anything else will force to RGBA at decode.
    default: *stride=image.w<<2; *fmt=EGG_TEX_FMT_RGBA;
  }
}

static int egg_wasm_image_decode(wasm_exec_env_t ee,void *dst,int dsta,int qual,int rid) {
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

static int egg_wasm_res_get(wasm_exec_env_t ee,void *dst,int dsta,int tid,int qual,int rid) {
  const void *src=0;
  int srcc=rom_get(&src,&egg.rom,tid,qual,rid);
  if (srcc<=dsta) memcpy(dst,src,srcc);
  return srcc;
}

static int egg_wasm_res_for_each(wasm_exec_env_t ee,int cbid,int userdata) {
  //int (*cb)(int tid,int qual,int rid,int len,void *userdata)
  uint32_t argv[5]={0,0,0,0,userdata};
  const struct rom_res *res=egg.rom.resv;
  int i=egg.rom.resc;
  for (;i-->0;res++) {
    rom_unpack_fqrid(argv+0,argv+1,argv+2,res->fqrid);
    argv[3]=res->c;
    if (wamr_call_table(egg.wamr,cbid,argv,5)<0) return -1;
    if (argv[0]) return argv[0];
  }
  return 0;
}

static int egg_wasm_store_get(wasm_exec_env_t ee,char *dst,int dsta,const char *k,int kc) {
  return egg_store_get(dst,dsta,k,kc);
}

static int egg_wasm_store_set(wasm_exec_env_t ee,const char *k,int kc,const char *v,int vc) {
  return egg_store_set(k,kc,v,vc);
}

static int egg_wasm_store_key_by_index(wasm_exec_env_t ee,char *dst,int dsta,int p) {
  return egg_store_key_by_index(dst,dsta,p);
}

static int egg_wasm_event_get(wasm_exec_env_t ee,int vp/*union egg_event *v*/,int a) {
  union egg_event *v=wamr_validate_pointer(egg.wamr,1,vp,sizeof(union egg_event)*a);
  if (!v) return 0;
  return egg_event_get(v,a);
}

static int egg_wasm_event_enable(wasm_exec_env_t ee,int type,int enable) {
  return egg_event_enable(type,enable);
}

static void egg_wasm_show_cursor(wasm_exec_env_t ee,int show) {
  egg_show_cursor(show);
}

static int egg_wasm_lock_cursor(wasm_exec_env_t ee,int lock) {
  return egg_lock_cursor(lock);
}

static int egg_wasm_joystick_devid_by_index(wasm_exec_env_t ee,int p) {
  return egg_joystick_devid_by_index(p);
}

static void egg_wasm_joystick_get_ids(wasm_exec_env_t ee,int *vid,int *pid,int *version,int devid) {
  egg_joystick_get_ids(vid,pid,version,devid);
}

static int egg_wasm_joystick_get_name(wasm_exec_env_t ee,char *dst,int dsta,int devid) {
  return egg_joystick_get_name(dst,dsta,devid);
}

struct egg_joystick_for_each_button_ctx {
  int cbid;
  int userdata;
};

static int egg_wasm_cb_joystick_for_each_button(int btnid,int usage,int lo,int hi,int value,void *userdata) {
  struct egg_joystick_for_each_button_ctx *ctx=userdata;
  uint32_t argv[6]={btnid,usage,lo,hi,value,ctx->userdata};
  if (wamr_call_table(egg.wamr,ctx->cbid,argv,6)<0) return -1;
  return argv[0];
}

static int egg_wasm_joystick_for_each_button(wasm_exec_env_t ee,int devid,int cbid,int userdata) {
  // int (*cb)(int btnid,int usage,int lo,int hi,int value,void *userdata)
  struct egg_joystick_for_each_button_ctx ctx={
    .cbid=cbid,
    .userdata=userdata,
  };
  return egg_joystick_for_each_button(devid,egg_wasm_cb_joystick_for_each_button,&ctx);
}

static void egg_wasm_audio_play_song(wasm_exec_env_t ee,int qual,int rid,int force,int repeat) {
  if (egg_lock_audio()<0) return;
  synth_play_song(egg.synth,qual,rid,force,repeat);
}

static void egg_wasm_audio_play_sound(wasm_exec_env_t ee,int qual,int rid,int trim,int pan) {
  if (egg_lock_audio()<0) return;
  synth_play_sound(egg.synth,qual,rid,trim/65536.0,pan/65536.0);
}

static void egg_wasm_audio_event(wasm_exec_env_t ee,int chid,int opcode,int a,int b) {
  if (egg_lock_audio()<0) return;
  synth_event(egg.synth,chid,opcode,a,b,0);
}

static double egg_wasm_audio_get_playhead(wasm_exec_env_t ee) {
  // No need to lock.
  //TODO Adjust per driver: About how far into the last buffer are we?
  return synth_get_playhead(egg.synth);
}

static void egg_wasm_audio_set_playhead(wasm_exec_env_t ee,double beat) {
  if (egg_lock_audio()<0) return;
  synth_set_playhead(egg.synth,beat);
}

/* Table of exports to wasm.
 */
 
#include "egg_gles2_wrapper.xc"

static NativeSymbol egg_wasm_exports[]={

  // Egg.
  {"egg_log",egg_wasm_log,"($i)"},
  {"egg_time_real",egg_wasm_time_real,"()F"},
  {"egg_time_local",egg_wasm_time_local,"(ii)"},
  {"egg_request_termination",egg_wasm_request_termination,"()"},
  {"egg_get_user_languages",egg_wasm_get_user_languages,"(ii)i"},
  {"egg_video_set_string_buffer",egg_wasm_video_set_string_buffer,"(*~)"},// lives in egg_gles2_wrapper.xc
  {"egg_video_get_size",egg_wasm_video_get_size,"(**)"},
  {"egg_texture_del",egg_wasm_texture_del,"(i)"},
  {"egg_texture_new",egg_wasm_texture_new,"()i"},
  {"egg_texture_get_header",egg_wasm_texture_get_header,"(***i)"},
  {"egg_texture_load_image",egg_wasm_texture_load_image,"(iii)i"},
  {"egg_texture_upload",egg_wasm_texture_upload,"(iiiii*~)i"},
  {"egg_texture_clear",egg_wasm_texture_clear,"(i)"},
  {"egg_render_tint",egg_wasm_render_tint,"(i)"},
  {"egg_render_alpha",egg_wasm_render_alpha,"(i)"},
  {"egg_draw_rect",egg_wasm_draw_rect,"(iiiiii)"},
  {"egg_draw_line",egg_wasm_draw_line,"(iii)"},
  {"egg_draw_trig",egg_wasm_draw_trig,"(iii)"},
  {"egg_draw_decal",egg_wasm_draw_decal,"(iiiiiiiii)"},
  {"egg_draw_decal_mode7",egg_wasm_draw_decal_mode7,"(iiiiiiiiiii)"},
  {"egg_draw_tile",egg_wasm_draw_tile,"(iiii)"},
  {"egg_image_get_header",egg_wasm_image_get_header,"(****ii)"},
  {"egg_image_decode",egg_wasm_image_decode,"(*~ii)i"},
  {"egg_res_get",egg_wasm_res_get,"(*~iii)i"},
  {"egg_res_for_each",egg_wasm_res_for_each,"(ii)i"},
  {"egg_store_get",egg_wasm_store_get,"(*~*~)i"},
  {"egg_store_set",egg_wasm_store_set,"(*~*~)i"},
  {"egg_store_key_by_index",egg_wasm_store_key_by_index,"(*~i)i"},
  {"egg_event_get",egg_wasm_event_get,"(ii)i"},
  {"egg_event_enable",egg_wasm_event_enable,"(ii)i"},
  {"egg_show_cursor",egg_wasm_show_cursor,"(i)"},
  {"egg_lock_cursor",egg_wasm_lock_cursor,"(i)i"},
  {"egg_joystick_devid_by_index",egg_wasm_joystick_devid_by_index,"(i)i"},
  {"egg_joystick_get_ids",egg_wasm_joystick_get_ids,"(***i)"},
  {"egg_joystick_get_name",egg_wasm_joystick_get_name,"(*~i)i"},
  {"egg_joystick_for_each_button",egg_wasm_joystick_for_each_button,"(iii)i"},
  {"egg_audio_play_song",egg_wasm_audio_play_song,"(iiii)"},
  {"egg_audio_play_sound",egg_wasm_audio_play_sound,"(iiii)"},
  {"egg_audio_event",egg_wasm_audio_event,"(iiii)"},
  {"egg_audio_get_playhead",egg_wasm_audio_get_playhead,"()F"},
  {"egg_audio_set_playhead",egg_wasm_audio_set_playhead,"(F)"},
  
  // OpenGL ES 2.
  {"glActiveTexture",glActiveTexture_wasm,"(i)"},
  {"glAttachShader",glAttachShader_wasm,"(ii)"},
  {"glBindAttribLocation",glBindAttribLocation_wasm,"(ii$)"},
  {"glBindBuffer",glBindBuffer_wasm,"(ii)"},
  {"glBindFramebuffer",glBindFramebuffer_wasm,"(ii)"},
  {"glBindRenderbuffer",glBindRenderbuffer_wasm,"(ii)"},
  {"glBindTexture",glBindTexture_wasm,"(ii)"},
  {"glBlendColor",glBlendColor_wasm,"(ffff)"},
  {"glBlendEquation",glBlendEquation_wasm,"(i)"},
  {"glBlendEquationSeparate",glBlendEquationSeparate_wasm,"(ii)"},
  {"glBlendFunc",glBlendFunc_wasm,"(ii)"},
  {"glBlendFuncSeparate",glBlendFuncSeparate_wasm,"(iiii)"},
  {"glBufferData",glBufferData_wasm,"(iiii)"}, // [1]=length [2]=pointer
  {"glBufferSubData",glBufferSubData_wasm,"(iiii)"}, // [2]=length [3]=pointer
  {"glCheckFramebufferStatus",glCheckFramebufferStatus_wasm,"(i)i"},
  {"glClear",glClear_wasm,"(i)"},
  {"glClearColor",glClearColor_wasm,"(ffff)"},
  {"glClearDepthf",glClearDepthf_wasm,"(f)"},
  {"glClearStencil",glClearStencil_wasm,"(i)"},
  {"glColorMask",glColorMask_wasm,"(iiii)"},
  {"glCompileShader",glCompileShader_wasm,"(i)"},
  {"glCompressedTexImage2D",glCompressedTexImage2D_wasm,"(iiiiiiii)"}, // [6]=length [7]=pointer
  {"glCompressedTexSubImage2D",glCompressedTexSubImage2D_wasm,"(iiiiiiiii)"}, // [7]=length [8]=pointer
  {"glCopyTexImage2D",glCopyTexImage2D_wasm,"(iiiiiiii)"},
  {"glCopyTexSubImage2D",glCopyTexSubImage2D_wasm,"(iiiiiiii)"},
  {"glCreateProgram",glCreateProgram_wasm,"()i"},
  {"glCreateShader",glCreateShader_wasm,"(i)i"},
  {"glCullFace",glCullFace_wasm,"(i)"},
  {"glDeleteBuffers",glDeleteBuffers_wasm,"(ii)"},
  {"glDeleteFramebuffers",glDeleteFramebuffers_wasm,"(ii)"},
  {"glDeleteProgram",glDeleteProgram_wasm,"(i)"},
  {"glDeleteRenderbuffers",glDeleteRenderbuffers_wasm,"(ii)"},
  {"glDeleteShader",glDeleteShader_wasm,"(i)"},
  {"glDeleteTextures",glDeleteTextures_wasm,"(ii)"},
  {"glDepthFunc",glDepthFunc_wasm,"(i)"},
  {"glDepthMask",glDepthMask_wasm,"(i)"},
  {"glDepthRangef",glDepthRangef_wasm,"(ff)"},
  {"glDetachShader",glDetachShader_wasm,"(ii)"},
  {"glDisable",glDisable_wasm,"(i)"},
  {"glDisableVertexAttribArray",glDisableVertexAttribArray_wasm,"(i)"},
  {"glDrawArrays",glDrawArrays_wasm,"(iii)"},
  {"glDrawElements",glDrawElements_wasm,"(iiii)"},
  {"glEnable",glEnable_wasm,"(i)"},
  {"glEnableVertexAttribArray",glEnableVertexAttribArray_wasm,"(i)"},
  {"glFinish",glFinish_wasm,"()"},
  {"glFlush",glFlush_wasm,"()"},
  {"glFramebufferRenderbuffer",glFramebufferRenderbuffer_wasm,"(iiii)"},
  {"glFramebufferTexture2D",glFramebufferTexture2D_wasm,"(iiiii)"},
  {"glFrontFace",glFrontFace_wasm,"(i)"},
  {"glGenBuffers",glGenBuffers_wasm,"(ii)"},
  {"glGenerateMipmap",glGenerateMipmap_wasm,"(i)"},
  {"glGenFramebuffers",glGenFramebuffers_wasm,"(ii)"},
  {"glGenRenderbuffers",glGenRenderbuffers_wasm,"(ii)"},
  {"glGenTextures",glGenTextures_wasm,"(ii)"},
  {"glGetActiveAttrib",glGetActiveAttrib_wasm,"(iiiiiii)"},
  {"glGetActiveUniform",glGetActiveUniform_wasm,"(iiiiiii)"},
  {"glGetAttachedShaders",glGetAttachedShaders_wasm,"(iiii)"},
  {"glGetAttribLocation",glGetAttribLocation_wasm,"(i$)i"},
  {"glGetBooleanv",glGetBooleanv_wasm,"(ii)"},
  {"glGetBufferParameteriv",glGetBufferParameteriv_wasm,"(iii)"},
  {"glGetError",glGetError_wasm,"()i"},
  {"glGetFloatv",glGetFloatv_wasm,"(ii)"},
  {"glGetFramebufferAttachmentParameteriv",glGetFramebufferAttachmentParameteriv_wasm,"(iiii)"},
  {"glGetIntegerv",glGetIntegerv_wasm,"(ii)"},
  {"glGetProgramiv",glGetProgramiv_wasm,"(iii)"},
  {"glGetProgramInfoLog",glGetProgramInfoLog_wasm,"(iiii)"},
  {"glGetRenderbufferParameteriv",glGetRenderbufferParameteriv_wasm,"(iii)"},
  {"glGetShaderiv",glGetShaderiv_wasm,"(iii)"},
  {"glGetShaderInfoLog",glGetShaderInfoLog_wasm,"(iiii)"},
  {"glGetShaderPrecisionFormat",glGetShaderPrecisionFormat_wasm,"(iiii)"},
  {"glGetShaderSource",glGetShaderSource_wasm,"(iiii)"},
  {"glGetString",glGetString_wasm,"(i)i"},
  {"glGetTexParameterfv",glGetTexParameterfv_wasm,"(iii)"},
  {"glGetTexParameteriv",glGetTexParameteriv_wasm,"(iii)"},
  {"glGetUniformfv",glGetUniformfv_wasm,"(iii)"},
  {"glGetUniformiv",glGetUniformiv_wasm,"(iii)"},
  {"glGetUniformLocation",glGetUniformLocation_wasm,"(i$)i"},
  {"glGetVertexAttribfv",glGetVertexAttribfv_wasm,"(iii)"},
  {"glGetVertexAttribiv",glGetVertexAttribiv_wasm,"(iii)"},
  {"glGetVertexAttribPointerv",glGetVertexAttribPointerv_wasm,"(iii)"},
  {"glHint",glHint_wasm,"(ii)"},
  {"glIsBuffer",glIsBuffer_wasm,"(i)i"},
  {"glIsEnabled",glIsEnabled_wasm,"(i)i"},
  {"glIsFramebuffer",glIsFramebuffer_wasm,"(i)i"},
  {"glIsProgram",glIsProgram_wasm,"(i)i"},
  {"glIsRenderbuffer",glIsRenderbuffer_wasm,"(i)i"},
  {"glIsShader",glIsShader_wasm,"(i)i"},
  {"glIsTexture",glIsTexture_wasm,"(i)i"},
  {"glLineWidth",glLineWidth_wasm,"(f)"},
  {"glLinkProgram",glLinkProgram_wasm,"(i)"},
  {"glPixelStorei",glPixelStorei_wasm,"(ii)"},
  {"glPolygonOffset",glPolygonOffset_wasm,"(ff)"},
  {"glReadPixels",glReadPixels_wasm,"(iiiiiii)"},
  {"glReleaseShaderCompiler",glReleaseShaderCompiler_wasm,"()"},
  {"glRenderbufferStorage",glRenderbufferStorage_wasm,"(iiii)"},
  {"glSampleCoverage",glSampleCoverage_wasm,"(fi)"},
  {"glScissor",glScissor_wasm,"(iiii)"},
  {"glShaderBinary",glShaderBinary_wasm,"(iii*~)"},
  {"glShaderSource",glShaderSource_wasm,"(iiii)"},
  {"glStencilFunc",glStencilFunc_wasm,"(iii)"},
  {"glStencilFuncSeparate",glStencilFuncSeparate_wasm,"(iiii)"},
  {"glStencilMask",glStencilMask_wasm,"(i)"},
  {"glStencilMaskSeparate",glStencilMaskSeparate_wasm,"(ii)"},
  {"glStencilOp",glStencilOp_wasm,"(iii)"},
  {"glStencilOpSeparate",glStencilOpSeparate_wasm,"(iiii)"},
  {"glTexImage2D",glTexImage2D_wasm,"(iiiiiiiii)"},
  {"glTexParameterf",glTexParameterf_wasm,"(iif)"},
  {"glTexParameterfv",glTexParameterfv_wasm,"(iii)"},
  {"glTexParameteri",glTexParameteri_wasm,"(iii)"},
  {"glTexParameteriv",glTexParameteriv_wasm,"(iii)"},
  {"glTexSubImage2D",glTexSubImage2D_wasm,"(iiiiiiiii)"},
  {"glUniform1f",glUniform1f_wasm,"(if)"},
  {"glUniform1fv",glUniform1fv_wasm,"(iii)"},
  {"glUniform1i",glUniform1i_wasm,"(ii)"},
  {"glUniform1iv",glUniform1iv_wasm,"(iii)"},
  {"glUniform2f",glUniform2f_wasm,"(iff)"},
  {"glUniform2fv",glUniform2fv_wasm,"(iii)"},
  {"glUniform2i",glUniform2i_wasm,"(iii)"},
  {"glUniform2iv",glUniform2iv_wasm,"(iii)"},
  {"glUniform3f",glUniform3f_wasm,"(ifff)"},
  {"glUniform3fv",glUniform3fv_wasm,"(iii)"},
  {"glUniform3i",glUniform3i_wasm,"(iiii)"},
  {"glUniform3iv",glUniform3iv_wasm,"(iii)"},
  {"glUniform4f",glUniform4f_wasm,"(iffff)"},
  {"glUniform4fv",glUniform4fv_wasm,"(iii)"},
  {"glUniform4i",glUniform4i_wasm,"(iiiii)"},
  {"glUniform4iv",glUniform4iv_wasm,"(iii)"},
  {"glUniformMatrix2fv",glUniformMatrix2fv_wasm,"(iiii)"},
  {"glUniformMatrix3fv",glUniformMatrix3fv_wasm,"(iiii)"},
  {"glUniformMatrix4fv",glUniformMatrix4fv_wasm,"(iiii)"},
  {"glUseProgram",glUseProgram_wasm,"(i)"},
  {"glValidateProgram",glValidateProgram_wasm,"(i)"},
  {"glVertexAttrib1f",glVertexAttrib1f_wasm,"(if)"},
  {"glVertexAttrib1fv",glVertexAttrib1fv_wasm,"(ii)"},
  {"glVertexAttrib2f",glVertexAttrib2f_wasm,"(iff)"},
  {"glVertexAttrib2fv",glVertexAttrib2fv_wasm,"(ii)"},
  {"glVertexAttrib3f",glVertexAttrib3f_wasm,"(ifff)"},
  {"glVertexAttrib3fv",glVertexAttrib3fv_wasm,"(ii)"},
  {"glVertexAttrib4f",glVertexAttrib4f_wasm,"(iffff)"},
  {"glVertexAttrib4fv",glVertexAttrib4fv_wasm,"(ii)"},
  {"glVertexAttribPointer",glVertexAttribPointer_wasm,"(iiiiii)"},
  {"glViewport",glViewport_wasm,"(iiii)"},
};

/* Load.
 */
 
int egg_romsrc_load() {
  int err;
  
  #if EGG_BUNDLE_ROM
    if (rom_init_borrow(&egg.rom,egg_rom_bundled,egg_rom_bundled_length)<0) {
      fprintf(stderr,"%s: Failed to decode built-in ROM.\n",egg.exename);
      return -2;
    }
  #else
    if (!egg.config.rompath) {
      fprintf(stderr,"%s: Please specify a ROM file.\n",egg.exename);
      return -2;
    }
    void *serial=0;
    int serialc=file_read(&serial,egg.config.rompath);
    if (serialc<0) {
      fprintf(stderr,"%s: Failed to read file.\n",egg.config.rompath);
      return -2;
    }
    if (rom_init_handoff(&egg.rom,serial,serialc)<0) {
      fprintf(stderr,"%s: Invalid ROM file.\n",egg.config.rompath);
      free(serial);
      return -2;
    }
  #endif
  
  if ((err=egg_rom_assert_required())<0) return err;
  
  const void *wasm1ro=0;
  int wasm1c=rom_get(&wasm1ro,&egg.rom,EGG_RESTYPE_wasm,0,1);
  if (wasm1c<1) {
    fprintf(stderr,"%s: ROM file does not contain any code.\n",egg.config.rompath);
    return -2;
  }
  void *wasm1=malloc(wasm1c); // wasm_micro_runtime actually rewrites something live in memory. why would it do that
  if (!wasm1) return -1;
  memcpy(wasm1,wasm1ro,wasm1c);
  if (!(egg.wamr=wamr_new())) return -1;
  if (wamr_set_exports(egg.wamr,
    egg_wasm_exports,
    sizeof(egg_wasm_exports)/sizeof(egg_wasm_exports[0])
  )<0) return -1;
  if ((err=wamr_add_module(egg.wamr,1,wasm1,wasm1c,"wasm:0:1"))<0) {
    fprintf(stderr,"%s: Failed to load game's WebAssembly module.\n",egg.config.rompath);
    return -2;
  }
  #define IMPORT(fnid,name) if (wamr_link_function(egg.wamr,1,fnid,name)<0) { \
    fprintf(stderr,"%s: ROM does not implement '%s'\n",egg.config.rompath,name); \
    return -2; \
  }
  IMPORT(1,"egg_client_quit")
  IMPORT(2,"egg_client_init")
  IMPORT(3,"egg_client_update")
  IMPORT(4,"egg_client_render")
  #undef IMPORT
  return 0;
}

/* Call client functions.
 */
 
static int romsrc_called=0;

void egg_romsrc_call_client_quit() {
  if (romsrc_called&1) return;
  romsrc_called|=1;
  uint32_t argv[1]={0};
  wamr_call(egg.wamr,1,1,argv,0);
}

int egg_romsrc_call_client_init() {
  if (romsrc_called&2) return -1;
  romsrc_called|=2;
  uint32_t argv[1]={0};
  wamr_call(egg.wamr,1,2,argv,0);
  return argv[0];
}

void egg_romsrc_call_client_update(double elapsed) {
  uint32_t argv[2]={0};
  *(double*)argv=elapsed;
  wamr_call(egg.wamr,1,3,argv,2);
}

void egg_romsrc_call_client_render() {
  uint32_t argv[1]={0};
  wamr_call(egg.wamr,1,4,argv,0);
}
