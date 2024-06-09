#include "eggdev_internal.h"

/* Decode and store type names.
 */
 
static int eggdev_types_decode(const char *src,int srcc) {
  struct sr_decoder decoder={.v=src,.c=srcc};
  int lineno=0,linec;
  const char *line;
  while ((linec=sr_decode_line(&line,&decoder))>0) {
    lineno++;
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { linec--; line++; }
    if (!linec||(line[0]=='#')) continue;
    
    const char *itoken=line;
    int itokenc=0;
    while (linec&&((unsigned char)line[0]>0x20)) { linec--; line++; itokenc++; }
    while (linec&&((unsigned char)line[0]<=0x20)) { linec--; line++; }
    int tid;
    if ((sr_int_eval(&tid,itoken,itokenc)<2)||(tid<1)||(tid>0x3f)) {
      fprintf(stderr,"%s:%d: Expected 1..63, found '%.*s'\n",eggdev.typespath,lineno,itokenc,itoken);
      return -2;
    }
    if (eggdev.name_by_tid[tid]) {
      fprintf(stderr,"%s:%d: Multiple definition of tid %d ('%.*s' and '%s')\n",eggdev.typespath,lineno,tid,itokenc,itoken,eggdev.name_by_tid[tid]);
      return -2;
    }
    if (!(eggdev.name_by_tid[tid]=malloc(linec+1))) return -1;
    memcpy(eggdev.name_by_tid[tid],line,linec);
    eggdev.name_by_tid[tid][linec]=0;
  }
  return 0;
}

/* Load the user-supplied type names, if they exist and we haven't yet.
 */
 
static int eggdev_types_require() {
  if (eggdev.name_by_tid) return 0; // got em already
  if (!eggdev.typespath) return 0; // nothing to get
  char *src=0;
  int srcc=file_read(&src,eggdev.typespath);
  if (srcc<0) {
    fprintf(stderr,"%s: Failed to read type definitions.\n",eggdev.typespath);
    eggdev.typespath=0; // don't try again.
    return -2;
  }
  if (!(eggdev.name_by_tid=calloc(64,sizeof(void*)))) {
    free(src);
    return -1;
  }
  int err=eggdev_types_decode(src,srcc);
  free(src);
  return err;
}

/* Represent type.
 */
 
int eggdev_type_repr(char *dst,int dsta,int tid) {
  eggdev_types_require();
  const char *src=0;
  if (eggdev.name_by_tid) src=eggdev.name_by_tid[tid];
  if (!src) switch (tid) {
    #define _(tag) case EGG_RESTYPE_##tag: src=#tag; break;
    EGG_RESTYPE_FOR_EACH
    #undef _
  }
  if (src) {
    int dstc=0; while (src[dstc]) dstc++;
    if (dstc<=dsta) {
      memcpy(dst,src,dstc);
      if (dstc<dsta) dst[dstc]=0;
    }
    return dstc;
  }
  return sr_decsint_repr(dst,dsta,tid);
}

const char *eggdev_type_repr_static(int tid) {
  // We don't need to handle standard or really-unknown types.
  eggdev_types_require();
  if (!eggdev.name_by_tid) return 0;
  return eggdev.name_by_tid[tid];
}

/* Evaluate type.
 */
 
int eggdev_type_eval(const char *src,int srcc) {
  eggdev_types_require();
  if (!src) return 0;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if (eggdev.name_by_tid) {
    int tid=1; for (;tid<64;tid++) {
      const char *q=eggdev.name_by_tid[tid];
      if (!q) continue;
      if (memcmp(src,q,srcc)) continue;
      if (q[srcc]) continue;
      return tid;
    }
  }
  #define _(tag) if ((srcc==sizeof(#tag)-1)&&!memcmp(src,#tag,srcc)) return EGG_RESTYPE_##tag;
  EGG_RESTYPE_FOR_EACH
  #undef _
  return 0;
}
