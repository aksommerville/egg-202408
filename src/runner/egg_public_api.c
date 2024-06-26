/* egg_public_api.c
 * This doesn't contain the entire public API.
 * Simple things, and things that just call out to some manager,
 * are implemented independently by egg_romsrc_external.c and egg_romsrc_native.c.
 */

#include "egg_runner_internal.h"
#include "opt/serial/serial.h"
#include "opt/fs/fs.h"

/* Get user languages.
 */

int egg_get_user_languages(int *dst,int dsta) {
  if (dsta<1) return 0;
  
  /* If we were launched with --lang, that's the one and only answer.
   */
  if (egg.config.lang) {
    dst[0]=egg.config.lang;
    return 1;
  }
  
  /* POSIX systems typically have LANG as the single preferred locale, which starts with a language code.
   * There can also be LANGUAGE, which is multiple language codes separated by colons.
   */
  int dstc=0;
  const char *src;
  if (src=getenv("LANG")) {
    if ((src[0]>='a')&&(src[0]<='z')&&(src[1]>='a')&&(src[1]<='z')) {
      if (dstc<dsta) dst[dstc++]=rom_qual_eval(src,2);
    }
  }
  if (dstc>=dsta) return dstc;
  if (src=getenv("LANGUAGE")) {
    int srcp=0;
    while (src[srcp]&&(dstc<dsta)) {
      const char *token=src+srcp;
      int tokenc=0;
      while (src[srcp]&&(src[srcp++]!=':')) tokenc++;
      if ((tokenc>=2)&&(token[0]>='a')&&(token[0]<='z')&&(token[1]>='a')&&(token[1]<='z')) {
        int lang=rom_qual_eval(token,2);
        int already=0,i=dstc;
        while (i-->0) if (dst[i]==lang) { already=1; break; }
        if (!already) dst[dstc++]=lang;
      }
    }
  }
  
  //TODO I'm sure there are other mechanisms for MacOS and Windows. Find those.
  return dstc;
}

/* Store.
 * The store is a simple JSON object, and we keep it in memory fully encoded.
 * Updating one value, we rewrite the whole thing.
 */
 
void egg_store_flush() {
  if (!egg.store_dirty) return;
  egg.store_dirty=0;
  if (!egg.config.storepath) return;
  if (file_write(egg.config.storepath,egg.store,egg.storec)<0) {
    fprintf(stderr,"%s: Failed to write %d-byte store.\n",egg.config.storepath,egg.storec);
  } else {
    fprintf(stderr,"%s: Wrote store, %d bytes.\n",egg.config.storepath,egg.storec);
  }
}
 
void egg_store_quit() {
  egg_store_flush();
  if (egg.store) free(egg.store);
}

int egg_store_init() {
  if (!egg.config.storepath) {
    fprintf(stderr,"%s: Saving is disabled.\n",egg.exename);
    return 0;
  }
  if ((egg.storec=file_read(&egg.store,egg.config.storepath))<0) {
    egg.storec=0;
    fprintf(stderr,"%s: Failed to read store. This is ok.\n",egg.config.storepath);
  } else {
    fprintf(stderr,"%s: Loaded store, %d bytes.\n",egg.config.storepath,egg.storec);
  }
  return 0;
}
 
int egg_store_get(char *dst,int dsta,const char *k,int kc) {
  if (!k||(kc<1)) return 0;
  struct sr_decoder decoder={.v=egg.store,.c=egg.storec};
  if (sr_decode_json_object_start(&decoder)<0) return 0;
  char tmp[256];
  const char *q;
  int qc;
  while ((qc=sr_decode_json_next(&q,&decoder))>0) {
    if ((qc>=2)&&(q[0]=='"')&&(q[qc-1]=='"')) {
      qc=sr_string_eval(tmp,sizeof(tmp),q,qc);
      if (qc>sizeof(tmp)) qc=0;
      q=tmp;
    }
    if ((qc==kc)&&!memcmp(q,k,kc)) {
      return sr_decode_json_string(dst,dsta,&decoder);
    } else {
      if (sr_decode_json_skip(&decoder)<0) return 0;
    }
  }
  return 0;
}

int egg_store_set(const char *k,int kc,const char *v,int vc) {
  if (!k||(kc<1)) return 0;
  if (!egg.config.storepath) return -1;
  
  // When the store is empty, it's a different thing.
  if (!egg.storec) {
    if (!v||(vc<1)) return 0;
    struct sr_encoder encoder={0};
    if (
      (sr_encode_json_object_start(&encoder,0,0)<0)||
      (sr_encode_json_string(&encoder,k,kc,v,vc)<0)||
      (sr_encode_json_end(&encoder,0)<0)||
      (sr_encode_u8(&encoder,0x0a)<0)
    ) {
      sr_encoder_cleanup(&encoder);
      return -1;
    }
    if (encoder.c>egg.config.store_limit) {
      fprintf(stderr,"%s: %d-byte save file would exceed quota of %d bytes. Launch with --store-limit=BYTES to change.\n",egg.exename,encoder.c,egg.config.store_limit);
      sr_encoder_cleanup(&encoder);
      return -1;
    }
    if (egg.store) free(egg.store);
    egg.store=encoder.v;
    egg.storec=encoder.c;
    egg.store_dirty=1;
    return 0;
  }
  
  // Not empty, we'll kind of overkill it by decoding JSON into a fresh encoder.
  struct sr_encoder rewrite={0};
  struct sr_decoder decoder={.v=egg.store,.c=egg.storec};
  if (sr_decode_json_object_start(&decoder)<0) return -1;
  if (sr_encode_json_object_start(&rewrite,0,0)<0) return -1;
  char tmp[256];
  const char *q;
  int qc,got=0;
  while ((qc=sr_decode_json_next(&q,&decoder))>0) {
    if ((qc>=2)&&(q[0]=='"')&&(q[qc-1]=='"')) {
      qc=sr_string_eval(tmp,sizeof(tmp),q,qc);
      if (qc>sizeof(tmp)) {
        sr_encoder_cleanup(&rewrite);
        return -1;
      }
      q=tmp;
    }
    if ((qc==kc)&&!memcmp(q,k,kc)) {
      got=1;
      sr_decode_json_skip(&decoder);
      if (!v||(vc<1)) {
        // ok it's deleted
      } else {
        if (sr_encode_json_string(&rewrite,k,kc,v,vc)<0) {
          sr_encoder_cleanup(&rewrite);
          return -1;
        }
      }
    } else {
      const char *src=0;
      int srcc=sr_decode_json_expression(&src,&decoder);
      if ((srcc<0)||(sr_encode_json_preencoded(&rewrite,q,qc,src,srcc)<0)) {
        sr_encoder_cleanup(&rewrite);
        return -1;
      }
    }
  }
  if (!got&&v&&(vc>0)) {
    if (sr_encode_json_string(&rewrite,k,kc,v,vc)<0) {
      sr_encoder_cleanup(&rewrite);
      return -1;
    }
  }
  if ((sr_encode_json_end(&rewrite,0)<0)||(sr_encode_u8(&rewrite,0x0a)<0)) {
    sr_encoder_cleanup(&rewrite);
    return -1;
  }
  if (rewrite.c>egg.config.store_limit) {
    fprintf(stderr,"%s: %d-byte save file would exceed quota of %d bytes. Launch with --store-limit=BYTES to change.\n",egg.exename,rewrite.c,egg.config.store_limit);
    sr_encoder_cleanup(&rewrite);
    return -1;
  }
  if (egg.store) free(egg.store);
  egg.store=rewrite.v;
  egg.storec=rewrite.c;
  egg.store_dirty=1;
  return 0;
}

int egg_store_key_by_index(char *dst,int dsta,int p) {
  if (p<0) return 0;
  struct sr_decoder decoder={.v=egg.store,.c=egg.storec};
  if (sr_decode_json_object_start(&decoder)<0) return 0;
  char tmp[256];
  const char *q;
  int qc;
  while ((qc=sr_decode_json_next(&q,&decoder))>0) {
    if ((qc>=2)&&(q[0]=='"')&&(q[qc-1]=='"')) {
      qc=sr_string_eval(tmp,sizeof(tmp),q,qc);
      if (qc>sizeof(tmp)) return 0;
      q=tmp;
    }
    if (!p--) {
      if (qc<=dsta) memcpy(dst,q,qc);
      return qc;
    }
    sr_decode_json_skip(&decoder);
  }
  return 0;
}
