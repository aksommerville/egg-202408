#include "rom.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

/* Cleanup.
 */
 
void rom_cleanup(struct rom *rom) {
  if (rom->serial&&rom->ownserial) free((void*)rom->serial);
  if (rom->resv) free(rom->resv);
  memset(rom,0,sizeof(struct rom));
}

/* Append resource.
 * Validates id fully, and it must come at the end of the existing set.
 */
 
static int rom_append(struct rom *rom,int tid,int qual,int rid,const void *v,int c) {
  if (!c) return 0;
  if ((tid<1)||(tid>0x3f)) return -1;
  if ((qual<0)||(qual>0x3ff)) return -1;
  if ((rid<0)||(rid>0xffff)) return -1;
  uint32_t fqrid=(tid<<26)|(qual<<16)|rid;
  if (rom->resc&&(fqrid<=rom->resv[rom->resc-1].fqrid)) return -1;
  if (rom->resc>=rom->resa) {
    int na=rom->resa+1024;
    if (na>INT_MAX/sizeof(struct rom_res)) return -1;
    void *nv=realloc(rom->resv,sizeof(struct rom_res)*na);
    if (!nv) return -1;
    rom->resv=nv;
    rom->resa=na;
  }
  struct rom_res *res=rom->resv+rom->resc++;
  res->fqrid=fqrid;
  res->v=v;
  res->c=c;
  return 0;
}

/* Decode binary ROM file, after validating geometry.
 */
 
static int rom_decode(struct rom *rom,const uint8_t *toc,int tocc,const uint8_t *heap,int heapc) {
  int tocp=0,heapp=0;
  int tid=1,qual=0,rid=1;
  while (tocp<tocc) {
    uint8_t lead=toc[tocp++];

    if (!(lead&0x80)) { // SMALL
      int len=lead;
      if (heapp>heapc-len) return -1;
      if (rom_append(rom,tid,qual,rid,heap+heapp,len)<0) return -1;
      heapp+=len;
      rid++;
    } else switch (lead&0xe0) {
    
      case 0x80: { // MEDIUM
          if (tocp>tocc-2) return -1;
          int len=(lead&0x1f)<<16;
          len|=toc[tocp++]<<8;
          len|=toc[tocp++];
          len+=128;
          if (heapp>heapc-len) return -1;
          if (rom_append(rom,tid,qual,rid,heap+heapp,len)<0) return -1;
          heapp+=len;
          rid++;
        } break;
        
      case 0xa0: { // LARGE
          if (tocp>tocc-3) return -1;
          int len=(lead&0x1f)<<24;
          len|=toc[tocp++]<<16;
          len|=toc[tocp++]<<8;
          len|=toc[tocp++];
          len+=2097279;
          if (heapp>heapc-len) return -1;
          if (rom_append(rom,tid,qual,rid,heap+heapp,len)<0) return -1;
          heapp+=len;
          rid++;
        } break;
        
      case 0xc0: { // QUAL or RID
          if ((lead&0xf0)==0xd0) { // RID
            int d=(lead&0x0f)+1;
            rid+=d;
            break;
          }
          if (lead&0x1c) return -1; // 3 bits reserved, must be zero.
          if (tocp>tocc-1) return -1;
          int d=(lead&0x03)<<8;
          d|=toc[tocp++];
          d+=1;
          qual+=d;
          rid=1;
        } break;
        
      case 0xe0: { // TYPE
          int d=(lead&0x1f);
          d+=1;
          tid+=d;
          qual=0;
          rid=1;
        } break;
    }
  }
  return 0;
}

/* Init.
 */

int rom_init_borrow(struct rom *rom,const void *src,int srcc) {
  if (rom->serial||rom->resv) return -1;
  
  // Validate header before touching the context.
  const uint8_t *SRC=src;
  if (!src||(srcc<16)||memcmp(src,"\xea\x00\xff\xff",4)) return -1;
  int hdrlen=(SRC[4]<<24)|(SRC[5]<<16)|(SRC[6]<<8)|SRC[7];
  int toclen=(SRC[8]<<24)|(SRC[9]<<16)|(SRC[10]<<8)|SRC[11];
  int heaplen=(SRC[12]<<24)|(SRC[13]<<16)|(SRC[14]<<8)|SRC[15];
  if ((hdrlen<16)||(toclen<0)||(heaplen<0)) return -1;
  if (hdrlen>INT_MAX-toclen) return -1;
  if (hdrlen+toclen>INT_MAX-heaplen) return -1;
  if (hdrlen+toclen+heaplen>srcc) return -1;
  
  rom->serial=src;
  rom->serialc=srcc;
  rom->ownserial=0;
  if (rom_decode(rom,rom->serial+hdrlen,toclen,rom->serial+hdrlen+toclen,heaplen)<0) {
    rom_cleanup(rom);
    return -1;
  }
  return 0;
}

int rom_init_handoff(struct rom *rom,const void *src,int srcc) {
  if (!src||(srcc<1)) return -1;
  if (rom_init_borrow(rom,src,srcc)<0) return -1;
  rom->ownserial=1;
  return 0;
}

int rom_init_copy(struct rom *rom,const void *src,int srcc) {
  if (!src||(srcc<1)) return -1;
  void *nv=malloc(srcc);
  if (!nv) return -1;
  memcpy(nv,src,srcc);
  if (rom_init_borrow(rom,nv,srcc)<0) {
    free(nv);
    return -1;
  }
  rom->ownserial=1;
  return 0;
}

/* Get resource (public).
 */

int rom_get(void *dstpp,struct rom *rom,int tid,int qual,int rid) {
  if ((tid<1)||(tid>0x3f)) return 0;
  if ((qual<0)||(qual>0x3ff)) return 0;
  if ((rid<1)||(rid>0xffff)) return 0;
  uint32_t fqrid=(tid<<26)|(qual<<16)|rid;
  int lo=0,hi=rom->resc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct rom_res *res=rom->resv+ck;
         if (fqrid<res->fqrid) hi=ck;
    else if (fqrid>res->fqrid) lo=ck+1;
    else {
      if (dstpp) *(const void**)dstpp=res->v;
      return res->c;
    }
  }
  return 0;
}

/* ID analysis.
 */
 
void rom_unpack_fqrid(int *tid,int *qual,int *rid,uint32_t fqrid) {
  if (tid) *tid=fqrid>>26;
  if (qual) *qual=(fqrid>>16)&0x3ff;
  if (rid) *rid=fqrid&0xffff;
}

void rom_qual_repr(char *dst/*2*/,int qual) {
  dst[0]="abcdefghijklmnopqrstuvwxyz123456"[(qual>>5)&31];
  dst[1]="abcdefghijklmnopqrstuvwxyz123456"[qual&31];
}

static inline int rom_qual_eval_1(char src) {
  if ((src>='a')&&(src<='z')) return src-'a';
  if ((src>='1')&&(src<='6')) return src-'1'+26;
  return -1;
}

int rom_qual_eval(const char *src,int srcc) {
  if (!src) return 0;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if (srcc!=2) return 0;
  int hi=rom_qual_eval_1(src[0]);
  if (hi<0) return 0;
  int lo=rom_qual_eval_1(src[1]);
  if (lo<0) return 0;
  return (hi<<5)|lo;
}
