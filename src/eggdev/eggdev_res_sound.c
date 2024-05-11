#include "eggdev_internal.h"
#include "opt/sfg/sfg.h"

#define EGGDEV_HINT_PCMPRINT 1 /* For precompiled pcmprint or sfg (compiled at expansion). */

/* Slice sounds.
 */
 
struct eggrom_sound_ctx_sfg {
  struct romw *romw;
  uint16_t qual;
};
 
static int eggrom_sound_expand_cb_sfg(
  const char *src,int srcc,
  const char *id,int idc,int idn,
  const char *refname,int lineno0,
  void *userdata
) {
  struct eggrom_sound_ctx_sfg *ctx=userdata;
  struct romw_res *res=romw_res_add(ctx->romw);
  if (!res) return -1;
  res->tid=EGG_RESTYPE_sound;
  res->qual=ctx->qual;
  res->rid=idn;
  romw_res_set_path(res,refname,-1);
  romw_res_set_name(res,id,idc);
  res->hint=EGGDEV_HINT_PCMPRINT;
  
  struct sr_encoder bin={0};
  int err=sfg_compile(&bin,src,srcc,refname,lineno0);
  if (err<0) {
    if (err!=-2) fprintf(stderr,"%s:%d: Unspecified error compiling sfg sound.\n",refname,lineno0);
    return -2;
  }
  romw_res_handoff_serial(res,bin.v,bin.c);
  
  return 0;
}
 
int eggdev_sounds_slice(struct romw *romw,const char *src,int srcc,const struct eggdev_rpath *rpath) {

  // A sound file with no ID should be a multi-resource pcmprint text file.
  // Nevertheless, do a cursory signature check to make sure we don't try to parse WAV files as pcmprint.
  // Shouldn't do any harm if we did, just would lead to confusing error messages.
  if ((srcc>=12)&&!memcmp(src,"RIFF",4)&&!memcmp(src+8,"WAVE",4)) return 0;
  if ((srcc>=2)&&!memcmp(src,"\xeb\xeb",2)) return 0;
  
  // Split multi-resource SFG files.
  struct eggrom_sound_ctx_sfg sfgctx={
    .romw=romw,
    .qual=rpath->qual,
  };
  if (sfg_split(src,srcc,rpath->path,eggrom_sound_expand_cb_sfg,&sfgctx)>=0) return 1;
  
  return -1;
}

/* Compile one sound.
 */
 
int eggdev_sound_compile(struct romw *romw,struct romw_res *res) {

  // SFG binary?
  if (res->hint==EGGDEV_HINT_PCMPRINT) return 0;
  if ((res->serialc>=2)&&!memcmp(res->serial,"\xeb\xeb",2)) return 0;
  
  // WAV?
  if ((res->serialc>=12)&&!memcmp(res->serial,"RIFF",4)&&!memcmp((char*)res->serial+8,"WAVE",4)) return 0;
  
  // Single SFG text sound?
  struct sr_encoder dst={0};
  if (sfg_compile(&dst,res->serial,res->serialc,res->path,res->lineno0)>=0) {
    romw_res_handoff_serial(res,dst.v,dst.c);
    return 0;
  }
  sr_encoder_cleanup(&dst);

  fprintf(stderr,"%s: Sound format unknown.\n",res->path);
  return -2;
}
