#include "eggdev_internal.h"
#include "opt/sfg/sfg.h"
#include <sys/time.h>

/* Current real time.
 */
 
double eggdev_now() {
  struct timeval tv={0};
  gettimeofday(&tv,0);
  return (double)tv.tv_sec+(double)tv.tv_usec/1000000.0;
}

/* PCM-out callback.
 */
 
static void eggdev_serve_pcm(int16_t *v,int c,struct hostio_audio *audio) {
  synth_updatei(v,c,eggdev.synth);
}

/* Attempt initialization with one driver type.
 * If this reports success, it must also create (eggdev.audio).
 */
 
static int eggdev_serve_init_audio_driver(
  const struct hostio_audio_type *type,
  const char *device,int devicec,
  int rate,int chanc,int buffer
) {

  // Don't bother instantiating the dummy driver; we can tolerate none instead.
  if (!strcmp(type->name,"dummy")) return -1;
  
  struct hostio_audio_delegate delegate={
    .cb_pcm_out=eggdev_serve_pcm,
  };
  char zdevice[256];
  if (devicec>=sizeof(zdevice)) return -1;
  memcpy(zdevice,device,devicec);
  zdevice[devicec]=0;
  struct hostio_audio_setup setup={
    .rate=rate,
    .chanc=chanc,
    .device=zdevice,
    .buffer_size=buffer,
  };
  if (!(eggdev.audio=hostio_audio_new(type,&delegate,&setup))) return -1;
  
  return 0;
}

/* Initialize or tear down audio driver and synthesizer.
 * Both are optional and may remain null.
 */
 
int eggdev_serve_init_audio(
  const char *driver,int driverc,
  const char *device,int devicec,
  int rate,int chanc,int buffer
) {
  if (!driver) driverc=0; else if (driverc<0) { driverc=0; while (driver[driverc]) driverc++; }
  if (!device) devicec=0; else if (devicec<0) { devicec=0; while (device[devicec]) devicec++; }

  /* No matter what else, even if they request what we already have, tear everything down first.
   */
  if (eggdev.audio) {
    hostio_audio_del(eggdev.audio);
    eggdev.audio=0;
  }
  if (eggdev.synth) {
    synth_del(eggdev.synth);
    eggdev.synth=0;
  }
  
  /* If the requested driver is "none", we're done.
   */
  if ((driverc==4)&&!memcmp(driver,"none",4)) return 0;
  
  /* Empty driver name, run through them all.
   * Otherwise, try only the named one.
   */
  if (driverc) {
    const struct hostio_audio_type *type=hostio_audio_type_by_name(driver,driverc);
    if (!type) return -1;
    eggdev_serve_init_audio_driver(type,device,devicec,rate,chanc,buffer);
  } else {
    const struct hostio_audio_type *type;
    int typep=0;
    for (;type=hostio_audio_type_by_index(typep);typep++) {
      if (eggdev_serve_init_audio_driver(type,device,devicec,rate,chanc,buffer)>=0) break;
    }
  }
  if (!eggdev.audio) return -1;
  
  if (!(eggdev.synth=synth_new(eggdev.audio->rate,eggdev.audio->chanc,&eggdev.drums_rom))) return -1;
  
  eggdev.audio->type->play(eggdev.audio,1);
  
  return 0;
}

/* Reload drums from an SFG text file.
 */
 
static int eggdev_serve_load_drums_cb(
  const char *src,int srcc,
  const char *id,int idc,int idn,
  const char *refname,int lineno0,
  void *userdata
) {
  struct rom *rom=userdata;
  
  if (rom->resc>=rom->resa) {
    int na=rom->resa+64;
    if (na>INT_MAX/sizeof(struct rom_res)) return -1;
    void *nv=realloc(rom->resv,sizeof(struct rom_res)*na);
    if (!nv) return -1;
    rom->resv=nv;
    rom->resa=na;
  }
  
  uint32_t fqrid=rom_pack_fqrid(EGG_RESTYPE_sound,0,idn);
  if (!fqrid) return 0;
  int lo=0,hi=rom->resc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (fqrid<rom->resv[ck].fqrid) hi=ck;
    else if (fqrid>rom->resv[ck].fqrid) lo=ck+1;
    else return 0; // Duplicate id!
  }
  
  struct sr_encoder bin={0};
  int err=sfg_compile(&bin,src,srcc,refname,lineno0);
  if (err<0) {
    sr_encoder_cleanup(&bin);
    return err;
  }
  
  struct rom_res *res=rom->resv+lo;
  memmove(res+1,res,sizeof(struct rom_res)*(rom->resc-lo));
  rom->resc++;
  res->fqrid=fqrid;
  res->v=bin.v; // HANDOFF (beware res->v is usually borrow, but we don't use it in the normal way)
  res->c=bin.c;
  
  return 0;
}
 
static int eggdev_serve_load_drums(const char *src,int srcc,const char *path) {
  
  // Compile into a temporary rom.
  // (serial) is unset, and each resource must be freed. That's not how rom usually works.
  struct rom tmprom={0};
  if (sfg_split(src,srcc,path,eggdev_serve_load_drums_cb,&tmprom)<0) return -1;
  
  // We're ready to touch synth now, so lock it.
  if (eggdev.audio->type->lock&&(eggdev.audio->type->lock(eggdev.audio)<0)) {
    struct rom_res *res=tmprom.resv;
    int i=tmprom.resc;
    for (;i-->0;res++) free((void*)res->v);
    return -1;
  }
  
  // Stop current song and send a System Reset, to hopefully ensure nothing is playing.
  synth_play_song(eggdev.synth,0,0,0,0);
  synth_event(eggdev.synth,0xff,0xff,0,0,0);
  synth_clear_cache(eggdev.synth);
  
  // Cleanup the shared rom, then copy the new one over it.
  if (eggdev.drums_rom.resv) {
    struct rom_res *res=eggdev.drums_rom.resv;
    int i=eggdev.drums_rom.resc;
    for (;i-->0;res++) free((void*)res->v);
  }
  memcpy(&eggdev.drums_rom,&tmprom,sizeof(tmprom));
  
  if (eggdev.audio->type->unlock) eggdev.audio->type->unlock(eggdev.audio);
  return 0;
}

/* Reload drums if necessary.
 * Call only if you've verified (audio,synth) both exist.
 * Do not call while locked.
 */
 
static int eggdev_serve_require_drums() {
  if (!eggdev.datapath) return 0;
  char path[1024];
  int pathc=snprintf(path,sizeof(path),"%.*s/sound/midi_drums.sfg",eggdev.datapathc,eggdev.datapath);//TODO configurable. Or discover it generically?
  if ((pathc<1)||(pathc>=sizeof(path))) return -1;
  int mtime=file_get_mtime(path);
  if (mtime<=eggdev.drums_mtime) return 0;
  void *serial=0;
  int serialc=file_read(&serial,path);
  if (serialc<0) return -1;
  int err=eggdev_serve_load_drums(serial,serialc,path);
  free(serial);
  if (err<0) return err;
  eggdev.drums_mtime=mtime;
  return 0;
}

/* Start playing a song.
 */
 
int eggdev_serve_play_song(struct sr_encoder *src) {
  if (!eggdev.audio||!eggdev.synth) return 0;
  if (!src||!src->c) {
    if (!eggdev.audio->type->lock||(eggdev.audio->type->lock(eggdev.audio)>=0)) {
      synth_play_song(eggdev.synth,0,0,0,0);
      if (eggdev.audio->type->unlock) eggdev.audio->type->unlock(eggdev.audio);
    }
  } else {
    eggdev_serve_require_drums();
    // Our compiler expects a romw_res; we can fake it.
    struct romw_res res={
      .tid=EGG_RESTYPE_song,
      .qual=0,
      .rid=1,
      .path="<http>",
      .pathc=6,
      .serial=src->v,
      .serialc=src->c,
    };
    src->v=0; // Handed off (src)
    src->c=0;
    src->a=0;
    if (eggdev_song_compile(0,&res)<0) {
      if (res.serial) free(res.serial);
      return -1;
    }
    if (!eggdev.audio->type->lock||(eggdev.audio->type->lock(eggdev.audio)>=0)) {
      synth_play_song_serial(eggdev.synth,res.serial,res.serialc,0,1);
      if (eggdev.audio->type->unlock) eggdev.audio->type->unlock(eggdev.audio);
    }
    if (res.serial) free(res.serial);
  }
  return 0;
}

/* Adjust channel headers in-flight.
 */
 
int eggdev_serve_adjust(const uint8_t *src,int srcc) {
  if (!src||(srcc<42)) return 0;
  if (!eggdev.audio||!eggdev.synth) return 0;
  if (!eggdev.audio->type->lock||(eggdev.audio->type->lock(eggdev.audio)>=0)) {
    synth_channels_switcheroo(eggdev.synth,src,srcc);
    if (eggdev.audio->type->unlock) eggdev.audio->type->unlock(eggdev.audio);
  }
  return 0;
}

/* Routine maintenance.
 */
 
int eggdev_serve_update() {
  if ((eggdev.playhead_clientc>0)&&eggdev.synth&&eggdev.audio) {
    double now=eggdev_now();
    if (now>=eggdev.playhead_update_time) {
      eggdev.playhead_update_time=now+0.100;
      double adjust=0.0;
      if (eggdev.audio->type->estimate_remaining_buffer) adjust=eggdev.audio->type->estimate_remaining_buffer(eggdev.audio);
      double beats=synth_get_playhead(eggdev.synth,adjust);
      if (beats>0.0) {
        double duration=synth_get_duration(eggdev.synth);
        uint16_t normu16=(uint16_t)((beats/duration)*65536.0);
        uint8_t pkt[2]={normu16>>8,normu16};
        int i=eggdev.playhead_clientc;
        while (i-->0) {
          struct http_websocket *ws=eggdev.playhead_clientv[i];
          http_websocket_send(ws,2,pkt,sizeof(pkt));
        }
      }
    }
  }
  return 0;
}
