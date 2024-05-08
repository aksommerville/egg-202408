#include "eggdev_internal.h"

/* Slice strings.
 */
 
int eggdev_strings_slice(struct romw *romw,const char *src,int srcc,const struct eggdev_rpath *rpath) {
  struct sr_decoder decoder={.v=src,.c=srcc};
  int lineno=0,linec;
  const char *line;
  while ((linec=sr_decode_line(&line,&decoder))>0) {
    lineno++;
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { line++; linec--; }
    if (!linec||(line[0]=='#')) continue;
    
    int idc=0,rid=0;
    const char *name=0;
    int namec=0;
    while ((idc<linec)&&((unsigned char)line[idc]>0x20)) idc++;
    if ((line[0]>='0')&&(line[0]<='9')) {
      if ((sr_int_eval(&rid,line,idc)<2)||(rid<1)||(rid>0xffff)) {
        fprintf(stderr,"%s:%d: Invalid string ID '%.*s'. Expected 1..65535 or a C identifier.\n",rpath->path,lineno,idc,line);
        return -2;
      }
    } else {
      name=line;
      namec=idc;
    }
    
    while ((idc<linec)&&((unsigned char)line[idc]<=0x20)) idc++;
    line+=idc;
    linec-=idc;
    if (!linec) continue;
    
    struct romw_res *res=romw_res_add(romw);
    if (!res) return -1;
    res->tid=rpath->tid;
    res->qual=rpath->qual;
    res->rid=rid;
    res->lineno0=lineno;
    romw_res_set_name(res,name,namec);
    romw_res_set_path(res,rpath->path,-1); // TODO Seems excessive... are we ever going to need path set?
    
    if ((linec>=2)&&(line[0]=='"')) {
      char tmp[1024];
      int tmpc=sr_string_eval(tmp,sizeof(tmp),line,linec);
      if ((tmpc<0)||(tmpc>sizeof(tmp))) {
        fprintf(stderr,"%s:%d: Invalid string token.\n",rpath->path,lineno);
        return -2;
      }
      if (romw_res_set_serial(res,tmp,tmpc)<0) return -1;
    } else {
      if (romw_res_set_serial(res,line,linec)<0) return -1;
    }
  }
  return 1;
}
