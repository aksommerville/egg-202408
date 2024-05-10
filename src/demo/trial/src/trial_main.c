#include "egg/egg.h"

static double total_elapsed=0.0;
static int updatec=0;
static int texid;
static int imageid=0;
static double imageClock=0.0;
#define imageDisplayTime 2.0
static int screenw,screenh;
static int texid_glyphsheet;

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
  if (0) { // Decode all images.
    egg_res_for_each(cb_decode_image,0);
  }
  if (0) { // System odds and ends.
    egg_log("real time: %f",egg_time_real());
    int v[7]={0};
    egg_time_local(v,7);
    egg_log("local time: %04d-%02d-%02dT%02d:%02d:%02d.%03d",v[0],v[1],v[2],v[3],v[4],v[5],v[6]);
    int langc=egg_get_user_languages(v,7);
    egg_log("%d user languages:",langc);
    int i=0; for (;(i<langc)&&(i<7);i++) {
      char repr[2]={
        "012345abcdefghijklmnopqrstuvwxyz"[(v[i]>>5)&31],
        "012345abcdefghijklmnopqrstuvwxyz"[v[i]&31],
      };
      egg_log("  %.2s %d",repr,v[i]);
    }
  }
  
  egg_texture_get_header(&screenw,&screenh,0,1);
  if ((texid=egg_texture_new())<1) return -1;
  
  if (egg_texture_load_image(texid_glyphsheet=egg_texture_new(),0,1)<0) return -1;
  
  return 0;
}

void egg_client_update(double elapsed) {
  updatec++;
  total_elapsed+=elapsed;
  /**
  if (updatec>=5) {
    egg_log("%s: Terminating for no reason.",__func__);
    egg_request_termination();
  }
  /**/
  if ((imageClock-=elapsed)<=0.0) {
    imageClock+=imageDisplayTime;
    imageid++;
    if (egg_texture_load_image(texid,0,imageid)<0) {
      egg_log("%s: Failed to load image:0:%d, terminating",__FILE__,imageid);
      egg_request_termination();
    }
  }
}

void egg_client_render() {
  int w=0,h=0;
  egg_texture_get_header(&w,&h,0,texid);
  egg_draw_rect(1,0,0,screenw,screenh,0x4080c0ff);
  egg_draw_decal(1,texid,(screenw>>1)-(w>>1),(screenh>>1)-(h>>1),0,0,w,h,0);
  
  {
    static const struct egg_draw_tile vtxv[]={
      {100,200,'H',0},
      {108,200,'e',0},
      {116,200,'l',0},
      {124,200,'l',0},
      {132,200,'o',0},
      {148,200,'w',0},
      {156,200,'o',0},
      {164,200,'r',0},
      {172,200,'l',0},
      {180,200,'d',0},
      {188,200,'!',0},
    };
    egg_render_tint(0xffff00ff);
    egg_draw_tile(1,texid_glyphsheet,vtxv,sizeof(vtxv)/sizeof(vtxv[0]));
    egg_render_tint(0);
  }
  {
    static const struct egg_draw_line vtxv[]={
      { 10, 10,0xff,0x00,0x00,0xff},
      {100, 20,0x00,0xff,0x00,0xff},
      {100,100,0x00,0x00,0xff,0xff},
      { 10, 50,0xff,0xff,0xff,0xff},
    };
    egg_draw_line(1,vtxv,sizeof(vtxv)/sizeof(vtxv[0]));
  }
}
