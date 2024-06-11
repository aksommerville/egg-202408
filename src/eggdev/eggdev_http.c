#include "eggdev_internal.h"
#include <unistd.h>

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

/* GET /rt/*
 */
 
static int eggdev_http_get_rt(struct http_xfer *req,struct http_xfer *rsp) {
  
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
  
  if (eggdev.runtime) {
    const char *rpath=0;
    int rpathc=http_xfer_get_path(&rpath,req);
    if ((rpathc>=4)&&!memcmp(rpath,"/rt/",4)) { rpath+=3; rpathc-=3; }
    if ((rpathc<1)||(rpath[0]!='/')) return http_xfer_set_status(rsp,404,"Not found");
    if (rpathc==1) {
      rpath="/index.html";
      rpathc=11;
    }
    char prepath[1024];
    int prepathc=snprintf(prepath,sizeof(prepath),"%s%.*s",eggdev.runtime,rpathc,rpath);
    if ((prepathc<1)||(prepathc>=sizeof(prepath))) return http_xfer_set_status(rsp,404,"Not found");
    char *lpath=realpath(prepath,0);
    if (!lpath) return http_xfer_set_status(rsp,404,"Not found");
    if (memcmp(lpath,eggdev.runtime,eggdev.runtimec)||(lpath[eggdev.runtimec]!='/')) {
      free(lpath);
      return http_xfer_set_status(rsp,404,"Not found");
    }
    int err=eggdev_serve_static(rsp,lpath);
    free(lpath);
    return err;
  }
  return http_xfer_set_status(rsp,404,"Not found");
}

/* GET /res/** -- Directories are supported; they'll return a JSON array of basenames.
 */
 
static int eggdev_serve_res_dir_cb(const char *path,const char *base,char type,void *userdata) {
  if (!type) type=file_get_type(path);
  if (type=='d') {
    char tmp[256];
    int tmpc=snprintf(tmp,sizeof(tmp),"%s/",base);
    if ((tmpc<1)||(tmpc>=sizeof(tmp))) return 0;
    sr_encode_json_string(userdata,0,0,tmp,tmpc);
  } else if (type=='f') {
    sr_encode_json_string(userdata,0,0,base,-1);
  }
  return 0;
}
 
static int eggdev_serve_res_dir(struct http_xfer *rsp,const char *path) {
  struct sr_encoder *body=http_xfer_get_body(rsp);
  if (!body) return -1;
  sr_encode_json_array_start(body,0,0);
  dir_read(path,eggdev_serve_res_dir_cb,body);
  if (sr_encode_json_end(body,0)<0) return http_xfer_set_status(rsp,500,"Internal error");
  sr_encode_u8(body,0x0a);
  return http_xfer_set_status(rsp,200,"OK");
}
 
static int eggdev_serve_res_get(struct http_xfer *rsp,const char *path) {
  char ftype=file_get_type(path);
  if (ftype=='d') return eggdev_serve_res_dir(rsp,path);
  if (ftype=='f') return eggdev_serve_static(rsp,path);
  return http_xfer_set_status(rsp,404,"Not found");
}

/* PUT /res/**
 */
 
static int eggdev_serve_res_put(struct http_xfer *rsp,const char *path,struct http_xfer *req) {
  const struct sr_encoder *body=http_xfer_get_body(req);
  if (file_write(path,body->v,body->c)<0) return http_xfer_set_status(rsp,500,"Write failed");
  return http_xfer_set_status(rsp,200,"OK");
}

/* DELETE /res/**
 */
 
static int eggdev_serve_res_delete(struct http_xfer *rsp,const char *path) {
  if (unlink(path)<0) return http_xfer_set_status(rsp,500,"Delete failed");
  return http_xfer_set_status(rsp,200,"OK");
}

/* * /res/**
 */
 
static int eggdev_http_res(struct http_xfer *req,struct http_xfer *rsp) {
  if (!eggdev.datapath) return http_xfer_set_status(rsp,404,"Not found");
  char method[16];
  int methodc=http_xfer_get_method(method,sizeof(method),req);
  if ((methodc<1)||(methodc>=sizeof(method))) return http_xfer_set_status(rsp,405,"Method not supported");
  
  const char *path=0;
  int pathc=http_xfer_get_path(&path,req);
  if (pathc<1) return http_xfer_set_status(rsp,404,"Not found");
  if ((pathc>=5)&&!memcmp(path,"/res/",5)) { path+=4; pathc-=4; }
  else if ((pathc==4)&&!memcmp(path,"/res",4)) { path="/"; pathc=1; }
  char prepath[1024];
  int prepathc=snprintf(prepath,sizeof(prepath),"%s%.*s",eggdev.datapath,pathc,path);
  if ((prepathc<1)||(prepathc>=sizeof(prepath))) return http_xfer_set_status(rsp,404,"Not found");
  char *lpath=realpath(prepath,0);//TODO I think this will prevent us from PUTting a new file. We need a way to do that.
  if (!lpath) return http_xfer_set_status(rsp,404,"Not found");
  if (memcmp(lpath,eggdev.datapath,eggdev.datapathc)||(lpath[eggdev.datapathc]&&(lpath[eggdev.datapathc]!='/'))) {
    free(lpath);
    return http_xfer_set_status(rsp,404,"Not found");
  }
  
  int err=-1;
  if ((methodc==3)&&!memcmp(method,"GET",3)) err=eggdev_serve_res_get(rsp,lpath);
  else if ((methodc==3)&&!memcmp(method,"PUT",3)) err=eggdev_serve_res_put(rsp,lpath,req);
  else if ((methodc==6)&&!memcmp(method,"DELETE",6)) err=eggdev_serve_res_delete(rsp,lpath);
  else err=http_xfer_set_status(rsp,405,"Method not supported");
  free(lpath);
  return err;
}

/* GET /res-all
 * Convenience that loads the entire resource set.
 * One could compose this whole thing from GET /res/**, but that would generate a lot of back-and-forth traffic.
 * And the editor definitely wants to do this every time it starts up.
 */
 
static int eggdev_http_get_res_all_dir(struct sr_encoder *encoder,const char *path);
static int eggdev_http_get_res_all_file(struct sr_encoder *encoder,const char *path);
static int eggdev_http_get_res_all_1(struct sr_encoder *encoder,const char *path);
 
static int eggdev_http_get_res_all_cb(const char *path,const char *base,char type,void *userdata) {
  if (type=='d') return eggdev_http_get_res_all_dir(userdata,path);
  if (type=='f') return eggdev_http_get_res_all_file(userdata,path);
  if (!type) return eggdev_http_get_res_all_1(userdata,path);
  return 0;
}
 
static int eggdev_http_get_res_all_dir(struct sr_encoder *encoder,const char *path) {
  const char *base=path;
  int i=0;
  for (;path[i];i++) if (path[i]=='/') base=path+i+1;
  int jsonctx=sr_encode_json_object_start(encoder,0,0);
  sr_encode_json_string(encoder,"name",4,base,-1);
  int arrayctx=sr_encode_json_array_start(encoder,"files",5);
  dir_read(path,eggdev_http_get_res_all_cb,encoder);
  sr_encode_json_end(encoder,arrayctx);
  return sr_encode_json_end(encoder,jsonctx);
}
 
static int eggdev_http_get_res_all_file(struct sr_encoder *encoder,const char *path) {
  const char *base=path;
  int i=0;
  for (;path[i];i++) if (path[i]=='/') base=path+i+1;
  void *serial=0;
  int serialc=file_read(&serial,path);
  if (serialc<0) return 0;
  int jsonctx=sr_encode_json_object_start(encoder,0,0);
  sr_encode_json_string(encoder,"name",4,base,-1);
  sr_encode_json_base64(encoder,"serial",6,serial,serialc);
  free(serial);
  return sr_encode_json_end(encoder,jsonctx);
}
 
static int eggdev_http_get_res_all_1(struct sr_encoder *encoder,const char *path) {
  char ftype=file_get_type(path);
  if (ftype=='d') return eggdev_http_get_res_all_dir(encoder,path);
  if (ftype=='f') return eggdev_http_get_res_all_file(encoder,path);
  return 0;
}
 
static int eggdev_http_get_res_all(struct http_xfer *req,struct http_xfer *rsp) {
  if (!eggdev.datapath) return http_xfer_set_status(rsp,404,"Not found");
  struct sr_encoder *encoder=http_xfer_get_body(rsp);
  if (eggdev_http_get_res_all_1(encoder,eggdev.datapath)<0) return http_xfer_set_status(rsp,500,"Internal error");
  http_xfer_set_header(rsp,"Content-Type",12,"application/json",16);
  return http_xfer_set_status(rsp,200,"OK");
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
    HTTP_METHOD_GET,"/rt/**",eggdev_http_get_rt,
    HTTP_METHOD_GET,"/res-all",eggdev_http_get_res_all,
    HTTP_METHOD_GET,"/res",eggdev_http_res,
    0,"/res/**",eggdev_http_res,
    HTTP_METHOD_GET,"",eggdev_http_get_default
  );
}
