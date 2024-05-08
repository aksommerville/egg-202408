#include "eggdev_internal.h"

/* Guess MIME type.
 * We'll use the path exclusively if we recognize it, and we never fail to return something valid.
 */
 
static const char *eggdev_guess_mime_type(const char *path,const void *src,int srcc) {

  // Look for known path suffixes.
  // Only take content after the last dot. We can't detect eg ".tar.gz", I doubt it will matter.
  if (path) {
    char sfx[16];
    int sfxc=0,reading=0,pathp=0;
    for (;path[pathp];pathp++) {
      if (path[pathp]=='/') {
        sfxc=0;
        reading=0;
      } else if (path[pathp]=='.') {
        sfxc=0;
        reading=1;
      } else if (reading) {
        if (sfxc>=sizeof(sfx)) {
          sfxc=0;
          reading=0;
        } else {
          if ((path[pathp]>='A')&&(path[pathp]<='Z')) sfx[sfxc++]=path[pathp]+0x20;
          else sfx[sfxc++]=path[pathp];
        }
      }
    }
    switch (sfxc) {
      case 2: {
          if (!memcmp(sfx,"js",2)) return "application/javascript";
        } break;
      case 3: {
          if (!memcmp(sfx,"ico",3)) return "image/x-icon";
          if (!memcmp(sfx,"png",3)) return "image/png";
          if (!memcmp(sfx,"gif",3)) return "image/gif";
          if (!memcmp(sfx,"css",3)) return "text/css";
          if (!memcmp(sfx,"egg",3)) return "application/x-egg-rom";
          if (!memcmp(sfx,"txt",3)) return "text/plain";
          if (!memcmp(sfx,"mid",3)) return "audio/midi";
          if (!memcmp(sfx,"wav",3)) return "audio/wave";//TODO verify
        } break;
      case 4: {
          if (!memcmp(sfx,"html",4)) return "text/html";
          if (!memcmp(sfx,"jpeg",4)) return "image/jpeg";
          if (!memcmp(sfx,"json",4)) return "application/json";
        } break;
    }
  }
  
  // Look for unambiguous signatures.
  if (src) {
    if ((srcc>=8)&&!memcmp(src,"\x89PNG\r\n\x1a\n",8)) return "image/png";
    if ((srcc>=4)&&!memcmp(src,"\xea\x00\xff\xff",4)) return "application/x-egg-rom";
  }

  // Finally "text/plain" if the first 256 bytes are ASCII, or "application/octet-stream" otherwise.
  if (!src) return "application/octet-stream";
  const uint8_t *SRC=src;
  int i=srcc;
  if (i>256) i=256;
  while (i-->0) {
    if ((SRC[i]>=0x20)&&(SRC[i]<=0x7e)) continue;
    if (SRC[i]==0x0a) continue;
    if (SRC[i]==0x0d) continue;
    if (SRC[i]==0x09) continue;
    return "application/octet-stream";
  }
  return "text/plain";
}

/* Serve static file.
 * Caller provides a local path which we presume safe and valid.
 */
 
static int eggdev_serve_static(struct http_xfer *rsp,const char *path) {
  void *src=0;
  int srcc=file_read(&src,path);
  if (srcc<0) return http_xfer_set_status(rsp,404,"Not found");
  sr_encode_raw(http_xfer_get_body(rsp),src,srcc);
  http_xfer_set_header(rsp,"Content-Type",12,eggdev_guess_mime_type(path,src,srcc),-1);
  free(src);
  return http_xfer_set_status(rsp,200,"OK");
}

/* Local path of a ROM file provided at the command line, if it matches this request.
 */
 
static const char *eggdev_get_rom_path_for_request(struct http_xfer *req) {

  // Only consider basename of the request.
  const char *rpath=0;
  int rpathc=http_xfer_get_path(&rpath,req);
  if (rpathc<1) return 0;
  int slashp=-1,i=0;
  for (;i<rpathc;i++) if (rpath[i]=='/') slashp=i;
  if (slashp>=0) {
    slashp++;
    rpath+=slashp;
    rpathc-=slashp;
  }
  
  const char **rompathv=eggdev.srcpathv;
  for (i=eggdev.srcpathc;i-->0;rompathv++) {
    const char *lpath=*rompathv;
    const char *lbase=lpath;
    int lpp=0,lbasec=0;
    for (;lpath[lpp];lpp++) {
      if (lpath[lpp]=='/') {
        lbase=lpath+lpp+1;
        lbasec=0;
      } else {
        lbasec++;
      }
    }
    if (lbasec!=rpathc) continue;
    if (memcmp(lbase,rpath,lbasec)) continue;
    return lpath;
  }
  
  return 0;
}

/* GET *
 */
 
static int eggdev_http_get_default(struct http_xfer *req,struct http_xfer *rsp) {
  
  const char *rompath=eggdev_get_rom_path_for_request(req);
  if (rompath) {
    if (eggdev.has_wd_makefile) {
      struct sr_encoder output={0};
      int err=eggdev_command_sync(&output,"make 2>&1");
      if (err) { // Positive or negative.
        sr_encode_raw(http_xfer_get_body(rsp),output.v,output.c);
        http_xfer_set_header(rsp,"Content-Type",12,"text/plain",10);
        sr_encoder_cleanup(&output);
        return http_xfer_set_status(rsp,599,"make failed");
      }
      sr_encoder_cleanup(&output);
    }
    return eggdev_serve_static(rsp,rompath);
  }
  
  if (eggdev.htdocs) {
    const char *rpath=0;
    int rpathc=http_xfer_get_path(&rpath,req);
    if ((rpathc<1)||(rpath[0]!='/')) return http_xfer_set_status(rsp,404,"Not found");
    if (rpathc==1) {
      rpath="/index.html";
      rpathc=11;
    }
    char prepath[1024];
    int prepathc=snprintf(prepath,sizeof(prepath),"%s%.*s",eggdev.htdocs,rpathc,rpath);
    if ((prepathc<1)||(prepathc>=sizeof(prepath))) return http_xfer_set_status(rsp,404,"Not found");
    char *lpath=realpath(prepath,0);
    if (!lpath) return http_xfer_set_status(rsp,404,"Not found");
    if (memcmp(lpath,eggdev.htdocs,eggdev.htdocsc)||(lpath[eggdev.htdocsc]!='/')) {
      free(lpath);
      return http_xfer_set_status(rsp,404,"Not found");
    }
    int err=eggdev_serve_static(rsp,lpath);
    free(lpath);
    return err;
  }
  
  return http_xfer_set_status(rsp,404,"Not found");
}

/* GET /api/roms
 */
 
static int eggdev_http_get_roms(struct http_xfer *req,struct http_xfer *rsp) {
  struct sr_encoder *dst=http_xfer_get_body(rsp);
  sr_encode_json_array_start(dst,0,0);
  const char **pathv=eggdev.srcpathv;
  int i=eggdev.srcpathc;
  for (;i-->0;pathv++) {
    const char *base=*pathv;
    int basec=0;
    int j=0; for (;(*pathv)[j];j++) {
      if ((*pathv)[j]=='/') {
        base=(*pathv)+j+1;
        basec=0;
      } else {
        basec++;
      }
    }
    sr_encode_json_string(dst,0,0,base,basec);
  }
  if (sr_encode_json_end(dst,0)<0) {
    dst->c=0;
    return http_xfer_set_status(rsp,500,"Failed to encode ROMs list");
  }
  sr_encode_u8(dst,0x0a); // just to be polite
  http_xfer_set_header(rsp,"Content-Type",12,"application/json",16);
  return http_xfer_set_status(rsp,200,"OK");
}

/* Serve, main entry point.
 */
 
int eggdev_http_serve(struct http_xfer *req,struct http_xfer *rsp,void *userdata) {
  return http_dispatch(req,rsp,
    HTTP_METHOD_GET,"/api/roms",eggdev_http_get_roms,
    HTTP_METHOD_GET,"",eggdev_http_get_default
  );
}
