/* egg_romsrc_external.c
 * Only one of the egg_romsrc_*.c files actually get used in a given target.
 * This one is for the universal runner; ROM file must be provided at runtime.
 */

#include "egg_runner_internal.h"
#include "opt/fs/fs.h"
#include "opt/strfmt/strfmt.h"
#include "wasm_export.h"

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

/* Table of exports to wasm.
 */
 
static NativeSymbol egg_wasm_exports[]={
  {"egg_log",egg_wasm_log,"($i)"},
  //TODO
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
  int wasm1c=rom_get(&wasm1ro,&egg.rom,RESTYPE_wasm,0,1);
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
