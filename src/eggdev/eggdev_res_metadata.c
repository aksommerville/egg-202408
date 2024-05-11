#include "eggdev_internal.h"

/* Compile metadata.
 */
 
int eggdev_metadata_compile(struct romw *romw,struct romw_res *res) {

  // Already in the binary format? Great!
  if ((res->serialc>=2)&&!memcmp(res->serial,"\xeeM",2)) return 0;

  struct sr_encoder dst={0};
  if (sr_encode_raw(&dst,"\xeeM",2)<0) return -1;
  struct sr_decoder decoder={.v=res->serial,.c=res->serialc};
  int lineno=0,linec;
  const char *line;
  while ((linec=sr_decode_line(&line,&decoder))>0) {
    lineno++;
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { linec--; line++; }
    if (!linec||(line[0]=='#')) continue;
    
    const char *k=line;
    int kc=0;
    int linep=0;
    while ((linep<linec)&&(line[linep]!='=')&&(line[linep]!=':')) { linep++; kc++; }
    while (kc&&((unsigned char)k[kc-1]<=0x20)) kc--;
    const char *v=0;
    int vc=0;
    if (linep<linec) {
      linep++;
      while ((linep<linec)&&((unsigned char)line[linep]<=0x20)) linep++;
      v=line+linep;
      vc=linec-linep;
    }
    if ((kc>0xff)||(vc>0xff)) {
      fprintf(stderr,"%s:%d: Key or value too long (%d,%d, limit 255)\n",res->path,lineno,kc,vc);
      sr_encoder_cleanup(&dst);
      return -2;
    }
    if (
      (sr_encode_u8(&dst,kc)<0)||
      (sr_encode_u8(&dst,vc)<0)||
      (sr_encode_raw(&dst,k,kc)<0)||
      (sr_encode_raw(&dst,v,vc)<0)
    ) {
      sr_encoder_cleanup(&dst);
      return -1;
    }
  }
  romw_res_handoff_serial(res,dst.v,dst.c);
  return 0;
}
