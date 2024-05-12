#include "egg_runner_internal.h"
#include "opt/serial/serial.h"

/* --help
 */
 
static void egg_print_help(const char *topic,int topicc) {
  if (egg_romsrc==EGG_ROMSRC_EXTERNAL) {
    fprintf(stderr,"\nUsage: %s [OPTIONS] ROMFILE\n\n",egg.exename);
  } else {
    fprintf(stderr,"\nUsage: %s [OPTIONS]\n\n",egg.exename);
  }
  fprintf(stderr,
    "Omit all options for sensible defaults, that's usually sufficient.\n"
    "\n"
    "OPTIONS:\n"
    "  --help                   Print this message and exit.\n"
    "  --lang=en                Set language. Overrides LANG and LANGUAGE env vars.\n"
    "  --window=WxH             Set initial window size.\n"
    "  --fullscreen             Start fullscreen.\n"
    "  --video-device=STRING    If required by driver.\n"
    "  --video-driver=LIST      See below. First to start up wins.\n"
    "  --input-path=STRING      If required by driver.\n"
    "  --input-drivers=LIST     See below. We'll try to open all of them.\n"
    "  --audio-rate=HZ          Suggested audio output rate.\n"
    "  --audio-chanc=1|2        Suggested audio channel count.\n"
    "  --stereo                 --audio-chanc=2\n"
    "  --mono                   --audio-chanc=1\n"
    "  --audio-buffer=INT       If required by driver.\n"
    "  --audio-driver=LIST      See below. First to start up wins.\n"
    "  --save=PATH              Save file. \"none\" to disable saving, or empty for default.\n"
    "  --configure-input        Launch in a special mode to map a joystick.\n"
  );
  if (egg_romsrc!=EGG_ROMSRC_NATIVE) {
    fprintf(stderr,"  --ignore-required        Try to launch even if ROM's stated requirements can't be met.\n");
  }
  fprintf(stderr,"\n");
  #define LISTDRIVERS(type) { \
    fprintf(stderr,"Available %s drivers:\n",#type); \
    int p=0; for (;;p++) { \
      const struct hostio_##type##_type *driver=hostio_##type##_type_by_index(p); \
      if (!driver) break; \
      fprintf(stderr," %12s: %s\n",driver->name,driver->desc); \
    } \
    fprintf(stderr,"\n"); \
  }
  LISTDRIVERS(video)
  LISTDRIVERS(audio)
  LISTDRIVERS(input)
  #undef LISTDRIVERS
}

/* Set string field.
 */
 
static int egg_config_string(char **dstpp,const char *src,int srcc,int assert_first) {
  if (assert_first&&*dstpp) return -1;
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  char *nv=0;
  if (srcc) {
    if (!(nv=malloc(srcc+1))) return -1;
    memcpy(nv,src,srcc);
    nv[srcc]=0;
  }
  if (*dstpp) free(*dstpp);
  *dstpp=nv;
  return 0;
}

/* Key=value field.
 */
 
static int egg_config_kv(const char *k,int kc,const char *v,int vc) {
  if (!k) kc=0; else if (kc<0) { kc=0; while (k[kc]) kc++; }
  if (!v) vc=0; else if (vc<0) { vc=0; while (v[vc]) vc++; }
  
  if ((kc==4)&&!memcmp(k,"help",4)) {
    egg_print_help(v,vc);
    egg.terminate=1;
    return 0;
  }
  
  if (!v) {
    if ((kc>3)&&!memcmp(k,"no-",3)) {
      k+=3;
      kc-=3;
      v="0";
      vc=1;
    } else {
      v="1";
      vc=1;
    }
  }
  
  if ((kc==4)&&!memcmp(k,"lang",4)) {
    if ((egg.config.lang=rom_qual_eval(v,vc))<=0) {
      fprintf(stderr,"%s: Expected 2-letter ISO 639 language code for --lang, found '%.*s'\n",egg.exename,vc,v);
      return -2;
    }
    return 0;
  }
  
  if ((kc==6)&&!memcmp(k,"window",6)) {
    int sepp=-1,vp=0;
    for (;vp<vc;vp++) if (v[vp]=='x') { sepp=vp; break; }
    if (
      (sepp<0)||
      (sr_int_eval(&egg.config.windoww,v,sepp)<2)||
      (sr_int_eval(&egg.config.windowh,v+sepp+1,vc-sepp-1)<2)||
      (egg.config.windoww<1)||(egg.config.windowh<1)||(egg.config.windoww>0x7fff)||(egg.config.windowh>0x7fff)
    ) {
      fprintf(stderr,"%s: Expected 'WxH' for '--window', found '%.*s'\n",egg.exename,vc,v);
      return -2;
    }
    return 0;
  }
  
  if ((kc==6)&&!memcmp(k,"stereo",6)) {
    egg.config.audio_chanc=2;
    return 0;
  }
  if ((kc==4)&&!memcmp(k,"mono",4)) {
    egg.config.audio_chanc=1;
    return 0;
  }
  
  #define BOOLOPT(fldname,optname) if ((kc==sizeof(optname)-1)&&!memcmp(k,optname,kc)) { \
    if (sr_int_eval(&egg.config.fldname,v,vc)<2) { \
      fprintf(stderr,"%s: Expected 0 or 1 for '--%.*s', found '%.*s'\n",egg.exename,kc,k,vc,v); \
      return -2; \
    } \
    egg.config.fldname=egg.config.fldname?1:0; \
    return 0; \
  }
  #define INTOPT(fldname,optname,lo,hi) if ((kc==sizeof(optname)-1)&&!memcmp(k,optname,kc)) { \
    int n; \
    if ((sr_int_eval(&n,v,vc)<2)||(n<lo)||(n>hi)) { \
      if ((lo==INT_MIN)&&(hi==INT_MAX)) fprintf(stderr,"%s: Expected integer for '--%.*s', found '%.*s'\n",egg.exename,kc,k,vc,v); \
      else if (lo==INT_MIN) fprintf(stderr,"%s: Expected integer up to %d for '--%.*s', found '%.*s'\n",egg.exename,hi,kc,k,vc,v); \
      else if (hi==INT_MAX) fprintf(stderr,"%s: Expected integer no less than %d for '--%.*s', found '%.*s'\n",egg.exename,lo,kc,k,vc,v); \
      else fprintf(stderr,"%s: Expected integer in %d..%d for '--%.*s', found '%.*s'\n",egg.exename,lo,hi,kc,k,vc,v); \
      return -2; \
    } \
    egg.config.fldname=n; \
    return 0; \
  }
  #define STROPT(fldname,optname) if ((kc==sizeof(optname)-1)&&!memcmp(k,optname,kc)) { \
    if (egg_config_string(&egg.config.fldname,v,vc,0)<0) return -1; \
    return 0; \
  }
  BOOLOPT(fullscreen,"fullscreen")
  STROPT(video_device,"video-device")
  STROPT(video_driver,"video-driver")
  STROPT(input_path,"input-path")
  STROPT(input_drivers,"input-drivers")
  INTOPT(audio_rate,"audio-rate",200,200000)
  INTOPT(audio_chanc,"audio-chanc",1,8)
  INTOPT(audio_buffer,"audio-buffer",0,INT_MAX)
  STROPT(audio_device,"audio-device")
  STROPT(audio_driver,"audio-driver")
  STROPT(storepath,"save")
  BOOLOPT(ignore_required,"ignore-required")
  BOOLOPT(configure_input,"configure-input")
  #undef BOOLOPT
  #undef INTOPT
  #undef STROPT
  
  fprintf(stderr,"%s: Unknown option '%.*s' = '%.*s'\n",egg.exename,kc,k,vc,v);
  return -2;
}

/* argv.
 */
 
static int egg_config_argv(int argc,char **argv) {
  int err;
  int argi=1; while (argi<argc) {
    const char *arg=argv[argi++];
    if (!arg||!arg[0]) continue;
    
    // Positional arguments: rompath
    if (arg[0]!='-') {
      if (egg_romsrc!=EGG_ROMSRC_EXTERNAL) goto _unexpected_;
      if (egg_config_string(&egg.config.rompath,arg,-1,1)<0) {
        fprintf(stderr,"%s: Multiple ROM files.\n",egg.exename);
        return -2;
      }
      continue;
    }
    
    // Dash alone: undefined
    if (!arg[1]) goto _unexpected_;
    
    // Single dash: "-kVV" or "-k VV"
    if (arg[1]!='-') {
      char k=arg[1];
      const char *v=0;
      if (arg[2]) v=arg+2;
      else if ((argi<argc)&&argv[argi]&&argv[argi][0]&&(argv[argi][0]!='-')) v=argv[argi++];
      if ((err=egg_config_kv(&k,1,v,-1))<0) return err;
      continue;
    }
    
    // Double dash alone, or more than two dashes: undefined.
    if (!arg[2]) goto _unexpected_;
    if (arg[2]=='-') goto _unexpected_;
    
    // Double dash: "--kk=VV" or "--kk VV"
    const char *k=arg+2;
    int kc=0;
    while (k[kc]&&(k[kc]!='=')) kc++;
    const char *v=0;
    if (k[kc]=='=') v=k+kc+1;
    else if ((argi<argc)&&argv[argi]&&argv[argi][0]&&(argv[argi][0]!='-')) v=argv[argi++];
    if ((err=egg_config_kv(k,kc,v,-1))<0) return err;
    continue;
    
   _unexpected_:;
    fprintf(stderr,"%s: Unexpected argument '%s'.\n",egg.exename,arg);
    return -2;
  }
  return 0;
}

/* Make up storepath.
 */
 
static char *egg_config_neighbor_storepath(const char *rompath) {
  // Append ".save" to the ROM's path, but first if it exists, trim ".egg".
  int rompathc=0;
  while (rompath[rompathc]) rompathc++;
  if ((rompathc>=4)&&!memcmp(rompath+rompathc-4,".egg",4)) rompathc-=4;
  char tmp[1024];
  int tmpc=snprintf(tmp,sizeof(tmp),"%.*s.save",rompathc,rompath);
  if ((tmpc<1)||(tmpc>=sizeof(tmp))) return 0;
  return strdup(tmp);
}

static char *egg_config_exe_storepath(const char *exename) {
  int sepp=-1,i=0;
  for (;exename[i];i++) if (exename[i]=='/') sepp=i;
  if (sepp>=0) {
    char tmp[1024];
    int tmpc=snprintf(tmp,sizeof(tmp),"%.*s/save",sepp,exename);
    if ((tmpc<1)||(tmpc>=sizeof(tmp))) return 0;
    return strdup(tmp);
  }
  // Executable name has no slashes, so it must have been found on PATH or something.
  // We could guess a path globally i guess, like "~/.egg/GAMEHASH.save", something like that?
  return 0;
}

/* Finalize.
 */
 
static int egg_config_finalize() {

  // (storepath) unset means make one up. "none", we should set it null to disable saving.
  if (!egg.config.storepath) {
    if ((egg_romsrc==EGG_ROMSRC_EXTERNAL)&&egg.config.rompath) {
      egg.config.storepath=egg_config_neighbor_storepath(egg.config.rompath);
    } else {
      egg.config.storepath=egg_config_exe_storepath(egg.exename);
    }
  } else if (!strcmp(egg.config.storepath,"none")) {
    free(egg.config.storepath);
    egg.config.storepath=0;
  }
  
  return 0;
}

/* Configure, main entry point.
 */
 
int egg_configure(int argc,char **argv) {
  int err;
  egg.exename=((argc>=1)&&argv&&argv[0]&&argv[0][0])?argv[0]:"egg";
  //TODO config file?
  if ((err=egg_config_argv(argc,argv))<0) return err;
  if (egg.terminate) return 0;
  if ((err=egg_config_finalize())<0) return err;
  return 0;
}
