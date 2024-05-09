#include "eggdev_internal.h"

/* Decode one resource body from the HTML's base64.
 * You allocate the buffer in advance, length was already provided.
 * Text content is allowed to go long or short.
 * Returns length consumed, and does not consume the terminator ')' (but always ends directly upon it).
 */
 
static int eggdev_unbundle_html_resource(uint8_t *dst,int dsta,const char *src,int srcc) {
  int dstc=0,srcp=0;
  uint8_t tmp[4]; // 0..63
  int tmpc=0;
  for (;;) {
    if (srcp>=srcc) return -1;
    if (src[srcp]==')') break;
    char ch=src[srcp++];
    if ((unsigned char)ch<=0x20) continue;
         if ((ch>='A')&&(ch<='Z')) tmp[tmpc++]=ch-'A';
    else if ((ch>='a')&&(ch<='z')) tmp[tmpc++]=ch-'a'+26;
    else if ((ch>='0')&&(ch<='9')) tmp[tmpc++]=ch-'0'+52;
    else if (ch=='+') tmp[tmpc++]=62;
    else if (ch=='/') tmp[tmpc++]=63;
    else return -1;
    if (tmpc>=4) {
      if (dstc<dsta) dst[dstc++]=(tmp[0]<<2)|(tmp[1]>>4);
      if (dstc<dsta) dst[dstc++]=(tmp[1]<<4)|(tmp[2]>>2);
      if (dstc<dsta) dst[dstc++]=(tmp[2]<<6)|tmp[3];
      tmpc=0;
    }
  }
  if (tmpc) {
    memset(tmp+tmpc,0,sizeof(tmp)-tmpc);
    if (dstc<dsta) dst[dstc++]=(tmp[0]<<2)|(tmp[1]>>4);
    if (dstc<dsta) dst[dstc++]=(tmp[1]<<4)|(tmp[2]>>2);
    if (dstc<dsta) dst[dstc++]=(tmp[2]<<6)|tmp[3];
  }
  return srcp;
}

/* Locate HTML ROM and decode into a live romw.
 */
 
static int eggdev_unbundle_html_to_romw(struct romw *romw,const char *src,int srcc,const char *srcpath) {
  int srcp=0;
  int stopp=srcc-9;
  for (;;srcp++) {
    if (srcp>stopp) {
      fprintf(stderr,"%s: No Egg ROM found in HTML file\n",srcpath);
      return -2;
    }
    if (!memcmp(src+srcp,"<egg-rom ",9)) {
      srcp+=9;
      for (;;) {
        if (srcp>=srcc) return -1;
        if (src[srcp++]=='>') break;
      }
      break;
    }
  }
  // (srcp) is now situated just beyond the <egg-rom> opening tag. Read ROM text until '<'.
  int tid=1,qual=0,rid=1;
  for (;;) {
    if (srcp>=srcc) return -1;
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    if (src[srcp]=='<') break;
    
    // Each command is one letter (t,q,s,r) followed by a hexadecimal integer.
    char cmd=src[srcp++];
    int arg=0,digitc=0;
    while (srcp<srcc) {
      int digit;
           if ((src[srcp]>='0')&&(src[srcp]<='9')) digit=src[srcp]-'0';
      else if ((src[srcp]>='a')&&(src[srcp]<='f')) digit=src[srcp]-'a'+10;
      else if ((src[srcp]>='A')&&(src[srcp]<='F')) digit=src[srcp]-'A'+10;
      else break;
      arg<<=4;
      arg|=digit;
      digitc++;
      srcp++;
    }
    if (!digitc) return -1;
    
    switch (cmd) {
      case 't': tid+=arg+1; qual=0; rid=1; break;
      case 'q': qual+=arg+1; rid=1; break;
      case 's': rid+=arg+1; break;
      case 'r': {
          if ((tid>0x3f)||(qual>0x3ff)||(rid>0xffff)) return -1;
          while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
          if ((srcp>=srcc)||(src[srcp++]!='(')) return -1;
          void *v=calloc(1,arg);
          if (!v) return -1;
          int err=eggdev_unbundle_html_resource(v,arg,src+srcp,srcc-srcp);
          if (err<0) {
            free(v);
            return err;
          }
          srcp+=err;
          if ((srcp>=srcc)||(src[srcp++]!=')')) {
            free(v);
            return -1;
          }
          struct romw_res *res=romw_res_add(romw);
          if (!res) {
            free(v);
            return -1;
          }
          res->tid=tid;
          res->qual=qual;
          res->rid=rid;
          res->serial=v;
          res->serialc=arg;
          rid++;
        } break;
      default: return -1;
    }
  }
  return 0;
}

/* Extract archive from HTML file.
 */
 
int eggdev_unbundle_html(void *dstpp,const char *srcpath) {
  char *src=0;
  int srcc=file_read(&src,srcpath);
  if (srcc<0) {
    fprintf(stderr,"%s: Failed to read file\n",srcpath);
    return -2;
  }
  struct romw romw={0};
  int err=eggdev_unbundle_html_to_romw(&romw,src,srcc,srcpath);
  free(src);
  if (err<0) {
    romw_cleanup(&romw);
    if (err!=-2) fprintf(stderr,"%s: Unspecified error extracting ROM from HTML\n",srcpath);
    return -2;
  }
  struct sr_encoder dst={0};
  if (romw_encode(&dst,&romw)<0) {
    fprintf(stderr,"%s: Failed to reencode ROM file.\n",srcpath);
    romw_cleanup(&romw);
    sr_encoder_cleanup(&dst);
    return -2;
  }
  romw_cleanup(&romw);
  *(void**)dstpp=dst.v;
  return dst.c;
}

/* Parse output of nm.
 */
 
static int eggdev_locate_rom_in_nm_output(int *p,const char *src,int srcc) {
  struct sr_decoder decoder={.v=src,.c=srcc};
  int linec,c=0;
  const char *line;
  while ((linec=sr_decode_line(&line,&decoder))>0) {
    int qp=0,linep=0;
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while ((linep<linec)&&((unsigned char)line[linep]<=0x20)) linep++;
    while ((linep<linec)&&((unsigned char)line[linep]>0x20)) {
      int digit;
           if ((line[linep]>='0')&&(line[linep]<='9')) digit=line[linep]-'0';
      else if ((line[linep]>='a')&&(line[linep]<='f')) digit=line[linep]-'a'+10;
      else if ((line[linep]>='A')&&(line[linep]<='F')) digit=line[linep]-'A'+10;
      else { qp=-1; break; }
      if (qp&~(INT_MAX>>4)) { qp=-1; break; }
      qp<<=4;
      qp|=digit;
      linep++;
    }
    if (qp<=0) continue; // 0 might be valid, but very unrealistic
    const char *name=line+linec;
    int namec=0;
    while ((namec<linec)&&((unsigned char)name[-1]>0x20)) { name--; namec++; }
    if ((namec==15)&&!memcmp(name,"egg_rom_bundled",15)) {
      *p=qp;
    } else if ((namec==22)&&!memcmp(name,"egg_rom_bundled_length",22)) {
      c=qp;
    }
  }
  return c;
}

/* Extract archive from executable.
 */
 
int eggdev_unbundle_exe(void *dstpp,const char *srcpath) {
  char cmd[1024];
  int cmdc=snprintf(cmd,sizeof(cmd),"nm %s",srcpath);
  if ((cmdc<1)||(cmdc>=sizeof(cmd))) return 1;
  struct sr_encoder nmout={0};
  int err=eggdev_command_sync(&nmout,cmd);
  if (err<0) {
    fprintf(stderr,"%s: Error running 'nm' on '%s'\n",eggdev.exename,srcpath);
    sr_encoder_cleanup(&nmout);
    return -2;
  }
  int p=0,cp=0;
  if ((cp=eggdev_locate_rom_in_nm_output(&p,nmout.v,nmout.c))<=0) {
    fprintf(stderr,"%s: Failed to locate bundled ROM file.\n",srcpath);
    sr_encoder_cleanup(&nmout);
    return -2;
  }
  sr_encoder_cleanup(&nmout);
  uint8_t *src=0;
  int srcc=file_read(&src,srcpath);
  if (srcc<0) {
    fprintf(stderr,"%s: Failed to read file.\n",srcpath);
    return -2;
  }
  if ((cp<0)||(cp>srcc-4)) {
    fprintf(stderr,"%s: Impossible position %d for ROM length in %d-byte executable.\n",srcpath,cp,srcc);
    free(src);
    return -2;
  }
  int c=src[cp]|(src[cp+1]<<8)|(src[cp+2]<<16)|(src[cp+3]<<24); // Assume little-endian. Is there some way we can be sure?
  if ((p<0)||(c<0)||(p>srcc-c)) {
    fprintf(stderr,"%s: Invalid position (%d,%d) in %d-byte executable.\n",srcpath,p,c,srcc);
    free(src);
    return -2;
  }
  void *dst=malloc(c);
  if (!dst) {
    free(src);
    return -1;
  }
  memcpy(dst,src+p,c);
  free(src);
  *(void**)dstpp=dst;
  return c;
}
