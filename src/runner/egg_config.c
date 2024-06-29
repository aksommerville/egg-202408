#include "egg_runner_internal.h"
#include "opt/serial/serial.h"
#include "opt/fs/fs.h"

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
    "  --config=PATH            Load from the given config file before processing any other args.\n"
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
    "  --audio-device=STRING    If required by driver.\n"
    "  --audio-driver=LIST      See below. First to start up wins.\n"
    "  --save=PATH              Save file. \"none\" to disable saving, or empty for default.\n"
    "  --store-limit=BYTES      Force save file to stay under this length. Default 1 MB.\n"
    "  --state=PATH             File for saved state. Press a key in-game to load or save. \"none\" to disable.\n"
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
  // "--config=PATH" is not handled here: We don't allow loading config files recursively. It's handled special at egg_config_files().
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
  INTOPT(store_limit,"store-limit",0,INT_MAX)
  BOOLOPT(ignore_required,"ignore-required")
  BOOLOPT(configure_input,"configure-input")
  STROPT(savestatepath,"state")
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

/* Prepare one of the "*path" fields based on user input, ROM path, and executable path.
 */
 
static char *egg_config_path_from_neighbor(const char *ref,const char *sfx) {
  if (!sfx) sfx="";
  int refc=0;
  while (ref[refc]) refc++;
  if ((refc>=4)&&!memcmp(ref+refc-4,".egg",4)) refc-=4;
  else if ((refc>=4)&&!memcmp(ref+refc-4,".exe",4)) refc-=4;
  char tmp[1024];
  int tmpc=snprintf(tmp,sizeof(tmp),"%.*s%s",refc,ref,sfx);
  if ((tmpc<1)||(tmpc>=sizeof(tmp))) return 0;
  return strdup(tmp);
}
 
static int egg_config_default_path(char **v,const char *sfx) {
  
  // If unset, we make something up.
  // If we're using an external ROM file, that's the only reference we'll use.
  if (!*v) {
    if (egg_romsrc==EGG_ROMSRC_EXTERNAL) {
      if (!egg.config.rompath) return -1;
      if (*v=egg_config_path_from_neighbor(egg.config.rompath,sfx)) return 0;
      //TODO Opportunity here to select some global path like ~/.egg/GAMEHASH.sfx
      return 0;
    }
    if (*v=egg_config_path_from_neighbor(egg.exename,sfx)) return 0;
    return -1;
  }
  
  // If it's literally "none", clear it.
  if (!strcmp(*v,"none")) {
    free(*v);
    *v=0;
    return 0;
  }
  
  // Otherwise the user provided an explicit path and we will honor it.
}

/* Load one config file.
 */
 
static int egg_config_file_text(const char *src,int srcc,const char *path) {
  struct sr_decoder decoder={.v=src,.c=srcc};
  int lineno=1,linec;
  const char *line;
  for (;(linec=sr_decode_line(&line,&decoder))>0;lineno++) {
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { linec--; line++; }
    if (!linec||(line[0]=='#')) continue;
    int linep=0;
    const char *k=line+linep,*v=0;
    int kc=0,vc=0;
    while ((linep<linec)&&(line[linep]!='=')&&(line[linep]!=':')) { linep++; kc++; }
    while (kc&&((unsigned char)k[kc-1]<=0x20)) kc--;
    while (kc&&(k[0]=='-')) { k++; kc--; } // Allow leading dashes for harmony with argv.
    if (linep<linec) {
      linep++;
      while ((linep<linec)&&((unsigned char)line[linep]<=0x20)) linep++;
      v=line+linep;
      vc=linec-linep;
    }
    if ((kc==4)&&!memcmp(k,"help",4)) {
      fprintf(stderr,"%s:%d: 'help' in a config file is not allowed. Run with '--help' on command line instead.\n",path,lineno);
      return -2;
    }
    if ((kc==15)&&!memcmp(k,"configure-input",15)) {
      fprintf(stderr,"%s:%d: 'configure-input' in a config file is not allowed. Run with '--configure-input' on command line instead.\n",path,lineno);
      return -2;
    }
    int err=egg_config_kv(k,kc,v,vc);
    if (err<0) {
      fprintf(stderr,"%s:%d: Error setting config field '%.*s' = '%.*s'\n",path,lineno,kc,k,vc,v);
      return -2;
    }
  }
  return 0;
}
 
static int egg_config_file(const char *path) {
  void *src=0;
  int srcc=file_read(&src,path);
  if (srcc<0) {
    fprintf(stderr,"%s: Failed to read file.\n",path);
    return -2;
  }
  int err=egg_config_file_text(src,srcc,path);
  free(src);
  return err;
}

/* Load from config files.
 * If the user supplied "--config=PATH", we honor it and remove it from argv.
 * We return the new argc.
 */
 
static int egg_config_files(int argc,char **argv) {
  int filec=0,argp=1;
  while (argp<argc) {
    const char *arg=argv[argp];
    if (!arg||memcmp(arg,"--config=",9)) {
      argp++;
      continue;
    }
    const char *path=arg+9;
    int err=egg_config_file(path);
    if (err<0) {
      if (err!=-2) fprintf(stderr,"%s: Unspecified error loading config file.\n",path);
      return -2;
    }
    fprintf(stderr,"%s: Loaded general config.\n",path);
    argc--;
    memmove(argv+argp,argv+argp+1,sizeof(void*)*(argc-argp));
    filec++;
  }
  if (!filec) {
    // No --config, so let's try some default locations.
    const char *builtinv[]={
      "./config",
      "~/.egg/egg.cfg",
    };
    int builtinc=sizeof(builtinv)/sizeof(void*);
    int builtini=0;
    for (;builtini<builtinc;builtini++) {
      const char *prepath=builtinv[builtini];
      char path[1024];
      int pathc=path_resolve(path,sizeof(path),prepath,-1);
      if ((pathc<1)||(pathc>=sizeof(path))) continue;
      if (file_get_type(path)!='f') continue;
      int err=egg_config_file(path);
      if (err<0) {
        if (err!=-2) fprintf(stderr,"%s: Unspecified error loading config file.\n",path);
        return -2;
      }
      fprintf(stderr,"%s: Loaded general config.\n",path);
      break;
    }
  }
  return argc;
}

/* Finalize.
 */
 
static int egg_config_finalize() {
  
  egg_config_default_path(&egg.config.storepath,".save");
  egg_config_default_path(&egg.config.savestatepath,".state");
  
  return 0;
}

/* Initial defaults.
 * Everything is zeroed implicitly, so only do the nonzero things.
 */
 
static void egg_config_init() {
  egg.config.store_limit=1<<20;
}

/* Configure, main entry point.
 */
 
int egg_configure(int argc,char **argv) {
  int err;
  egg.exename=((argc>=1)&&argv&&argv[0]&&argv[0][0])?argv[0]:"egg";
  egg_config_init();
  if ((argc=egg_config_files(argc,argv))<0) return argc;
  if ((err=egg_config_argv(argc,argv))<0) return err;
  if (egg.terminate) return 0;
  if ((err=egg_config_finalize())<0) return err;
  return 0;
}
