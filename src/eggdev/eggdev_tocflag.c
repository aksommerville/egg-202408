#include "eggdev_internal.h"

/* Context.
 */
 
struct eggdev_tocflag_context {
  struct sr_encoder *dst;
  char **pathv;
  int pathc,patha;
};

static void eggdev_tocflag_cleanup(struct eggdev_tocflag_context *ctx) {
  if (ctx->pathv) {
    while (ctx->pathc-->0) free(ctx->pathv[ctx->pathc]);
    free(ctx->pathv);
  }
}

/* Add one path.
 */
 
static int eggdev_tocflag_append(struct eggdev_tocflag_context *ctx,const char *src) {
  if (ctx->pathc>=ctx->patha) {
    int na=ctx->patha+256;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(ctx->pathv,sizeof(void*)*na);
    if (!nv) return -1;
    ctx->pathv=nv;
    ctx->patha=na;
  }
  if (!(ctx->pathv[ctx->pathc]=strdup(src))) return -1;
  ctx->pathc++;
  return 0;
}

/* Add path recursively.
 */
 
static int eggdev_tocflag_cb(const char *path,const char *base,char type,void *userdata) {
  struct eggdev_tocflag_context *ctx=userdata;
  if (!type) type=file_get_type(path);
  if (type=='f') return eggdev_tocflag_append(ctx,path);
  if (type=='d') return dir_read(path,eggdev_tocflag_cb,ctx);
  return 0;
}

/* Generate, in context.
 */
 
static int strpcmp(const void *a,const void *b) {
  const char **A=(void*)a,**B=(void*)b;
  return strcmp(*A,*B);
}
 
static int eggdev_tocflag_generate_inner(struct eggdev_tocflag_context *ctx) {
  int err;
  char **pathv=(char**)eggdev.srcpathv;
  int i=eggdev.srcpathc;
  for (;i-->0;pathv++) {
    if ((err=dir_read(*pathv,eggdev_tocflag_cb,ctx))<0) return err;
  }
  qsort(ctx->pathv,ctx->pathc,sizeof(void*),strpcmp);
  for (pathv=ctx->pathv,i=ctx->pathc;i-->0;pathv++) {
    if (sr_encode_raw(ctx->dst,*pathv,-1)<0) return -1;
    if (sr_encode_u8(ctx->dst,0x0a)<0) return -1;
  }
  return 0;
}

/* Generate TOC flag file.
 * ie list all files under the given input directories, in a deterministic order.
 */
 
int eggdev_tocflag_generate(struct sr_encoder *dst) {
  struct eggdev_tocflag_context ctx={.dst=dst};
  int err=eggdev_tocflag_generate_inner(&ctx);
  eggdev_tocflag_cleanup(&ctx);
  if (err<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error generating TOC flag file.\n",eggdev.exename);
    return -2;
  }
  return 0;
}
