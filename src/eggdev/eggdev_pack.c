#include "eggdev_internal.h"

/* Analyze source path.
 */

static void eggdev_pack_analyze_path(struct eggdev_rpath *rpath,const char *path) {
  memset(rpath,0,sizeof(struct eggdev_rpath));
  rpath->path=path;
  
  // Walk thru the path componentwise. If we see a valid type name, keep it and reset qual.
  // After type, qual is allowed as a directory name.
  // We do examine the basename here too. For some things like string, it's convenient to use qual as the file name.
  const char *base=0;
  int pathp=0,basec=0;
  while (path[pathp]) {
    if (path[pathp]=='/') {
      pathp++;
      continue;
    }
    base=path+pathp;
    basec=0;
    while (path[pathp]&&(path[pathp++]!='/')) basec++;
    int q=eggdev_type_eval(base,basec);
    if (q) {
      rpath->tid=q;
      rpath->qual=0;
    } else if (rpath->tid) {
      if ((q=rom_qual_eval(base,basec))>0) {
        rpath->qual=q;
      }
    }
  }
  
  // If basename begins with a decimal integer, that's (rid).
  int basep=0;
  while ((basep<basec)&&(base[basep]>='0')&&(base[basep]<='9')) {
    rpath->rid*=10;
    rpath->rid+=base[basep++]-'0';
  }
  //...unless it's followed by a letter.
  if (basep<basec) {
    if (((base[basep]>='a')&&(base[basep]<='z'))||((base[basep]>='A')&&(base[basep]<='Z'))) {
      rpath->rid=0;
      basep=0;
    }
  }
  
  // Skip dashes after rid. Or leading dashes, but what kind of monster would use those?
  while ((basep<basec)&&(base[basep]=='-')) basep++;
  
  // Now everything up to the first dot is the name.
  if ((basep<basec)&&(base[basep]!='.')) {
    rpath->name=base+basep;
    while ((basep<basec)&&(base[basep]!='.')) { basep++; rpath->namec++; }
  }
  
  // Anything between dots is entirely ignored.
  // After the last dot, force lowercase as (sfx), if short enough.
  int dotp=-1;
  int i=basec;
  while (i>=basep) {
    if (base[i]=='.') { dotp=i; break; }
    i--;
  }
  if (dotp>=0) {
    const char *pre=base+dotp+1;
    int prec=basec-dotp-1;
    if (prec<=sizeof(rpath->sfx)) {
      rpath->sfxc=prec;
      for (i=prec;i-->0;) {
        if ((pre[i]>='A')&&(pre[i]<='Z')) rpath->sfx[i]=pre[i]+0x20;
        else rpath->sfx[i]=pre[i];
      }
    }
  }
}

/* Add regular file to archive.
 */
 
static int eggdev_pack_add_regular_file(struct romw *romw,const char *path) {
  struct eggdev_rpath rpath;
  eggdev_pack_analyze_path(&rpath,path);
  
  void *serial=0;
  int serialc=file_read(&serial,path);
  if (serialc<0) {
    fprintf(stderr,"%s: Failed to read file.\n",path);
    return -2;
  }
  
  // String or sound, if no ID is set, try slicing.
  if (!rpath.rid) {
    switch (rpath.tid) {
    
      case RESTYPE_string: {
          int err=eggdev_strings_slice(romw,serial,serialc,&rpath);
          if (err) {
            free(serial);
            if (err>=0) return 0;
            if (err!=-2) fprintf(stderr,"%s: Unspecified error slicing string resources.\n",path);
            return -2;
          }
        } break;
        
      case RESTYPE_sound: {
          int err=eggdev_sounds_slice(romw,serial,serialc,&rpath);
          if (err) {
            free(serial);
            if (err>=0) return 0;
            if (err!=-2) fprintf(stderr,"%s: Unspecified error slicing sound resources.\n",path);
            return -2;
          }
        } break;
    }
  }
  
  // Normal cases, add to romw as is.
  struct romw_res *res=romw_res_add(romw);
  if (!res) {
    free(serial);
    return -1;
  }
  res->tid=rpath.tid;
  res->qual=rpath.qual;
  res->rid=rpath.rid;
  romw_res_handoff_serial(res,serial,serialc);
  romw_res_set_name(res,rpath.name,rpath.namec);
  romw_res_set_path(res,path,-1);
  return 0;
}

/* Add file or directory to archive.
 */
 
static int eggdev_pack_add_from_dir(const char *path,const char *base,char type,void *userdata) {
  struct romw *romw=userdata;
  if (!type) type=file_get_type(path);
  switch (type) {
    case 'f': return eggdev_pack_add_regular_file(romw,path);
    case 'd': return dir_read(path,eggdev_pack_add_from_dir,romw);
  }
  return 0;
}
 
int eggdev_pack_add_file(struct romw *romw,const char *path) {
  switch (file_get_type(path)) {
    case 'f': return eggdev_pack_add_regular_file(romw,path);
    case 'd': return dir_read(path,eggdev_pack_add_from_dir,romw);
    default: {
        fprintf(stderr,"%s: Not a directory or regular file.\n",path);
        return -2;
      }
  }
  return 0;
}

/* Assign rid to any resource that needs one.
 */
 
static int eggdev_assign_missing_ids(struct romw *romw) {
  uint8_t tmpbits[256];
  struct romw_res *res=romw->resv;
  int i=romw->resc;
  for (;i-->0;res++) {
    if (res->rid) continue;
    if (!res->tid) continue; // Assume it's slated for removal.
    
    // If we have a name, and another resource of this type but different qualifier exists, take its rid.
    if (res->namec) {
      const struct romw_res *other=romw->resv;
      int otheri=romw->resc;
      for (;otheri-->0;other++) {
        if (!other->rid) continue;
        if (other->tid!=res->tid) continue;
        if (other->qual==res->qual) continue;
        if (other->namec!=res->namec) continue;
        if (memcmp(other->name,res->name,res->namec)) continue;
        res->rid=other->rid;
        break;
      }
      if (res->rid) continue;
    }
    
    // Search for gaps among the first 2k of this tid+qual, and record the highest.
    // Beyond 2k, we won't try to fill gaps.
    memset(tmpbits,0,sizeof(tmpbits));
    tmpbits[0]=1; // pretend rid zero in use, otherwise we'd select it
    const struct romw_res *other=romw->resv;
    int otheri=romw->resc;
    int maxrid=0;
    for (;otheri-->0;other++) {
      if (other->tid!=res->tid) continue;
      if (other->qual!=res->qual) continue;
      int bitsp=other->rid>>3;
      if (bitsp<sizeof(tmpbits)) tmpbits[bitsp]|=1<<(other->rid&7);
      if (other->rid>maxrid) maxrid=other->rid;
    }
    int major=0; for (;major<sizeof(tmpbits);major++) {
      if (tmpbits[major]==0xff) continue;
      int minor=0,mask=1;
      for (;minor<8;minor++,mask<<1) {
        if (!(tmpbits[major]&mask)) {
          res->rid=(major<<3)+minor;
          break;
        }
      }
      break;
    }
    if (!res->rid) res->rid=maxrid+1;
  }
  return 0;
}

/* Digest resources, after everything is loaded.
 */
 
int eggdev_pack_digest(struct romw *romw,int toc_only) {
  int err;
  if ((err=eggdev_assign_missing_ids(romw))<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error assigning missing resource IDs.\n",eggdev.dstpath);
    return -2;
  }
  if (toc_only) return 0;
  struct romw_res *res=romw->resv;
  int i=romw->resc;
  for (;i-->0;res++) {
    switch (res->tid) {
      case RESTYPE_metadata: err=eggdev_metadata_compile(romw,res); break;
      case RESTYPE_wasm: break;
      case RESTYPE_string: break;
      case RESTYPE_image: err=eggdev_image_compile(romw,res); break;
      case RESTYPE_song: err=eggdev_song_compile(romw,res); break;
      case RESTYPE_sound: err=eggdev_sound_compile(romw,res); break;
    }
  }
  return 0;
}
