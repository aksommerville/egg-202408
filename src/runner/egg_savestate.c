#include "egg_runner_internal.h"
#include "opt/serial/serial.h"
#include "opt/fs/fs.h"

/* Encode state.
 */
 
static int egg_savestate_encode(struct sr_encoder *dst) {

  // Signature.
  if (sr_encode_raw(dst,"EgSv\0\0\0\0",8)<0) return -1;
  
  // Metadata for matching. Don't emit empties, and only emit "vers" if "magc" is missing.
  char tmp[256];
  int tmpc;
  if ((tmpc=egg_rom_get_metadata(tmp,sizeof(tmp),"title",5,0))>0) {
    if (tmpc>sizeof(tmp)) return -1;
    if (sr_encode_raw(dst,"titl",4)<0) return -1;
    if (sr_encode_intbe(dst,tmpc,4)<0) return -1;
    if (sr_encode_raw(dst,tmp,tmpc)<0) return -1;
  }
  if ((tmpc=egg_rom_get_metadata(tmp,sizeof(tmp),"magic",5,0))>0) {
    if (tmpc>sizeof(tmp)) return -1;
    if (sr_encode_raw(dst,"magc",4)<0) return -1;
    if (sr_encode_intbe(dst,tmpc,4)<0) return -1;
    if (sr_encode_raw(dst,tmp,tmpc)<0) return -1;
  } else if ((tmpc=egg_rom_get_metadata(tmp,sizeof(tmp),"version",7,0))>0) {
    if (tmpc>sizeof(tmp)) return -1;
    if (sr_encode_raw(dst,"vers",4)<0) return -1;
    if (sr_encode_intbe(dst,tmpc,4)<0) return -1;
    if (sr_encode_raw(dst,tmp,tmpc)<0) return -1;
  }
  
  // Synthesizer.
  int songqual,songid,songrepeat;
  synth_get_song(&songqual,&songid,&songrepeat,egg.synth);
  if (songid) {
    if (sr_encode_raw(dst,"song\0\0\0\x09",8)<0) return -1;
    if (sr_encode_intbe(dst,songqual,2)<0) return 1;
    if (sr_encode_intbe(dst,songid,2)<0) return 1;
    if (sr_encode_intbe(dst,songrepeat,1)<0) return 1;
    double playhead=synth_get_playhead(egg.synth,0.0);
    if (sr_encode_intbe(dst,(int)(playhead*65536.0),4)<0) return 1;
  }
  
  // Input state.
  if (sr_encode_raw(dst,"emsk\0\0\0\x04",8)<0) return -1;
  if (sr_encode_intbe(dst,egg_get_eventmask(),4)<0) return -1;
  int idv[256];
  int idc;
  #define LISTU32(fn,chunkid) { \
    if ((idc=fn(idv,256))>0) { \
      if (sr_encode_raw(dst,chunkid,4)<0) return -1; \
      if (sr_encode_intbe(dst,idc*4,4)<0) return -1; \
      const int *v=idv; \
      int i=idc; \
      for (;i-->0;v++) { \
        if (sr_encode_intbe(dst,*v,4)<0) return -1; \
      } \
    } \
  }
  LISTU32(egg_get_held_keys,"keys")
  LISTU32(egg_get_joy_devids,"joid")
  LISTU32(egg_get_raw_devids,"rwid")
  #undef LISTU32
  
  // Textures.
  int texp=0,texid;
  for (;(texid=render_texid_by_index(egg.render,texp))>0;texp++) {
    if (texid==1) continue; // Don't store texture one.
    int qual=0,rid=0;
    render_texture_get_origin(&qual,&rid,egg.render,texid);
    if (rid) {
      if (sr_encode_raw(dst,"txim\0\0\0\x08",8)<0) return -1;
      if (sr_encode_intbe(dst,texid,4)<0) return -1;
      if (sr_encode_intbe(dst,qual,2)<0) return -1;
      if (sr_encode_intbe(dst,rid,2)<0) return -1;
    } else {
      int w=0,h=0,fmt=0;
      void *pixels=render_texture_get_pixels(&w,&h,&fmt,egg.render,texid);
      if (!pixels) {
        fprintf(stderr,"Failed to fetch pixels for texture %d\n",texid);
        return -1;
      }
      struct png_image png={
        .v=pixels,
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
        case EGG_TEX_FMT_A1: {
            png.depth=1;
            png.colortype=0;
            png.stride=(w+7)>>3;
            png.pixelsize=1;
          } break;
        case EGG_TEX_FMT_A8: {
            png.depth=8;
            png.colortype=0;
            png.stride=w;
            png.pixelsize=8;
          } break;
        default: free(pixels); return -1;
      }
      int lenp=dst->c+4;
      if (sr_encode_raw(dst,"txrw\0\0\0\0",8)<0) {
        free(pixels);
        return -1;
      }
      if (png_encode(dst,&png)<0) {
        free(pixels);
        return -1;
      }
      free(pixels);
      int len=dst->c-lenp-4;
      uint8_t *lendst=((uint8_t*)dst->v)+lenp;
      *(lendst++)=len>>24;
      *(lendst++)=len>>16;
      *(lendst++)=len>>8;
      *(lendst++)=len;
    }
  }
  
  // Memory.
  const uint8_t *heap=0;
  int heapc=egg_get_full_heap(&heap);
  if (heapc<0) return -1;
  int heapp=0;
  while (heapp<heapc) {
    // If we're looking at nine or more zeroes, skip all the zeroes.
    if ((heapc-heapp>=9)&&!memcmp(heap+heapp,"\0\0\0\0\0\0\0\0\0",9)) {
      heapp+=9;
      while ((heapp<heapc)&&!heap[heapp]) heapp++;
      continue;
    }
    // Measure up to the first run of nine zeroes.
    int subc=0;
    while (heapp+subc<heapc) {
      if (heapp+subc>heapc-9) subc=heapc-heapp;
      else if (!memcmp(heap+heapp+subc,"\0\0\0\0\0\0\0\0\0",9)) break;
      else subc++;
    }
    // Eliminate zeroes fore and aft.
    while (subc&&!heap[heapp]) { heapp++; subc--; }
    while (subc&&!heap[heapp+subc-1]) subc--;
    // Emit a chunk.
    if (subc>0) {
      if (sr_encode_raw(dst,"mmry",4)<0) return -1;
      if (sr_encode_intbe(dst,4+subc,4)<0) return -1;
      if (sr_encode_intbe(dst,heapp,4)<0) return -1;
      if (sr_encode_raw(dst,heap+heapp,subc)<0) return -1;
      heapp+=subc;
    }
  }
  
  return 0;
}

/* Decoder context.
 */
 
struct egg_savestate_decode {
  // Single-appearance chunks:
  const uint8_t *joid; int joidc;
  const uint8_t *rwid; int rwidc;
  const uint8_t *keys; int keysc;
  const uint8_t *song; int songc;
  const uint8_t *titl; int titlc;
  const uint8_t *magc; int magcc;
  const uint8_t *vers; int versc;
  const uint8_t *emsk; int emskc;
  // "mmry" chunks:
  struct egg_savestate_mmry {
    int p,c;
    const void *v;
  } *mmryv;
  int mmryc,mmrya;
  // "txim" and "txrw" chunks:
  struct egg_savestate_tex {
    int texid;
    int qual;
    int imageid; // zero to use raw
    const void *raw;
    int rawc;
  } *texv;
  int texc,texa;
};

static void egg_savestate_decode_cleanup(struct egg_savestate_decode *ctx) {
  if (ctx->mmryv) free(ctx->mmryv);
  if (ctx->texv) free(ctx->texv);
}

static struct egg_savestate_mmry *egg_savestate_new_mmry(struct egg_savestate_decode *ctx) {
  if (ctx->mmryc>=ctx->mmrya) {
    int na=ctx->mmrya+16;
    if (na>INT_MAX/sizeof(struct egg_savestate_mmry)) return 0;
    void *nv=realloc(ctx->mmryv,sizeof(struct egg_savestate_mmry)*na);
    if (!nv) return 0;
    ctx->mmryv=nv;
    ctx->mmrya=na;
  }
  struct egg_savestate_mmry *mmry=ctx->mmryv+ctx->mmryc++;
  memset(mmry,0,sizeof(struct egg_savestate_mmry));
  return mmry;
}

static int egg_savestate_decode_mmry(struct egg_savestate_decode *ctx,const uint8_t *src,int srcc,const char *refname) {
  if (srcc<4) return -1;
  struct egg_savestate_mmry *mmry=egg_savestate_new_mmry(ctx);
  if (!mmry) return -1;
  mmry->p=(src[0]<<24)|(src[1]<<16)|(src[2]<<8)|src[3];
  if (mmry->p<0) return -1;
  mmry->c=srcc-4;
  if (mmry->p>INT_MAX-mmry->c) return -1;
  mmry->v=src+4;
  return 0;
}

static struct egg_savestate_tex *egg_savestate_new_tex(struct egg_savestate_decode *ctx) {
  if (ctx->texc>=ctx->texa) {
    int na=ctx->texa+16;
    if (na>INT_MAX/sizeof(struct egg_savestate_tex)) return 0;
    void *nv=realloc(ctx->texv,sizeof(struct egg_savestate_tex)*na);
    if (!nv) return 0;
    ctx->texv=nv;
    ctx->texa=na;
  }
  struct egg_savestate_tex *tex=ctx->texv+ctx->texc++;
  memset(tex,0,sizeof(struct egg_savestate_tex));
  return tex;
}

static int egg_savestate_decode_txim(struct egg_savestate_decode *ctx,const uint8_t *src,int srcc,const char *refname) {
  if (srcc!=8) return -1;
  struct egg_savestate_tex *tex=egg_savestate_new_tex(ctx);
  if (!tex) return -1;
  tex->texid=(src[0]<<24)|(src[1]<<16)|(src[2]<<8)|src[3];
  tex->qual=(src[4]<<8)|src[5];
  tex->imageid=(src[6]<<8)|src[7];
  return 0;
}

static int egg_savestate_decode_txrw(struct egg_savestate_decode *ctx,const uint8_t *src,int srcc,const char *refname) {
  if (srcc<4) return -1;
  struct egg_savestate_tex *tex=egg_savestate_new_tex(ctx);
  if (!tex) return -1;
  tex->texid=(src[0]<<24)|(src[1]<<16)|(src[2]<<8)|src[3];
  tex->raw=src+4;
  tex->rawc=srcc-4;
  return 0;
}

static int egg_savestate_decode_single(void *dstpp,int *dstc,const void *src,int srcc,const char *refname) {
  if (*(void**)dstpp) return -1; // Duplicate chunk that's required to be single.
  *(const void**)dstpp=src;
  *dstc=srcc;
  return 0;
}

/* Decode into manifold and do not touch or examine live state.
 */
 
static int valid_chunkid(const char *src) {
  int i=4; while (i-->0) {
    if (*src<0x21) return 0;
    if (*src>0x7e) return 0;
    src++;
  }
  return 1;
}
 
static int egg_savestate_decode(struct egg_savestate_decode *ctx,const uint8_t *src,int srcc,const char *refname) {
  // The first chunk doubles as a signature.
  if ((srcc<8)||memcmp(src,"EgSv\0\0\0\0",8)) {
    fprintf(stderr,"%s: Not a saved state file.\n",refname);
    return -2;
  }
  int srcp=8,stopp=srcc-8;
  while (srcp<=stopp) {
    const char *chunkid=(char*)(src+srcp);
    srcp+=4;
    int chunklen=(src[srcp]<<24)|(src[srcp+1]<<16)|(src[srcp+2]<<8)|src[srcp+3];
    srcp+=4;
    if (!valid_chunkid(chunkid)||(chunklen<0)||(srcp>srcc-chunklen)) {
      fprintf(stderr,"%s: Invalid chunk header around %d/%d\n",refname,srcp-8,srcc);
      return -2;
    }
    int err=-2;
    if (!memcmp(chunkid,"mmry",4)) err=egg_savestate_decode_mmry(ctx,src+srcp,chunklen,refname);
    else if (!memcmp(chunkid,"txim",4)) err=egg_savestate_decode_txim(ctx,src+srcp,chunklen,refname);
    else if (!memcmp(chunkid,"txrw",4)) err=egg_savestate_decode_txrw(ctx,src+srcp,chunklen,refname);
    #define SINGLE(tag) else if (!memcmp(chunkid,#tag,4)) err=egg_savestate_decode_single(&ctx->tag,&ctx->tag##c,src+srcp,chunklen,refname);
    SINGLE(joid)
    SINGLE(rwid)
    SINGLE(keys)
    SINGLE(song)
    SINGLE(titl)
    SINGLE(magc)
    SINGLE(vers)
    SINGLE(emsk)
    #undef SINGLE
    else fprintf(stderr,"%s: Unknown save state chunk '%.4s' around %d/%d\n",refname,chunkid,srcp-8,srcc);
    if (err<0) {
      if (err!=-2) fprintf(stderr,"%s: Unspecified error decoding '%.4s' chunk around %d/%d\n",refname,chunkid,srcp-8,srcc);
      return -2;
    }
    srcp+=chunklen;
  }
  return 0;
}

/* Validate and apply decoded state.
 */
 
static int egg_savestate_apply(const struct egg_savestate_decode *ctx,const char *refname) {
  
  // If any of "titl","magc","vers" was supplied, they must match the current game's metadata.
  #define META(cfld,mfld) if (ctx->cfld) { \
    char v[256]; \
    int vc=egg_rom_get_metadata(v,sizeof(v),mfld,sizeof(mfld)-1,0); \
    if (vc>sizeof(v)) vc=-1; \
    if ((vc!=ctx->cfld##c)||memcmp(v,ctx->cfld,vc)) { \
      fprintf(stderr,"%s: Saved state is for a different game, or different version of this one.\n",refname); \
      return -2; \
    } \
  }
  META(titl,"title")
  META(magc,"magic")
  META(vers,"version")
  #undef META
  
  // If "song" is present, it must be exactly 9 bytes.
  if (ctx->song) {
    if (ctx->songc!=9) {
      fprintf(stderr,"%s: Invalid 'song' chunk, length %d\n",refname,ctx->songc);
      return -2;
    }
  }
  
  // "joid","rwid","keys" must be a multiple of 4 if present, and "emsk" must be exactly 4.
  if (ctx->joid&&(ctx->joidc&3)) return -1;
  if (ctx->rwid&&(ctx->rwidc&3)) return -1;
  if (ctx->keys&&(ctx->keysc&3)) return -1;
  if (ctx->emsk&&(ctx->emskc!=4)) return -1;
  
  // "mmry" chunks must not exceed the existing heap.
  // Validate all before applying any.
  char *heap=0;
  int heapc=egg_get_full_heap(&heap);
  if (heapc<0) return -1;
  const struct egg_savestate_mmry *mmry=ctx->mmryv;
  int i=ctx->mmryc;
  for (;i-->0;mmry++) {
    if (mmry->p>heapc-mmry->c) {
      fprintf(stderr,"%s: Heap out of bounds.\n",refname);
      return -2;
    }
  }
  
  // --- OK we're going to apply it. --- Failures below this point will leave the runtime in an inconsistent state. ---
  
  // Video. Start with these since they are the most fallible.
  render_drop_textures(egg.render);
  const struct egg_savestate_tex *tex=ctx->texv;
  for (i=ctx->texc;i-->0;tex++) {
    if (render_texture_require(egg.render,tex->texid)<0) return -1;
    if (tex->imageid) {
      const void *serial=0;
      int serialc=rom_get(&serial,&egg.rom,EGG_RESTYPE_image,tex->qual,tex->imageid);
      if (serialc<=0) return -1;
      if (render_texture_load(egg.render,tex->texid,0,0,0,0,serial,serialc)<0) return -1;
      render_texture_set_origin(egg.render,tex->texid,tex->qual,tex->imageid);
    } else {
      if (render_texture_load(egg.render,tex->texid,0,0,0,0,tex->raw,tex->rawc)<0) return -1;
    }
  }
  
  // Memory.
  memset(heap,0,heapc);
  for (i=ctx->mmryc,mmry=ctx->mmryv;i-->0;mmry++) {
    memcpy(heap+mmry->p,mmry->v,mmry->c);
  }
  
  // Audio. Absence of a "song" chunk means "play silence", not "do nothing".
  if (ctx->song) {
    int qual=(ctx->song[0]<<8)|ctx->song[1];
    int rid=(ctx->song[2]<<8)|ctx->song[3];
    int repeat=ctx->song[4];
    double playhead=((ctx->song[5]<<24)|(ctx->song[6]<<16)|(ctx->song[7]<<8)|ctx->song[8])/65536.0;
    if (egg_lock_audio()>=0) {
      synth_play_song(egg.synth,qual,rid,1,repeat);
      synth_set_playhead(egg.synth,playhead);
      egg_unlock_audio();
    }
  } else {
    if (egg_lock_audio()>=0) {
      synth_play_song(egg.synth,0,0,0,0);
      egg_unlock_audio();
    }
  }
  
  // Input.
  if (ctx->emsk) {
    int mask=(ctx->emsk[0]<<24)|(ctx->emsk[1]<<16)|(ctx->emsk[2]<<8)|ctx->emsk[3];
    egg_force_eventmask(mask);
  }
  for (i=0;i<ctx->joidc;) {
    int devid=ctx->joid[i++]<<24;
    devid|=ctx->joid[i++]<<16;
    devid|=ctx->joid[i++]<<8;
    devid|=ctx->joid[i++];
    egg_artificial_joy_disconnect(devid);
  }
  for (i=0;i<ctx->rwidc;) {
    int devid=ctx->rwid[i++]<<24;
    devid|=ctx->rwid[i++]<<16;
    devid|=ctx->rwid[i++]<<8;
    devid|=ctx->rwid[i++];
    egg_artificial_raw_disconnect(devid);
  }
  for (i=0;i<ctx->keysc;) {
    int keycode=ctx->keys[i++]<<24;
    keycode|=ctx->keys[i++]<<16;
    keycode|=ctx->keys[i++]<<8;
    keycode|=ctx->keys[i++];
    egg_artificial_key_release(keycode);
  }
  
  return 0;
}

/* Save state, public entry point.
 */
 
int egg_savestate_save() {
  int err;
  if (!egg.config.savestatepath) {
    fprintf(stderr,"%s: Can't save state as no file was provided. Launch with '--state=PATH'.\n",egg.exename);
    return -2;
  }
  struct sr_encoder serial={0};
  if ((err=egg_savestate_encode(&serial))<0) {
    sr_encoder_cleanup(&serial);
    return err;
  }
  err=file_write(egg.config.savestatepath,serial.v,serial.c);
  sr_encoder_cleanup(&serial);
  if (err<0) {
    fprintf(stderr,"%s: Failed to save state, %d bytes.\n",egg.config.savestatepath,serial.c);
    return -2;
  }
  fprintf(stderr,"%s: Saved state, %d bytes.\n",egg.config.savestatepath,serial.c);
  return 0;
}

/* Load state, public entry point.
 */
 
int egg_savestate_load() {
  if (!egg.config.savestatepath) {
    fprintf(stderr,"%s: Can't load state as no file was provided. Launch with '--state=PATH'.\n",egg.exename);
    return -2;
  }
  void *serial=0;
  int serialc=file_read(&serial,egg.config.savestatepath);
  if (serialc<0) {
    fprintf(stderr,"%s: Failed to read save state.\n",egg.config.savestatepath);
    return -2;
  }
  struct egg_savestate_decode ctx={0};
  int err=egg_savestate_decode(&ctx,serial,serialc,egg.config.savestatepath);
  if (err<0) {
    egg_savestate_decode_cleanup(&ctx);
    free(serial);
    if (err!=-2) fprintf(stderr,"%s: Failed to load saved state from %d bytes serial.\n",egg.config.savestatepath,serialc);
    return -2;
  }
  if ((err=egg_savestate_apply(&ctx,egg.config.savestatepath))<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error applying saved state.\n",egg.config.savestatepath);
    free(serial);
    egg_savestate_decode_cleanup(&ctx);
    return -2;
  }
  free(serial);
  egg_savestate_decode_cleanup(&ctx);
  fprintf(stderr,"%s: Loaded state.\n",egg.config.savestatepath);
  return 0;
}
