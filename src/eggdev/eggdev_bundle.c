/* eggdev_bundle.c
 * Only ROM=>HTML is implemented here.
 * Fake-native and true-native executable bundling is managed by shell scripts generated at build time.
 */

#include "eggdev_internal.h"

/* Write out a ROM in the HTML-specific bundler format.
 */
 
static const char *alphabet=
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz"
  "0123456789+/";
  
static void eggdev_base64_encode_1(char *dst,const uint8_t *src) {
  dst[0]=alphabet[src[0]>>2];
  dst[1]=alphabet[((src[0]<<4)|(src[1]>>4))&0x3f];
  dst[2]=alphabet[((src[1]<<2)|(src[2]>>6))&0x3f];
  dst[3]=alphabet[src[2]&0x3f];
}
 
static int eggdev_bundle_rewrite_rom(struct sr_encoder *dst,const struct rom *rom,const char *srcpath) {
  int xtid=1,xqual=0,xrid=1,linelen=0;
  const struct rom_res *res=rom->resv;
  int i=rom->resc;
  for (;i-->0;res++) {
    int tid=0,qual=0,rid=0;
    rom_unpack_fqrid(&tid,&qual,&rid,res->fqrid);
    
    // Advance tid?
    if (tid<xtid) return -1;
    if (tid>xtid) {
      int d=tid-xtid-1;
      if (sr_encode_fmt(dst,"t%x",d)<0) return -1;
      xtid=tid;
      xqual=0;
      xrid=1;
      linelen+=2;
    }
    
    // Advance qual?
    if (qual<xqual) return -1;
    if (qual>xqual) {
      int d=qual-xqual-1;
      if (sr_encode_fmt(dst,"q%x",d)<0) return -1;
      xqual=qual;
      xrid=1;
      linelen+=2;
    }
    
    // Advance rid?
    if (rid<xrid) return -1;
    if (rid>xrid) {
      int d=rid-xrid-1;
      if (sr_encode_fmt(dst,"s%x",d)<0) return -1;
      xrid=rid;
      linelen+=2;
    }
    
    // Emit resource.
    if (sr_encode_fmt(dst,"r%x(",res->c)<0) return -1;
    const uint8_t *src=res->v;
    int srcc=res->c;
    int stopp=(srcc/3)*3;
    int srcp=0;
    while (srcp<stopp) {
      char b64[4];
      eggdev_base64_encode_1(b64,src+srcp);
      if (sr_encode_raw(dst,b64,4)<0) return -1;
      linelen+=4;
      if (linelen>=100) {
        if (sr_encode_u8(dst,0x0a)<0) return -1;
        linelen=0;
      }
      srcp+=3;
    }
    switch (srcc-srcp) {
      case 1: {
          src+=srcp;
          if (sr_encode_u8(dst,alphabet[src[0]>>2])<0) return -1;
          if (sr_encode_u8(dst,alphabet[(src[0]<<4)&0x3f])<0) return -1;
        } break;
      case 2: {
          src+=srcp;
          if (sr_encode_u8(dst,alphabet[src[0]>>2])<0) return -1;
          if (sr_encode_u8(dst,alphabet[((src[0]<<4)|(src[1]>>4))&0x3f])<0) return -1;
          if (sr_encode_u8(dst,alphabet[((src[1]<<2))&0x3f])<0) return -1;
        } break;
    }
    if (sr_encode_u8(dst,')')<0) return -1;
    xrid++;
  }
  return 0;
}

/* Generate bundled HTML from live ROM store.
 */
 
static int eggdev_bundle_html_from_rom(struct sr_encoder *dst,const struct rom *rom,const char *srcpath) {

  // Template must live at "../web/bundle-template.html", relative to this executable.
  const char *pfx=eggdev.exename;
  int pfxc=0; while (pfx[pfxc]) pfxc++;
  while (pfxc&&(pfx[pfxc-1]!='/')) pfxc--;
  if (!pfxc--) return -1;
  while (pfxc&&(pfx[pfxc-1]!='/')) pfxc--;
  if (!pfxc) return -1;
  char tmpath[1024];
  int tmpathc=snprintf(tmpath,sizeof(tmpath),"%.*sweb/bundle-template.html",pfxc,pfx);
  if ((tmpathc<1)||(tmpathc>=sizeof(tmpath))) return -1;
  char *tm=0;
  int tmc=file_read(&tm,tmpath);
  if (tmc<0) {
    fprintf(stderr,"%s: Failed to read web bundle template\n",tmpath);
    return -2;
  }
  
  // Find the ROM insertion point in template.
  int insp=-1,stopp=tmc-15;
  int i=0; for (;i<=stopp;i++) {
    if (!memcmp(tm+i,"INSERT ROM HERE",15)) {
      insp=i;
      break;
    }
  }
  if (insp<0) {
    fprintf(stderr,"%s: Template does not contain the marker 'INSERT ROM HERE'\n",tmpath);
    free(tm);
    return -1;
  }
  
  // Emit everything before the insertion point.
  if (sr_encode_raw(dst,tm,insp)<0) {
    free(tm);
    return -1;
  }
  
  // Reformat and emit the ROM file.
  int err=eggdev_bundle_rewrite_rom(dst,rom,srcpath);
  if (err<0) {
    free(tm);
    if (err!=-2) fprintf(stderr,"%s: Unspecified error rewriting ROM file\n",srcpath);
    return -2;
  }
  
  // Emit the rest.
  const char *tail=tm+insp+15;
  int tailc=tmc-insp-15;
  err=sr_encode_raw(dst,tail,tailc);
  free(tm);
  return err;
}

/* Bundle HTML, main entry point.
 */
 
int eggdev_bundle_html(const char *dstpath,const char *srcpath) {
  void *romserial=0;
  int romserialc=file_read(&romserial,srcpath);
  if (romserialc<0) {
    fprintf(stderr,"%s: Failed to read file\n",srcpath);
    return -2;
  }
  struct rom rom={0};
  if (rom_init_handoff(&rom,romserial,romserialc)<0) {
    fprintf(stderr,"%s: Failed to decode ROM file\n",srcpath);
    free(romserial);
    return -2;
  }
  struct sr_encoder html={0};
  int err=eggdev_bundle_html_from_rom(&html,&rom,srcpath);
  rom_cleanup(&rom);
  if (err<0) {
    sr_encoder_cleanup(&html);
    if (err!=-2) fprintf(stderr,"%s: Failed to generate bundled HTML\n",dstpath);
    return -2;
  }
  err=file_write(dstpath,html.v,html.c);
  sr_encoder_cleanup(&html);
  if (err<0) {
    fprintf(stderr,"%s: Failed to write file\n",dstpath);
    return -2;
  }
  return 0;
}
