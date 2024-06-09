#include "rom.h"
#include "opt/serial/serial.h"
#include "egg/egg_store.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* Type names.
 */
 
static char romw_tid_repr_buf[32];
 
static const char *romw_tid_repr(const struct romw *romw,int tid) {
  if (romw->tid_repr) {
    const char *name=romw->tid_repr(tid);
    if (name&&name[0]) return name;
  }
  switch (tid) {
    #define _(tag) case EGG_RESTYPE_##tag: return #tag;
    EGG_RESTYPE_FOR_EACH
    #undef _
  }
  sr_decsint_repr(romw_tid_repr_buf,sizeof(romw_tid_repr_buf),tid);
  return romw_tid_repr_buf;
}

/* Cleanup.
 */
 
static void romw_res_cleanup(struct romw_res *res) {
  if (res->name) free(res->name);
  if (res->path) free(res->path);
  if (res->serial) free(res->serial);
}

void romw_cleanup(struct romw *romw) {
  if (romw->resv) {
    while (romw->resc-->0) romw_res_cleanup(romw->resv+romw->resc);
    free(romw->resv);
  }
  memset(romw,0,sizeof(struct romw));
}

/* Add resource.
 */

struct romw_res *romw_res_add(struct romw *romw) {
  if (romw->resc>=romw->resa) {
    int na=romw->resa+256;
    if (na>INT_MAX/sizeof(struct romw_res)) return 0;
    void *nv=realloc(romw->resv,sizeof(struct romw_res)*na);
    if (!nv) return 0;
    romw->resv=nv;
    romw->resa=na;
  }
  struct romw_res *res=romw->resv+romw->resc++;
  memset(res,0,sizeof(struct romw_res));
  return res;
}

/* Resource fields.
 */
 
int romw_res_set_name(struct romw_res *res,const char *src,int srcc) {
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  char *nv=0;
  if (srcc) {
    if (!(nv=malloc(srcc+1))) return -1;
    memcpy(nv,src,srcc);
    nv[srcc]=0;
  }
  if (res->name) free(res->name);
  res->name=nv;
  res->namec=srcc;
  return 0;
}

int romw_res_set_path(struct romw_res *res,const char *src,int srcc) {
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  char *nv=0;
  if (srcc) {
    if (!(nv=malloc(srcc+1))) return -1;
    memcpy(nv,src,srcc);
    nv[srcc]=0;
  }
  if (res->path) free(res->path);
  res->path=nv;
  res->pathc=srcc;
  return 0;
}

int romw_res_set_serial(struct romw_res *res,const void *src,int srcc) {
  if ((srcc<0)||(srcc&&!src)) return -1;
  void *nv=0;
  if (srcc) {
    if (!(nv=malloc(srcc))) return -1;
    memcpy(nv,src,srcc);
  }
  if (res->serial) free(res->serial);
  res->serial=nv;
  res->serialc=srcc;
  return 0;
}

void romw_res_handoff_serial(struct romw_res *res,void *v,int c) {
  if ((c<0)||(c&&!v)) return;
  if (res->serial) free(res->serial);
  res->serial=v;
  res->serialc=c;
}

/* Search.
 */

struct romw_res *romw_res_get_by_id(const struct romw *romw,int tid,int qual,int rid) {
  struct romw_res *q0=0;
  struct romw_res *res=romw->resv;
  int i=romw->resc;
  for (;i-->0;res++) {
    if (res->tid!=tid) continue;
    if (res->rid!=rid) continue;
    if (res->qual==qual) return res;
    if (!res->qual) q0=res;
  }
  return q0;
}

struct romw_res *romw_res_get_by_name(const struct romw *romw,int tid,int qual,const char *name,int namec) {
  if (!name) namec=0; else if (namec<0) { namec=0; while (name[namec]) namec++; }
  struct romw_res *q0=0;
  struct romw_res *res=romw->resv;
  int i=romw->resc;
  for (;i-->0;res++) {
    if (res->tid!=tid) continue;
    if (res->namec!=namec) continue;
    if (memcmp(res->name,name,namec)) continue;
    if (res->qual==qual) return res;
    if (!res->qual) q0=res;
  }
  return q0;
}

/* Remove range.
 */

void romw_remove(struct romw *romw,int p,int c) {
  if (p<0) return;
  if (c<1) return;
  if (p>romw->resc-c) return;
  struct romw_res *res=romw->resv+p;
  int i=c;
  for (;i-->0;res++) romw_res_cleanup(res);
  res=romw->resv+p;
  romw->resc-=c;
  memmove(res,res+c,sizeof(struct romw_res)*(romw->resc-p));
}

/* Filter.
 */
 
void romw_filter(struct romw *romw,int (*cb)(const struct romw_res *res,void *userdata),void *userdata) {
  int i=romw->resc;
  struct romw_res *res=romw->resv+i-1;
  for (;i-->0;res--) {
    if (cb(res,userdata)) continue;
    romw->resc--;
    romw_res_cleanup(res);
    memmove(res,res+1,sizeof(struct romw_res)*(romw->resc-i));
  }
}

/* Sort.
 */
 
static int romw_res_cmp(const void *a,const void *b) {
  const struct romw_res *A=a,*B=b;
  if (A->tid<B->tid) return -1;
  if (A->tid>B->tid) return 1;
  if (A->qual<B->qual) return -1;
  if (A->qual>B->qual) return 1;
  if (A->rid<B->rid) return -1;
  if (A->rid>B->rid) return 1;
  return 0;
}

void romw_sort(struct romw *romw) {
  qsort(romw->resv,romw->resc,sizeof(struct romw_res),romw_res_cmp);
}

/* Encode.
 */
 
static int romw_encode_toc(struct sr_encoder *dst,const struct romw *romw) {
  int xtid=1,xqual=0,xrid=1,heapp=0;
  const struct romw_res *res=romw->resv;
  int i=romw->resc;
  for (;i-->0;res++) {
    if (!res->tid&&!res->qual&&!res->rid) continue; // Skip if ids are straight zero.
    if (!res->serialc) continue; // Skip empties. Empty and absent are the same thing at runtime.
    if ((res->tid<1)||(res->tid>63)||(res->qual<0)||(res->qual>1023)||(res->rid<1)||(res->rid>65535)) {
      char qstr[2]; rom_qual_repr(qstr,res->qual);
      fprintf(stderr,"Invalid resource ID: %s:%.2s:%d (%s:%d)\n",romw_tid_repr(romw,res->tid),qstr,res->rid,res->path,res->lineno0);
      return -2;
    }
    
    // Advance tid if needed.
    if (res->tid<xtid) return -1;
    if (res->tid>xtid) {
      int d=res->tid-xtid;
      while (d>32) {
        if (sr_encode_u8(dst,0xff)<0) return -1;
        d-=32;
      }
      if (sr_encode_u8(dst,0xe0|(d-1))<0) return -1;
      xtid=res->tid;
      xqual=0;
      xrid=1;
    }
    
    // Advance qual if needed.
    if (res->qual<xqual) return -1;
    if (res->qual>xqual) {
      int d=res->qual-xqual-1;
      if (sr_encode_u8(dst,0xc0|(d>>8))<0) return -1;
      if (sr_encode_u8(dst,d)<0) return -1;
      xqual=res->qual;
      xrid=1;
    }
    
    // Advance rid if needed.
    if (res->rid<xrid) {
      char qstr[2]; rom_qual_repr(qstr,res->qual);
      fprintf(stderr,"!!! Resource ID conflict: (%s:%.2s:%d), expecting rid %d. %s:%d\n",romw_tid_repr(romw,res->tid),qstr,res->rid,xrid,res->path,res->lineno0);
      return -1;
    }
    if (res->rid>xrid) {
      int d=res->rid-xrid;
      while (d>16) {
        if (sr_encode_u8(dst,0xdf)<0) return -1;
        d-=16;
      }
      if (sr_encode_u8(dst,0xd0|(d-1))<0) return -1;
      xrid=res->rid;
    }
    
    // Emit the resource.
    int c=res->serialc;
    if ((c<0)||(c>=538968190)) {
      char qstr[2]; rom_qual_repr(qstr,res->qual);
      fprintf(stderr,"!!! Unreasonable size %d for resource %s:%.2s:%d !!! %s:%d\n",c,romw_tid_repr(romw,res->tid),qstr,res->rid,res->path,res->lineno0);
      return -1;
    }
    if (c>=2097279) {
      int wc=c-2097279;
      if (sr_encode_u8(dst,0xa0|(wc>>24))<0) return -1;
      if (sr_encode_u8(dst,wc>>16)<0) return -1;
      if (sr_encode_u8(dst,wc>>8)<0) return -1;
      if (sr_encode_u8(dst,wc)<0) return -1;
    } else if (c>=128) {
      int wc=c-128;
      if (sr_encode_u8(dst,0x80|(wc>>16))<0) return -1;
      if (sr_encode_u8(dst,wc>>8)<0) return -1;
      if (sr_encode_u8(dst,wc)<0) return -1;
    } else {
      if (sr_encode_u8(dst,c)<0) return -1;
    }
    heapp+=c;
    xrid++;
  }
  return heapp;
}

static int romw_encode_heap(struct sr_encoder *dst,const struct romw *romw) {
  const struct romw_res *res=romw->resv;
  int i=romw->resc;
  for (;i-->0;res++) {
    if (!res->tid&&!res->qual&&!res->rid) continue; // Skip if ids are straight zero.
    if (!res->serialc) continue; // Skip empties. Empty and absent are the same thing at runtime.
    if (sr_encode_raw(dst,res->serial,res->serialc)<0) return -1;
  }
  return 0;
}

int romw_encode(struct sr_encoder *dst,const struct romw *romw) {

  // Emit signature and Header length, then 8 bytes placeholder for TOC length and Heap length.
  if (sr_encode_raw(dst,"\xea\x00\xff\xff\0\0\0\x10",8)<0) return -1;
  int dstc0=dst->c;
  if (sr_encode_zero(dst,8)<0) return -1;
  
  // Emit TOC, and it will return the expected heap length.
  int tocp=dst->c;
  int heap_expect=romw_encode_toc(dst,romw);
  if (heap_expect<0) return -1;
  int tocc=dst->c-tocp;
  
  // Now emit the Heap, and it must produce the expected length.
  int heapp=dst->c;
  if (romw_encode_heap(dst,romw)<0) return -1;
  int heapc=dst->c-heapp;
  if (heapc!=heap_expect) {
    fprintf(stderr,"%s:%d: Expected to produce %d bytes heap, but actual was %d.\n",__FILE__,__LINE__,heap_expect,heapc);
    return -1;
  }
  
  // Fill in TOC and Heap lengths.
  uint8_t *p=((uint8_t*)dst->v)+dstc0;
  *p++=tocc>>24;
  *p++=tocc>>16;
  *p++=tocc>>8;
  *p++=tocc;
  *p++=heapc>>24;
  *p++=heapc>>16;
  *p++=heapc>>8;
  *p++=heapc;
  
  return 0;
}
