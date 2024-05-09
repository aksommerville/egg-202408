#include "eggdev_internal.h"
#include <signal.h>

struct eggdev eggdev={0};

/* pack
 */
 
static int eggdev_main_pack() {
  if (!eggdev.dstpath) {
    fprintf(stderr,"%s: Please specify output path '-oPATH'\n",eggdev.exename);
    return 1;
  }
  int err;
  struct romw romw={0};
  int i=0; for (;i<eggdev.srcpathc;i++) {
    if ((err=eggdev_pack_add_file(&romw,eggdev.srcpathv[i]))<0) {
      if (err!=-2) fprintf(stderr,"%s: Unspecified error adding file.\n",eggdev.srcpathv[i]);
      return 1;
    }
  }
  if ((err=eggdev_pack_digest(&romw,0))<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error processing archive.\n",eggdev.dstpath);
    return 1;
  }
  romw_sort(&romw);
  struct sr_encoder dst={0};
  if (romw_encode(&dst,&romw)<0) {
    fprintf(stderr,"%s: Failed to encode %d-member archive.\n",eggdev.dstpath,romw.resc);
    return 1;
  }
  if (file_write(eggdev.dstpath,dst.v,dst.c)<0) {
    fprintf(stderr,"%s: Failed to write %d-byte file.\n",eggdev.dstpath,dst.c);
    return 1;
  }
  return 0;
}

/* unpack
 */
 
static int eggdev_main_unpack() {
  if (!eggdev.dstpath) {
    fprintf(stderr,"%s: Please specify output path '-oDIRECTORY'. We will create it.\n",eggdev.exename);
    return 1;
  }
  if (eggdev.srcpathc!=1) {
    fprintf(stderr,"%s: Please specify one input path.\n",eggdev.exename);
    return 1;
  }
  const char *srcpath=eggdev.srcpathv[0];
  void *serial=0;
  int serialc=file_read(&serial,srcpath);
  if (serialc<0) {
    fprintf(stderr,"%s: Failed to read file.\n",srcpath);
    return 1;
  }
  struct rom rom={0};
  if (rom_init_borrow(&rom,serial,serialc)<0) {
    fprintf(stderr,"%s: Failed to decode %d-byte file as Egg ROM.\n",srcpath,serialc);
    return 1;
  }
  if (dir_mkdirp(eggdev.dstpath)<0) {
    fprintf(stderr,"%s: Failed to create output directory.\n",eggdev.dstpath);
    return 1;
  }
  int pvtid=-1;
  char tname[256];
  int tnamec=0;
  const struct rom_res *res=rom.resv;
  int i=rom.resc;
  for (;i-->0;res++) {
    int tid=0,qual=0,rid=0;
    rom_unpack_fqrid(&tid,&qual,&rid,res->fqrid);
    if (tid!=pvtid) {
      tnamec=eggdev_type_repr(tname,sizeof(tname),tid);
      if ((tnamec<1)||(tnamec>=sizeof(tname))) return 1;
      pvtid=tid;
      char dpath[1024];
      int dpathc=snprintf(dpath,sizeof(dpath),"%s/%.*s",eggdev.dstpath,tnamec,tname);
      if ((dpathc<1)||(dpathc>=sizeof(dpath))) return 1;
      if (dir_mkdir(dpath)<0) {
        fprintf(stderr,"%s: Failed to create directory.\n",dpath);
        return 1;
      }
    }
    char dstpath[1024];
    int dstpathc;
    if (qual) {
      char qualstr[2];
      rom_qual_repr(qualstr,qual);
      dstpathc=snprintf(dstpath,sizeof(dstpath),"%s/%.*s/%d-%.2s",eggdev.dstpath,tnamec,tname,rid,qualstr);
    } else {
      dstpathc=snprintf(dstpath,sizeof(dstpath),"%s/%.*s/%d",eggdev.dstpath,tnamec,tname,rid);
    }
    if ((dstpathc<1)||(dstpathc>=sizeof(dstpath))) return 1;
    if (file_write(dstpath,res->v,res->c)<0) {
      fprintf(stderr,"%s: Failed to write %d-byte resource.\n",dstpath,res->c);
      return 1;
    }
  }
  return 0;
}

/* list
 */
 
static int eggdev_main_list() {
  if (eggdev.srcpathc!=1) {
    fprintf(stderr,"%s: Please provide exactly one input path.\n",eggdev.exename);
    return 1;
  }
  const char *srcpath=eggdev.srcpathv[0];
  void *serial=0;
  int serialc=file_read(&serial,srcpath);
  if (serialc<0) {
    fprintf(stderr,"%s: Failed to read file.\n",srcpath);
    return 1;
  }
  struct rom rom={0};
  if (rom_init_borrow(&rom,serial,serialc)<0) {
    fprintf(stderr,"%s: Failed to decode %d-byte file as Egg ROM.\n",srcpath,serialc);
    return 1;
  }
  const struct rom_res *res=rom.resv;
  int i=rom.resc;
  
  if (!eggdev.format||!strcmp(eggdev.format,"default")) {
    fprintf(stdout,"%s: Decoded %d-byte ROM file:\n",srcpath,serialc);
    char tname[256];
    int tnamec=0,pvtid=-1;
    for (;i-->0;res++) {
      int tid=0,qual=0,rid=0;
      rom_unpack_fqrid(&tid,&qual,&rid,res->fqrid);
      if (tid!=pvtid) {
        tnamec=eggdev_type_repr(tname,sizeof(tname),tid);
        if ((tnamec<1)||(tnamec>=sizeof(tname))) return 1;
        pvtid=tid;
      }
      char qualstr[2]="--";
      if (qual) rom_qual_repr(qualstr,qual);
      fprintf(stdout,"  %.*s:%.2s:%d: %d\n",tnamec,tname,qualstr,rid,res->c);
    }
  
  } else if (!strcmp(eggdev.format,"machine")) {
    for (;i-->0;res++) {
      int tid=0,qual=0,rid=0;
      rom_unpack_fqrid(&tid,&qual,&rid,res->fqrid);
      fprintf(stdout,"%d %d %d %d\n",tid,qual,rid,res->c);
    }
  
  } else if (!strcmp(eggdev.format,"summary")) {
    int count_by_tid[64]={0};
    int size_by_tid[64]={0};
    int tidmax=0;
    for (;i-->0;res++) {
      int tid=0,qual=0,rid=0;
      rom_unpack_fqrid(&tid,&qual,&rid,res->fqrid);
      count_by_tid[tid]++;
      size_by_tid[tid]+=res->c;
      tidmax=tid;
    }
    fprintf(stdout,"%s: Summary of %d-byte ROM file:\n",srcpath,serialc);
    int tid=0;
    for (;tid<=tidmax;tid++) {
      if (!count_by_tid[tid]) continue;
      char tname[256];
      int tnamec=eggdev_type_repr(tname,sizeof(tname),tid);
      if ((tnamec<1)||(tnamec>=sizeof(tname))) return 1;
      fprintf(stdout,"  %20.*s: %7d bytes for %5d resources.\n",tnamec,tname,size_by_tid[tid],count_by_tid[tid]);
    }
  
  } else {
    fprintf(stderr,"%s: Unknown output format '%s'. Expected 'default', 'machine', or 'summary'.\n",eggdev.exename,eggdev.format);
    return 1;
  }
  return 0;
}

/* toc
 */
 
static int eggdev_main_toc() {
  int err;
  struct romw romw={0};
  int i=0; for (;i<eggdev.srcpathc;i++) {
    if ((err=eggdev_pack_add_file(&romw,eggdev.srcpathv[i]))<0) {
      if (err!=-2) fprintf(stderr,"%s: Unspecified error adding file.\n",eggdev.srcpathv[i]);
      return 1;
    }
  }
  if ((err=eggdev_pack_digest(&romw,1))<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error processing archive.\n",eggdev.dstpath);
    return 1;
  }
  romw_sort(&romw);
  const struct romw_res *res=romw.resv;
  for (i=romw.resc;i-->0;res++) {
    if (!res->tid) continue;
    if (eggdev.named_only&&!res->namec) continue;
    char tname[256];
    int tnamec=eggdev_type_repr(tname,sizeof(tname),res->tid);
    if ((tnamec<1)||(tnamec>sizeof(tname))) return 1;
    char qualstr[2];
    rom_qual_repr(qualstr,res->qual);
    fprintf(stdout,"%.*s %.2s %d %.*s\n",tnamec,tname,qualstr,res->rid,res->namec,res->name);
  }
  return 0;
}

/* tocflag
 */
 
static int eggdev_main_tocflag() {
  if (!eggdev.dstpath) {
    fprintf(stderr,"%s: Please specify flag file path '-oPATH'\n",eggdev.exename);
    return 1;
  }
  void *prior=0;
  int priorc=file_read(&prior,eggdev.dstpath);
  if (priorc<0) priorc=0; // No harm if it doesn't exist yet.
  struct sr_encoder current={0};
  if (eggdev_tocflag_generate(&current)<0) return 1;
  if ((current.c==priorc)&&!memcmp(current.v,prior,priorc)) return 0; // Unchanged.
  if ((dir_mkdirp_parent(eggdev.dstpath)<0)||(file_write(eggdev.dstpath,current.v,current.c)<0)) {
    fprintf(stderr,"%s: Failed to write file.\n",eggdev.dstpath);
    return 1;
  }
  return 0;
}

/* bundle
 * The work gets done in a shell script, generated at the same time as eggdev.
 * Basically all we do is find the script and run it.
 */
 
static int eggdev_main_bundle() {
  if (!eggdev.dstpath) {
    fprintf(stderr,"%s: Please specify output executable path '-oPATH'\n",eggdev.exename);
    return 1;
  }
  if (!eggdev.rompath) {
    fprintf(stderr,"%s: Please specify input rom file '--rom=PATH'\n",eggdev.exename);
    return 1;
  }
  int dstpathc=0,ishtml=0;
  while (eggdev.dstpath[dstpathc]) dstpathc++;
  if ((dstpathc>=5)&&!memcmp(eggdev.dstpath+dstpathc-5,".html",5)) {
    ishtml=1;
  }
  int err;
  if (eggdev.codepath) {
    if (ishtml) {
      fprintf(stderr,"%s: '--code' is incompatible with '.html' output.\n",eggdev.exename);
      //...and if i have to explain that to you, you shouldn't be trusted with web dev!
      return 1;
    }
    err=eggdev_shell_script("egg-native.sh",eggdev.dstpath,eggdev.rompath,eggdev.codepath);
  } else if (ishtml) {
    err=eggdev_bundle_html(eggdev.dstpath,eggdev.rompath);
  } else {
    err=eggdev_shell_script("egg-bundle.sh",eggdev.dstpath,eggdev.rompath);
  }
  if (err<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error bundling executable.\n",eggdev.exename);
    return 1;
  }
  return 0;
}

/* unbundle
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
 
static int eggdev_main_unbundle() {
  if (!eggdev.dstpath) {
    fprintf(stderr,"%s: Please specify output ROM path '-oPATH'.\n",eggdev.exename);
    return 1;
  }
  if (eggdev.srcpathc!=1) {
    fprintf(stderr,"%s: Expected exactly one input file.\n",eggdev.exename);
    return 1;
  }
  const char *srcpath=eggdev.srcpathv[0];
  char cmd[1024];
  int cmdc=snprintf(cmd,sizeof(cmd),"nm %s",srcpath);
  if ((cmdc<1)||(cmdc>=sizeof(cmd))) return 1;
  struct sr_encoder nmout={0};
  int err=eggdev_command_sync(&nmout,cmd);
  if (err<0) {
    fprintf(stderr,"%s: Error running 'nm' on '%s'\n",eggdev.exename,srcpath);
    return 1;
  }
  int p=0,cp=0;
  if ((cp=eggdev_locate_rom_in_nm_output(&p,nmout.v,nmout.c))<=0) {
    fprintf(stderr,"%s: Failed to located bundled ROM file.\n",srcpath);
    return 1;
  }
  uint8_t *src=0;
  int srcc=file_read(&src,srcpath);
  if (srcc<0) {
    fprintf(stderr,"%s: Failed to read file.\n",srcpath);
    return 1;
  }
  if ((cp<0)||(cp>srcc-4)) {
    fprintf(stderr,"%s: Impossible position %d for ROM length in %d-byte executable.\n",srcpath,cp,srcc);
    return 1;
  }
  int c=src[cp]|(src[cp+1]<<8)|(src[cp+2]<<16)|(src[cp+3]<<24); // Assume little-endian. Is there some way we can be sure?
  if ((p<0)||(c<0)||(p>srcc-c)) {
    fprintf(stderr,"%s: Invalid position (%d,%d) in %d-byte executable.\n",srcpath,p,c,srcc);
    return 1;
  }
  if (file_write(eggdev.dstpath,src+p,c)<0) {
    fprintf(stderr,"%s: Failed to write %d-byte ROM\n",eggdev.dstpath,c);
    return 1;
  }
  return 0;
}

/* serve
 */
 
static volatile int eggdev_sigc=0;
 
static void eggdev_rcvsig(int sigid) {
  switch (sigid) {
    case SIGINT: if (++eggdev_sigc>=3) {
        fprintf(stderr,"Too many unprocessed signals.\n");
        exit(1);
      } break;
  }
}
 
static int eggdev_main_serve() {
  if (!eggdev.port) eggdev.port=8080;
  signal(SIGINT,eggdev_rcvsig);
  struct http_context_delegate delegate={
    .cb_serve=eggdev_http_serve,
  };
  if (!(eggdev.http=http_context_new(&delegate))) return 1;
  if (http_listen(eggdev.http,eggdev.external?0:1,eggdev.port)<0) {
    fprintf(stderr,"%s: Failed to open HTTP server on port %d\n",eggdev.exename,eggdev.port);
    return 1;
  }
  if (file_get_type("Makefile")=='f') eggdev.has_wd_makefile=1;
  fprintf(stderr,"%s: Listening on port %d. %s\n",eggdev.exename,eggdev.port,eggdev.external?"*** externally accessible ***":"localhost only");
  while (!eggdev_sigc) {
    if (http_update(eggdev.http,1000)<0) {
      fprintf(stderr,"%s: Error updating HTTP server.\n",eggdev.exename);
      http_context_del(eggdev.http);
      return 1;
    }
  }
  http_context_del(eggdev.http);
  fprintf(stderr,"%s: Normal exit.\n",eggdev.exename);
  return 0;
}

/* Main.
 */
 
int main(int argc,char **argv) {
  int err;
  if ((err=eggdev_configure(argc,argv))<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error during configuration.\n",eggdev.exename);
    return 1;
  }
  if (!eggdev.command) { eggdev_print_help(0,0); return 1; }
  if (!strcmp(eggdev.command,"help")) return 0;
  if (!strcmp(eggdev.command,"pack")) return eggdev_main_pack();
  if (!strcmp(eggdev.command,"unpack")) return eggdev_main_unpack();
  if (!strcmp(eggdev.command,"list")) return eggdev_main_list();
  if (!strcmp(eggdev.command,"toc")) return eggdev_main_toc();
  if (!strcmp(eggdev.command,"tocflag")) return eggdev_main_tocflag();
  if (!strcmp(eggdev.command,"bundle")) return eggdev_main_bundle();
  if (!strcmp(eggdev.command,"unbundle")) return eggdev_main_unbundle();
  if (!strcmp(eggdev.command,"serve")) return eggdev_main_serve();
  fprintf(stderr,"%s: Unknown command '%s'\n",eggdev.exename,eggdev.command);
  eggdev_print_help(0,0);
  return 1;
}
