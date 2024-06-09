/* eggdev_bundle.c
 * Only ROM=>HTML is implemented here.
 * Fake-native and true-native executable bundling is managed by shell scripts generated at build time.
 */

#include "eggdev_internal.h"

/* Copy ROM file without wasm, if any wasm is present.
 */
 
static char eggdev_wasmless_path_storage[1024];

const char *eggdev_rewrite_rom_if_wasm(const char *path) {
  void *serial=0;
  int serialc=file_read(&serial,path);
  if (serialc<0) return path; // It's an error, but let someone else report it.
  struct rom rom={0};
  if (rom_init_handoff(&rom,serial,serialc)<0) {
    free(serial);
    return path;
  }
  int have_wasm=0;
  const struct rom_res *res=rom.resv;
  int i=rom.resc;
  for (;i-->0;res++) {
    int tid=0;
    rom_unpack_fqrid(&tid,0,0,res->fqrid);
    if (tid<EGG_RESTYPE_wasm) continue;
    if (tid==EGG_RESTYPE_wasm) have_wasm=1;
    break;
  }
  if (!have_wasm) {
    rom_cleanup(&rom);
    return path;
  }
  int dstpathc=snprintf(eggdev_wasmless_path_storage,sizeof(eggdev_wasmless_path_storage),"%s-wasmless",path);
  if ((dstpathc<1)||(dstpathc>=sizeof(eggdev_wasmless_path_storage))) {
    rom_cleanup(&rom);
    return 0;
  }
  struct romw romw={0};
  romw.tid_repr=eggdev_type_repr_static;
  for (res=rom.resv,i=rom.resc;i-->0;res++) {
    int tid=0,qual=0,rid=0;
    rom_unpack_fqrid(&tid,&qual,&rid,res->fqrid);
    if (tid==EGG_RESTYPE_wasm) continue;
    struct romw_res *dstres=romw_res_add(&romw);
    if (!dstres||(romw_res_set_serial(dstres,res->v,res->c)<0)) {
      rom_cleanup(&rom);
      romw_cleanup(&romw);
      return 0;
    }
    dstres->tid=tid;
    dstres->qual=qual;
    dstres->rid=rid;
  }
  rom_cleanup(&rom);
  struct sr_encoder dst={0};
  if (romw_encode(&dst,&romw)<0) {
    romw_cleanup(&romw);
    sr_encoder_cleanup(&dst);
    return 0;
  }
  romw_cleanup(&romw);
  int err=file_write(eggdev_wasmless_path_storage,dst.v,dst.c);
  sr_encoder_cleanup(&dst);
  if (err<0) return 0;
  return eggdev_wasmless_path_storage;
}

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

/* Generate CSS for the main canvas.
 */
 
static void eggdev_bundle_read_framebuffer_size(int *w,int *h,const uint8_t *src,int srcc) {
  if ((srcc<2)||memcmp(src,"\xeeM",2)) return;
  int srcp=2;
  int stopp=srcc-2;
  while (srcp<=stopp) {
    int kc=src[srcp++];
    int vc=src[srcp++];
    if (srcp>srcc-vc-kc) break;
    const char *k=(char*)(src+srcp);
    srcp+=kc;
    const char *v=(char*)(src+srcp);
    srcp+=vc;
    if ((kc==11)&&!memcmp(k,"framebuffer",11)) {
      int vp=0;
      while ((vp<vc)&&((unsigned char)v[vp]<=0x20)) vp++;
      int _w=0,_h=0;
      while ((vp<vc)&&(v[vp]>='0')&&(v[vp]<='9')) {
        _w*=10;
        _w+=v[vp]-'0';
        vp++;
      }
      if ((vp<vc)&&(v[vp++]=='x')) {
        while ((vp<vc)&&(v[vp]>='0')&&(v[vp]<='9')) {
          _h*=10;
          _h+=v[vp]-'0';
          vp++;
        }
      }
      if ((_w>0)&&(_w<=4096)&&(_h>0)&&(_h<=4096)) {
        *w=_w;
        *h=_h;
      }
      return;
    }
  }
}
 
static int eggdev_bundle_generate_canvas_style(struct sr_encoder *dst,const struct rom *rom) {

  /* Locate the "framebuffer" declaration in metadata:0:1, or make something up.
   * metadata:0:1 has to be the first resource; that's not a coincidence and it's actually impossible not to be.
   */
  int fbw=360,fbh=180;
  if ((rom->resc>=1)&&(rom->resv[0].fqrid==((EGG_RESTYPE_metadata<<26)|1))) {
    eggdev_bundle_read_framebuffer_size(&fbw,&fbh,rom->resv[0].v,rom->resv[0].c);
  }
  
  /* Make an arbitrary target size range that ought to fit in sane browser windows.
   * Determine whether it's possible to scale up by some integer to fit in that range.
   */
  const int targetwlo=640,targetwhi=1280;
  const int targethlo=480,targethhi=720;
  int scale=0;
  if ((fbw<targetwlo)&&(fbh<targethlo)) {
    scale=targetwhi/fbw;
    if ((scale>0)&&(fbh*scale>=targethlo)&&(fbh*scale<=targethhi)) {
      // cool
    } else {
      scale=targetwlo/fbw;
      if (fbw*scale<targetwlo) scale++;
      if ((scale>0)&&(fbh*scale>=targethlo)&&(fbh*scale<=targethhi)) {
        // cool
      } else {
        scale=0;
      }
    }
  }
  
  /* If we found a reasonable scale-up factor, force an explicit width and nearest-neighbor filtering.
   * If not, just leave (width,image-rendering) unset and let the browser figure it out. Linear filtering is the default, everywhere I've seen.
   */
  if (scale>0) {
    if (sr_encode_fmt(dst,"width: %dpx;\n",fbw*scale)<0) return -1;
    if (sr_encode_fmt(dst,"image-rendering: pixelated;\n")<0) return -1;
  }
  
  /* Must also have a black background, otherwise some framebuffer transfers show artifacts.
   * (when portions of the framebuffer end up less than fully opaque or something).
   */
  if (sr_encode_fmt(dst,"background-color: #000;\n")<0) return -1;
  
  return 0;
}

/* Split the HTML template into sections, on its insertion points.
 * All insertion points are single complete lines. So we can process the text linewise.
 * Fails if you don't provide enough output.
 */
 
#define EGGDEV_HTML_REPLACE_LITERAL 0
#define EGGDEV_HTML_REPLACE_ROM 1
#define EGGDEV_HTML_REPLACE_CANVAS_STYLE 2
 
struct eggdev_html_section {
  const char *src;
  int srcc;
  int replacement;
};

static int eggdev_html_split(struct eggdev_html_section *sectionv,int sectiona,const char *src,int srcc,const char *path) {
  const char *raw=0;
  int rawc=0;
  struct sr_decoder decoder={.v=src,.c=srcc};
  int linec;
  const char *line;
  int sectionc=0;
  while ((linec=sr_decode_line(&line,&decoder))>0) {
    const char *inner=line;
    int innerc=linec;
    while (innerc&&((unsigned char)inner[innerc-1]<=0x20)) innerc--;
    while (innerc&&((unsigned char)inner[0]<=0x20)) { inner++; innerc--; }
    
    int replacement=EGGDEV_HTML_REPLACE_LITERAL;
    if ((innerc==15)&&!memcmp(inner,"INSERT ROM HERE",15)) replacement=EGGDEV_HTML_REPLACE_ROM;
    else if ((innerc==24)&&!memcmp(inner,"INSERT CANVAS STYLE HERE",24)) replacement=EGGDEV_HTML_REPLACE_CANVAS_STYLE;
    
    if (replacement) {
      if (rawc) {
        if (sectionc>=sectiona) return -1;
        sectionv[sectionc].src=raw;
        sectionv[sectionc].srcc=rawc;
        sectionv[sectionc].replacement=EGGDEV_HTML_REPLACE_LITERAL;
        sectionc++;
        raw=0;
        rawc=0;
      }
      if (sectionc>=sectiona) return -1;
      sectionv[sectionc].replacement=replacement;
      sectionc++;
    } else {
      if (!raw) raw=line;
      rawc+=linec;
    }
  }
  if (rawc) {
    if (sectionc>=sectiona) return -1;
    sectionv[sectionc].src=raw;
    sectionv[sectionc].srcc=rawc;
    sectionv[sectionc].replacement=EGGDEV_HTML_REPLACE_LITERAL;
    sectionc++;
  }
  return sectionc;
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
  
  // Split into sections. There will always be five. But when that eventually changes, we shouldn't have to.
  struct eggdev_html_section sectionv[16];
  int sectiona=sizeof(sectionv)/sizeof(sectionv[0]);
  int sectionc=eggdev_html_split(sectionv,sectiona,tm,tmc,tmpath);
  if ((sectionc<0)||(sectionc>sectiona)) {
    if (sectionc!=-2) fprintf(stderr,"%s: Unspecified error splitting HTML template.\n",tmpath);
    free(tm);
    return -2;
  }
  
  // Emit each section.
  const struct eggdev_html_section *section=sectionv;
  int i=sectionc;
  int err=0;
  for (;i-->0;section++) {
    switch (section->replacement) {
    
      case EGGDEV_HTML_REPLACE_LITERAL: {
          if ((err=sr_encode_raw(dst,section->src,section->srcc))<0) goto _done_;
        } break;
        
      case EGGDEV_HTML_REPLACE_ROM: {
          if ((err=eggdev_bundle_rewrite_rom(dst,rom,srcpath))<0) goto _done_;
        } break;
        
      case EGGDEV_HTML_REPLACE_CANVAS_STYLE: {
          if ((err=eggdev_bundle_generate_canvas_style(dst,rom))<0) goto _done_;
        } break;
        
      default: {
          fprintf(stderr,"%s: Unknown HTML template section '%d'\n",tmpath,section->replacement);
          err=-2;
          goto _done_;
        }
    }
  }
  
 _done_:
  free(tm);
  if ((err<0)&&(err!=-2)) {
    fprintf(stderr,"%s: Unspecified error applying HTML template.\n",tmpath);
    err=-2;
  }
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
