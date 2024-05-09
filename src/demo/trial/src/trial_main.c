#include "egg/egg.h"

static double total_elapsed=0.0;
static int updatec=0;

void egg_client_quit() {
  egg_log("%s elapsed=%f updatec=%d",__func__,total_elapsed,updatec);
}

static int cb_res(int tid,int qual,int rid,int len,void *userdata) {
  egg_log("  %d:%d:%d, %d bytes",tid,qual,rid,len);
  return 0;
}

static int cb_decode_image(int tid,int qual,int rid,int len,void *userdata) {
  if (tid>EGG_RESTYPE_image) return 1;
  if (tid<EGG_RESTYPE_image) return 0;
  egg_log("%s %d:%d:%d len=%d... -------------------------------",__func__,tid,qual,rid,len);
  int w=0,h=0,stride=0,fmt=0;
  egg_image_get_header(&w,&h,&stride,&fmt,qual,rid);
  int pixelslen=stride*h;
  egg_log("w=%d h=%d stride=%d fmt=%d pixelslen=%d",w,h,stride,fmt,pixelslen);
  uint8_t v[65536];
  int c=egg_image_decode(v,sizeof(v),qual,rid);
  if (c!=pixelslen) {
    egg_log("!!!!!!!!!!! actual pixels len %d",c);
    return 0;
  }
  if (c>sizeof(v)) {
    egg_log("...decode looks ok but larger than our buffer");
    return 0;
  }
  int yi=h; if (yi>40) yi=40;
  const uint8_t *row=v;
  for (;yi-->0;row+=stride) {
    char tmp[80];
    int tmpc=0;
    int xi=stride; if (xi>40) xi=40;
    const uint8_t *p=row;
    for (;xi-->0;p++) {
      tmp[tmpc++]=" 123456789abcdef"[(*p)>>4];
      tmp[tmpc++]=" 123456789abcdef"[(*p)&15];
    }
    egg_log(">> %.*s",tmpc,tmp);
  }
  return 0;
}

int egg_client_init() {
  egg_log("%s",__func__);
  
  if (0) { // Dump all resources.
    egg_log("All resources:");
    int err=egg_res_for_each(cb_res,0);
    egg_log("egg_res_for_each => %d",err);
  }
  if (0) { // Dump content of a known custom resource.
    egg_log("Dumping privatetype:0:2, should be 'London':");
    char serial[32];
    int serialc=egg_res_get(serial,sizeof(serial),16,0,2);
    if ((serialc<1)||(serialc>sizeof(serial))) {
      egg_log("!!! Unexpected length %d",serialc);
    } else {
      egg_log("'%.*s'",serialc,serial);
    }
  }
  if (0) { // Read content of an image resource.
    int w=0,h=0,stride=0,fmt=0;
    egg_image_get_header(&w,&h,&stride,&fmt,0,2);
    if ((w<1)||(h<1)||(fmt!=EGG_TEX_FMT_A1)) {
      egg_log("image:0:2: Decode header failed or bad data: %d,%d,%d,%d",w,h,stride,fmt);
    } else {
      egg_log("image:0:2: w=%d h=%d stride=%d fmt=%d",w,h,stride,fmt);
      uint8_t v[1024]; // this image should be 594 (11*54)
      char rowbuf[128]; // 84 for this image
      int c=egg_image_decode(v,sizeof(v),0,2);
      if ((c!=h*stride)||(c>sizeof(v))||(w>sizeof(rowbuf))) {
        egg_log("image:0:2 decode failed. expected %d*%d==%d, got %d",h,stride,h*stride,c);
      } else {
        const uint8_t *row=v;
        int yi=h;
        for (;yi-->0;row+=stride) {
          const uint8_t *src=row;
          uint8_t srcmask=0x80;
          char *dst=rowbuf;
          int xi=w;
          for (;xi-->0;dst++) {
            if ((*src)&srcmask) *dst='X'; else *dst='.';
            if (srcmask==1) { srcmask=0x80; src++; }
            else srcmask>>=1;
          }
          egg_log(">> %.*s",w,rowbuf);
        }
      }
    }
  }
  if (1) { // Decode all images.
    egg_res_for_each(cb_decode_image,0);
  }
  
  return 0;
}

void egg_client_update(double elapsed) {
  updatec++;
  total_elapsed+=elapsed;
  if (updatec>=5) {
    egg_log("%s: Terminating for no reason.",__func__);
    egg_request_termination();
  }
}

void egg_client_render() {
}
