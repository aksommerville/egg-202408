#include "eggdev_internal.h"

/* Analyze source path.
 */

static int eggdev_pack_analyze_path(struct eggdev_rpath *rpath,const char *path) {
  memset(rpath,0,sizeof(struct eggdev_rpath));
  rpath->path=path;
  
  // Extract the last two components of the path, those are all we care about.
  int pathc=0; while (path[pathc]) pathc++;
  while (pathc&&(path[pathc-1]=='/')) pathc--; // Trailing slashes would mess us up.
  const char *tname=0,*base=path;
  int tnamec=0,basec=0;
  int pathp=0;
  for (;pathp<pathc;pathp++) {
    if (path[pathp]=='/') {
      tname=base;
      tnamec=basec;
      base=path+pathp+1;
      basec=0;
    } else {
      basec++;
    }
  }
  
  /* Some standard types enjoy special privileges in naming.
   * Custom types (and most standard ones) must follow the generic pattern.
   */
  if ((basec==8)&&!memcmp(base,"metadata",8)) {
    // "metadata" is always metadata:0:1, the only allowed ID.
    rpath->tid=EGG_RESTYPE_metadata;
    rpath->qual=0;
    rpath->rid=1;
    rpath->name="metadata";
    rpath->namec=8;
    return 0;
  }
  if ((tnamec==6)&&!memcmp(tname,"string",6)) {
    // "string" type are named "QUAL[-COMMENT]", and we drop COMMENT immediately (only the editor needs it).
    rpath->tid=EGG_RESTYPE_string;
    if ((basec<2)||((basec>2)&&(base[2]!='-'))) {
      fprintf(stderr,"%s: String file basename must be 'QUAL' or 'QUAL-COMMENT' where QUAL is two letters.\n",path);
      return -2;
    }
    if ((base[0]=='0')&&(base[1]=='0')) {
      rpath->qual=0; // This is legal (albeit weird). Pick off specially, so we can treat 0 from rom_qual_eval as an error.
    } else if ((rpath->qual=rom_qual_eval(base,2))<1) {
      fprintf(stderr,"%s: Expected two-letter ISO 639 code, found '%.2s'\n",path,base);
      return -2;
    }
    if (basec>2) {
      // This won't be used anywhere, but might as well record it.
      rpath->name=base+3;
      rpath->namec=basec-3;
    }
    return 0;
  }
  
  /* Everything else follows the generic pattern:
   *   TYPE/ID[.FORMAT]
   *   TYPE/ID-NAME[.FORMAT]
   *   TYPE/ID-QUAL-NAME[.FORMAT]
   *   TYPE/NAME[.FORMAT]
   *   TYPE/QUAL-NAME[.FORMAT]
   */
  if (!(rpath->tid=eggdev_type_eval(tname,tnamec))) {
    fprintf(stderr,"%s: Expected resource type, found '%.*s'\n",path,tnamec,tname);
    return -2;
  }
  // Strip ".FORMAT" suffix first; it's the same in all cases.
  // If there's more than one dot, we preserve the bit after the last one and discard anything in between.
  int basep=0,dot1p=-1,dot2p=-1;
  for (;basep<basec;basep++) {
    if (base[basep]=='.') {
      if (dot1p<0) dot1p=basep;
      else dot2p=basep;
    }
  }
  if (dot2p>=0) {
    const char *src=base+dot2p+1;
    int srcc=basec-dot2p-1;
    if (srcc<=sizeof(rpath->sfx)) {
      int i=srcc;
      while (i-->0) {
        if ((src[i]>='A')&&(src[i]<='Z')) rpath->sfx[i]=src[i]+0x20;
        else rpath->sfx[i]=src[i];
      }
      rpath->sfxc=srcc;
    }
  }
  if (dot1p>=0) basec=dot1p;
  if (basec<1) return -1; // Don't allow a leading dot.
  // Find dashes. There must be zero, one, or two.
  int dash1p=-1,dash2p=-1;
  for (basep=0;basep<basec;basep++) {
    if (base[basep]=='-') {
      if (dash1p<0) dash1p=basep;
      else if (dash2p<0) dash2p=basep;
      else {
        fprintf(stderr,"%s: Invalid resource name. Must contain no more than 2 dashes. (Separating ID, QUAL, and NAME).\n",path);
        return -2;
      }
    }
  }
  // If there's two dashes, base must be "ID-QUAL-NAME".
  if (dash2p>=0) {
    if ((sr_int_eval(&rpath->rid,base,dash1p)<2)||(rpath->rid<1)||(rpath->rid>0xffff)) return -1;
    rpath->qual=rom_qual_eval(base+dash1p+1,dash2p-dash1p-1);
    rpath->name=base+dash2p+1;
    rpath->namec=basec-dash2p-1;
  // One dash can be either "ID-NAME" or "QUAL-NAME". There's overlap here, and ID wins ties.
  } else if (dash1p>=0) {
    if ((sr_int_eval(&rpath->rid,base,dash1p)<2)||(rpath->rid<1)||(rpath->rid>0xffff)) {
      rpath->rid=0;
      rpath->qual=rom_qual_eval(base,dash1p);
    }
    rpath->name=base+dash1p+1;
    rpath->namec=basec-dash1p-1;
  // No dashes, base is either ID or NAME, depending on whether its first character is a digit.
  } else if ((base[0]>='0')&&(base[0]<='9')) {
    if ((sr_int_eval(&rpath->rid,base,dash1p)<2)||(rpath->rid<1)||(rpath->rid>0xffff)) return -1;
  } else {
    rpath->name=base;
    rpath->namec=basec;
  }
  
  return 0;
}

/* Add regular file to archive.
 */
 
static int eggdev_pack_add_regular_file(struct romw *romw,const char *path) {
  struct eggdev_rpath rpath;
  int err=eggdev_pack_analyze_path(&rpath,path);
  if (err<0) {
    if (err!=2) fprintf(stderr,"%s: Invalid resource path.\n",path);
    return -2;
  }
  
  void *serial=0;
  int serialc=file_read(&serial,path);
  if (serialc<0) {
    fprintf(stderr,"%s: Failed to read file.\n",path);
    return -2;
  }
  
  // String or sound, if no ID is set, try slicing.
  if (!rpath.rid) {
    switch (rpath.tid) {
    
      case EGG_RESTYPE_string: {
          err=eggdev_strings_slice(romw,serial,serialc,&rpath);
          if (err) {
            free(serial);
            if (err>=0) return 0;
            if (err!=-2) fprintf(stderr,"%s: Unspecified error slicing string resources.\n",path);
            return -2;
          }
        } break;
        
      case EGG_RESTYPE_sound: {
          err=eggdev_sounds_slice(romw,serial,serialc,&rpath);
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

/* Add regular file.
 */

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
      for (;minor<8;minor++,mask<<=1) {
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

/* Validate resource set.
 * Everything has been compiled and finalized.
 * But there can still be blanks, and they aren't sorted yet.
 * We can return errors, but I think everything here will be advisory-only.
 */
 
static int eggdev_pack_validate(const struct romw *romw) {
  int have_metadata=0,have_wasm=0;
  const struct romw_res *res=romw->resv;
  int i=romw->resc;
  for (;i-->0;res++) {
    switch (res->tid) {
    
      case EGG_RESTYPE_metadata: {
          if ((res->qual!=0)||(res->rid!=1)) {
            fprintf(stderr,"%s:WARNING: 'metadata' type should only be used once, with qual zero and rid one\n",res->path);
          } else {
            have_metadata=1;
          }
        } break;
        
      case EGG_RESTYPE_wasm: {
          if ((res->qual!=0)||(res->rid!=1)) {
            fprintf(stderr,"%s:WARNING: 'wasm' type should only be used once, with qual zero and rid one\n",res->path);
          } else {
            have_wasm=1;
          }
        } break;
    }
  }
  if (!have_metadata) fprintf(stderr,"%s:WARNING: ROM file must contain a resource metadata:0:1\n",eggdev.dstpath);
  if (!have_wasm) fprintf(stderr,"%s:WARNING: ROM file must contain a resource wasm:0:1\n",eggdev.dstpath);
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
      case EGG_RESTYPE_metadata: err=eggdev_metadata_compile(romw,res); break;
      case EGG_RESTYPE_wasm: err=eggdev_wasm_compile(romw,res); break;
      case EGG_RESTYPE_string: break;
      case EGG_RESTYPE_image: err=eggdev_image_compile(romw,res); break;
      case EGG_RESTYPE_song: err=eggdev_song_compile(romw,res); break;
      case EGG_RESTYPE_sound: err=eggdev_sound_compile(romw,res); break;
    }
    if (err<0) {
      if (err!=-2) {
        char tname[32];
        int tnamec=eggdev_type_repr(tname,sizeof(tname),res->tid);
        if ((tnamec<1)||(tnamec>sizeof(tname))) tnamec=sr_decsint_repr(tname,sizeof(tname),res->tid);
        fprintf(stderr,"%s:%d: Unspecified error processing resource of type %.*s:%d:%d\n",res->path,res->lineno0,tnamec,tname,res->qual,res->rid);
      }
      return -2;
    }
  }
  if ((err=eggdev_pack_validate(romw))<0) return err;
  return 0;
}
