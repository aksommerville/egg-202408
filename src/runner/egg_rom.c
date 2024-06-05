#include "egg_runner_internal.h"
#include "opt/serial/serial.h"

/* Get a field from metadata:0:1.
 */
 
int egg_rom_get_metadata(char *dst,int dsta,const char *k,int kc,int translatable) {
  if (!k) kc=0; else if (kc<0) { kc=0; while (k[kc]) kc++; }
  
  // Fields for human consumption are "translatable", meaning there can be a field (k+"String") which is a string id.
  if (translatable) {
    char sk[256];
    if (kc<=sizeof(sk)-6) {
      memcpy(sk,k,kc);
      memcpy(sk+kc,"String",6);
      int skc=kc+6;
      int rid;
      char idstring[16];
      int idstringc=egg_rom_get_metadata(idstring,sizeof(idstring),sk,skc,0);
      if ((idstringc>0)&&(idstringc<=sizeof(idstring))&&(sr_int_eval(&rid,idstring,idstringc)>=2)&&(rid>0)&&(rid<=0xffff)) {
        int lang=0;
        if (egg_get_user_languages(&lang,1)>=0) {
          const void *tmp=0;
          int tmpc=rom_get(&tmp,&egg.rom,EGG_RESTYPE_string,lang,rid);
          if (tmpc>0) {
            if (tmpc<=dsta) {
              memcpy(dst,tmp,tmpc);
              if (tmpc<dsta) dst[tmpc]=0;
            }
            return tmpc;
          }
        }
      }
    }
  }
  
  // Not translatable or anything goes wrong in the string lookup, pull straight from metadata.
  // I don't think there's much value in pre-processing the metadata. It's very easy to parse from scratch each time.
  const uint8_t *src=0;
  int srcc=rom_get(&src,&egg.rom,EGG_RESTYPE_metadata,0,1);
  if ((srcc>=2)&&(src[0]==0xee)&&(src[1]=='M')) {
    int srcp=2,stopp=srcc-2;
    while (srcp<=stopp) {
      int qkc=src[srcp++];
      int qvc=src[srcp++];
      if (srcp>srcc-qkc-qvc) break;
      if ((qkc==kc)&&!memcmp(src+srcp,k,kc)) {
        srcp+=qkc;
        if (qvc<=dsta) {
          memcpy(dst,src+srcp,qvc);
          if (qvc<dsta) dst[qvc]=0;
        }
        return qvc;
      }
      srcp+=qkc+qvc;
    }
  }
  if (dsta>0) dst[0]=0;
  return 0;
}

/* Fetch some metadata fields, and digest for use at startup.
 */

int egg_rom_startup_props(struct egg_rom_startup_props *props) {
  char tmp[1024];
  int tmpc;
  
  if (((tmpc=egg_rom_get_metadata(tmp,sizeof(tmp),"title",5,1))>0)&&(tmpc<=sizeof(tmp))) {
    if (!(props->title=malloc(tmpc+1))) return -1;
    memcpy(props->title,tmp,tmpc);
    props->title[tmpc]=0;
  }
  
  if (((tmpc=egg_rom_get_metadata(tmp,sizeof(tmp),"iconImage",9,0))>0)&&(tmpc<=sizeof(tmp))) {
    int imageid=0;
    if ((sr_int_eval(&imageid,tmp,tmpc)>=2)&&(imageid>0)&&(imageid<=0xffff)) {
      const void *serial=0;
      int serialc=rom_get(&serial,&egg.rom,EGG_RESTYPE_image,0,imageid);
      if (serialc>0) {
        struct png_image *image=png_decode(serial,serialc);
        if (image) {
          if (png_image_reformat(image,8,6)>=0) {
            props->iconrgba=image->v; // handoff
            image->v=0;
            props->iconw=image->w;
            props->iconh=image->h;
          }
          png_image_del(image);
        }
      }
    }
  }
  
  if (((tmpc=egg_rom_get_metadata(tmp,sizeof(tmp),"framebuffer",11,0))>0)&&(tmpc<=sizeof(tmp))) {
    // Must start "WIDTHxHEIGHT".
    int tmpp=0;
    while ((tmpp<tmpc)&&((unsigned char)tmp[tmpp]<=0x20)) tmpp++;
    const char *token=tmp+tmpp;
    int tokenc=0;
    while ((tmpp<tmpc)&&(tmp[tmpp++]!='x')) tokenc++;
    if (sr_int_eval(&props->fbw,token,tokenc)<2) props->fbw=0;
    token=tmp+tmpp;
    tokenc=0;
    while ((tmpp<tmpc)&&((unsigned char)tmp[tmpp]>0x20)) { tmpp++; tokenc++; }
    if (sr_int_eval(&props->fbh,token,tokenc)<2) props->fbh=0;
    // Followed optionally by space-delimited extra tokens.
    while (tmpp<tmpc) {
      if ((unsigned char)tmp[tmpp]<=0x20) { tmpp++; continue; }
      token=tmp+tmpp;
      tokenc=0;
      while ((tmpp<tmpc)&&((unsigned char)tmp[tmpp]>0x20)) { tmpp++; tokenc++; }
      if ((tokenc==2)&&!memcmp(token,"gl",2)) {
        props->directgl=1;
      } else {
        fprintf(stderr,"%s: Ignoring unexpected framebuffer tag '%.*s'\n",egg.exename,tokenc,token);
      }
    }
  }
  // TODO Should we have a default framebuffer size?
  if ((props->fbw<1)||(props->fbh<1)||(props->fbw>0x7fff)||(props->fbh>0x7fff)) {
    fprintf(stderr,"%s: Invalid framebuffer size.\n",egg.exename);
    egg_rom_startup_props_cleanup(props);
    return -2;
  }
  
  return 0;
}

void egg_rom_startup_props_cleanup(struct egg_rom_startup_props *props) {
  if (props->title) {
    free(props->title);
    props->title=0;
  }
  if (props->iconrgba) {
    free(props->iconrgba);
    props->iconrgba=0;
    props->iconw=0;
    props->iconh=0;
  }
}

/* Assert "require" from metadata.
 */
 
int egg_rom_assert_required() {
  const char *refname=egg.config.rompath;
  if (!refname) refname=egg.exename;
  #define IGNORABLE(fmt,...) { \
    if (egg.config.ignore_required) fprintf(stderr,"%s:WARNING: "fmt" Proceeding anyway.\n",refname,##__VA_ARGS__); \
    else { fprintf(stderr,"%s: "fmt" Launch with '--ignore-required' to try anyway.\n",refname,##__VA_ARGS__); return -2; } \
  }
  #define FATAL(fmt,...) { \
    fprintf(stderr,"%s: "fmt"%s\n",refname,##__VA_ARGS__,egg.config.ignore_required?" This error can't be ignored.":""); \
    return -2; \
  }
  char src[1024];
  int srcc=egg_rom_get_metadata(src,sizeof(src),"require",7,0);
  if (!srcc) return 0;
  if (srcc>sizeof(src)) {
    IGNORABLE("ROM's requirements list is unreadable. We are not able to validate compatibility.")
    return 0;
  }
  int srcp=0;
  while (srcp<srcc) {
    if (((unsigned char)src[srcp]<=0x20)||(src[srcp]==',')) { srcp++; continue; }
    const char *token=src+srcp;
    int tokenc=0;
    while ((srcp<srcc)&&(src[srcp++]!=',')) tokenc++;
    while (tokenc&&((unsigned char)token[tokenc-1]<=0x20)) tokenc--;
    if (!tokenc) continue;
      
    // TODO Pick off known feature tokens.
    
    IGNORABLE("Unknown required feature '%.*s'.",tokenc,token)
  }
  #undef IGNORABLE
  #undef FATAL
  return 0;
}
