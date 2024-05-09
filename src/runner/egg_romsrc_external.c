/* egg_romsrc_external.c
 * Only one of the egg_romsrc_*.c files actually get used in a given target.
 * This one is for the universal runner; ROM file must be provided at runtime.
 */

#include "egg_runner_internal.h"
#include "opt/fs/fs.h"
#include "opt/strfmt/strfmt.h"
#include "wasm_export.h"
#include <sys/time.h>

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
  //TODO egg_time_local
}

static void egg_wasm_request_termination(wasm_exec_env_t ee) {
  fprintf(stderr,"%s: Terminating due to request from game.\n",egg.exename);
  egg.terminate=1;
}

static int egg_wasm_get_user_languages(wasm_exec_env_t ee,int vp,int a) {
  return 0;//TODO egg_get_user_languages
}

static void egg_wasm_texture_del(wasm_exec_env_t ee,int texid) {
  //TODO egg_texture_del
}

static int egg_wasm_texture_new(wasm_exec_env_t ee) {
  return 0;//TODO egg_texture_new
}

static void egg_wasm_texture_get_header(wasm_exec_env_t ee,int *w,int *h,int *fmt,int texid) {
  //TODO egg_texture_get_header
}

static int egg_wasm_texture_load_image(wasm_exec_env_t ee,int texid,int qual,int rid) {
  return -1;//TODO egg_texture_load_image
}

static int egg_wasm_texture_upload(wasm_exec_env_t ee,int texid,int w,int h,int stride,int fmt,const void *v,int c) {
  return -1;//TODO egg_texture_upload
}

static void egg_wasm_texture_clear(wasm_exec_env_t ee,int texid) {
  //TODO egg_texture_clear
}

static void egg_wasm_render_tint(wasm_exec_env_t ee,uint32_t rgba) {
  //TODO egg_render_tint
}

static void egg_wasm_render_alpha(wasm_exec_env_t ee,uint8_t a) {
  //TODO egg_render_alpha
}

static void egg_wasm_draw_rect(wasm_exec_env_t ee,int dsttexid,int x,int y,int w,int h,int rgba) {
  //TODO egg_draw_rect
}

static void egg_wasm_draw_line(wasm_exec_env_t ee,int dsttexid,int vp/*const struct egg_draw_line *v*/,int c) {
  //TODO egg_draw_line
}

static void egg_wasm_draw_decal(wasm_exec_env_t ee,int dsttexid,int srctexid,int dstx,int dsty,int srcx,int srcy,int w,int h,int xform) {
  //TODO egg_draw_decal
}

static void egg_wasm_draw_tile(wasm_exec_env_t ee,int dsttexid,int srctexid,int vp/*const struct egg_draw_tile *v*/,int c) {
  //TODO egg_draw_tile
}

static void egg_wasm_image_get_header(wasm_exec_env_t ee,int *w,int *h,int *stride,int *fmt,int qual,int rid) {
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
  return 0;//TODO egg_store_get
}

static int egg_wasm_store_set(wasm_exec_env_t ee,const char *k,int kc,const char *v,int vc) {
  return -1;//TODO egg_store_set
}

static int egg_wasm_store_key_by_index(wasm_exec_env_t ee,char *dst,int dsta,int p) {
  return 0;//TODO egg_store_key_by_index
}

static int egg_wasm_event_get(wasm_exec_env_t ee,int vp/*union egg_event *v*/,int a) {
  return 0;//TODO egg_event_get
}

static int egg_wasm_event_enable(wasm_exec_env_t ee,int type,int enable) {
  return 0;//TODO egg_event_enable
}

static void egg_wasm_show_cursor(wasm_exec_env_t ee,int show) {
  //TODO egg_show_cursor
}

static int egg_wasm_lock_cursor(wasm_exec_env_t ee,int lock) {
  return 0;//TODO egg_lock_cursor
}

static int egg_wasm_joystick_devid_by_index(wasm_exec_env_t ee,int p) {
  return 0;//TODO egg_joystick_devid_by_index
}

static void egg_wasm_joystick_get_ids(wasm_exec_env_t ee,int *vid,int *pid,int *version,int devid) {
  //TODO egg_joystick_get_ids
}

static int egg_wasm_joystick_get_name(wasm_exec_env_t ee,char *dst,int dsta,int devid) {
  return 0;//TODO egg_joystick_get_name
}

static void egg_wasm_audio_play_song(wasm_exec_env_t ee,int qual,int rid,int force,int repeat) {
  //TODO egg_audio_play_song
}

static void egg_wasm_audio_play_sound(wasm_exec_env_t ee,int qual,int rid,double trim,double pan) {
  //TODO egg_audio_play_sound
}

static void egg_wasm_audio_event(wasm_exec_env_t ee,int chid,int opcode,int a,int b) {
  //TODO egg_audio_event
}

static double egg_wasm_audio_get_playhead(wasm_exec_env_t ee) {
  return 0.0;//TODO egg_audio_get_playhead
}

static void egg_wasm_audio_set_playhead(wasm_exec_env_t ee,double beat) {
  //TODO egg_audio_set_playhead
}

/* Table of exports to wasm.
 */
 
static NativeSymbol egg_wasm_exports[]={
  {"egg_log",egg_wasm_log,"($i)"},
  {"egg_time_real",egg_wasm_time_real,"()F"},
  {"egg_time_local",egg_wasm_time_local,"(ia)"},
  {"egg_request_termination",egg_wasm_request_termination,"()"},
  {"egg_get_user_languages",egg_wasm_get_user_languages,"(ia)"},
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
  {"egg_draw_decal",egg_wasm_draw_decal,"(iiiiiiiii)"},
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
  {"egg_audio_play_song",egg_wasm_audio_play_song,"(iiii)"},
  {"egg_audio_play_sound",egg_wasm_audio_play_sound,"(iiFF)"},
  {"egg_audio_event",egg_wasm_audio_event,"(iiii)"},
  {"egg_audio_get_playhead",egg_wasm_audio_get_playhead,"()F"},
  {"egg_audio_set_playhead",egg_wasm_audio_set_playhead,"(F)"},
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
