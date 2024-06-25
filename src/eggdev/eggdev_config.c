#include "eggdev_internal.h"

/* --help, specific topics.
 */
 
static void eggdev_print_help_commands() {
  fprintf(stderr,"\nUsage: %s COMMAND [OPTIONS]\n\n",eggdev.exename);
  fprintf(stderr,"Try `--help=COMMAND` for more detail.\n\n");
  fprintf(stderr,"      pack -oROM [--types=PATH] [INPUTS...]\n");
  fprintf(stderr,"    unpack -oDIR ROM [--types=PATH]\n");
  fprintf(stderr,"      list ROM [-fFORMAT] [--types=PATH]\n");
  fprintf(stderr,"       toc [INPUTS...] [--named-only] [--types=PATH]\n");
  fprintf(stderr,"   tocflag -oPATH [INPUTS...] [--types=PATH]\n");
  fprintf(stderr,"    bundle -oEXE|HTML --rom=ROM [--code=LIB]\n");
  fprintf(stderr,"  unbundle -oROM EXE|HTML\n");
  fprintf(stderr,"     serve [ROMS...] [--port=8080] [--external=0] [--htdocs=PATH]\n");
  fprintf(stderr,"\n");
}

static void eggdev_print_help_pack() {
  fprintf(stderr,"\nUsage: %s pack -oROM [--types=PATH] [INPUTS...]\n\n",eggdev.exename);
  fprintf(stderr,"Generate an Egg ROM file from loose inputs.\n");
  fprintf(stderr,"INPUTS can be files or directories to walk recursively.\n");
  fprintf(stderr,"We expect to find resources named '.../TYPE/ID[-NAME][.FORMAT]', for the most part.\n");
  fprintf(stderr,"See etc/doc/rom-format.md for more detail on paths.\n");
  fprintf(stderr,"Optional '--types' file contains lines of 'TID NAME' for your custom types.\n");
  fprintf(stderr,"Use command 'unpack' to reverse the process. Some information will be lost, eg resource names.\n");
  fprintf(stderr,"string, song, sound, and metadata resources get compiled during pack.\n");
  fprintf(stderr,"All other types (in particular wasm) must be in their final format before packing.\n");
  fprintf(stderr,"\n");
}

static void eggdev_print_help_unpack() {
  fprintf(stderr,"\nUsage: %s unpack -oDIRECTORY ROM [--types=PATH]\n\n",eggdev.exename);
  fprintf(stderr,"Extract all resources from a ROM file.\n");
  fprintf(stderr,"DIRECTORY will be created if it doesn't already exist.\n");
  fprintf(stderr,"The directory we produce can be given back to 'eggdev pack' and should reproduce the input ROM exactly.\n");
  fprintf(stderr,"Resource contents are not modified during unpack (eg songs will be in Egg format, not MIDI).\n");
  fprintf(stderr,"'--types' is entirely optional: We'll use numeric type IDs if necessary.\n");
  fprintf(stderr,"\n");
}

static void eggdev_print_help_list() {
  fprintf(stderr,"\nUsage: %s list ROM [-fFORMAT] [--types=PATH]\n\n",eggdev.exename);
  fprintf(stderr,"Print content of a ROM file.\n");
  fprintf(stderr,"FORMAT is 'default', 'machine', or 'summary'.\n");
  fprintf(stderr,"\n");
}

static void eggdev_print_help_toc() {
  fprintf(stderr,"\nUsage: %s toc [INPUTS...] [--named-only] [--types=PATH]\n\n",eggdev.exename);
  fprintf(stderr,"Generate a machine-readable list of resources, from your source files.\n");
  fprintf(stderr,"Provide the same INPUTS and '--types' that you would give to 'eggdev pack'.\n");
  fprintf(stderr,"This output can be used to generate a header mapping resource names to IDs.\n");
  fprintf(stderr,"\n");
}

static void eggdev_print_help_tocflag() {
  fprintf(stderr,"\nUsage: %s tocflag -oFLAGFILE [INPUTS...] [--types=PATH]\n\n",eggdev.exename);
  fprintf(stderr,"Rewrites the opaque FLAGFILE *only* if the set of resources has changed.\n");
  fprintf(stderr,"Addition, removal, and renames count as changes; content modification does not.\n");
  fprintf(stderr,"We do not track multi-resource files like string and sound. You should assume that any change to those is a TOC change.\n");
  fprintf(stderr,"You can use FLAGFILE as a signal to regenerate your TOC header (see 'eggdev toc').\n");
  fprintf(stderr,"\n");
}

static void eggdev_print_help_bundle() {
  fprintf(stderr,"\nUsage: %s bundle -oEXE|HTML --rom=ROM [--code=LIB]\n\n",eggdev.exename);
  fprintf(stderr,"Generate a self-contained executable or web page from an Egg ROM.\n");
  fprintf(stderr,"If output path ends '.html' or '.htm', we produce HTML. Otherwise we produce an executable.\n");
  fprintf(stderr,"If producing an executable, you may supply native code as a static library: --code=my-game.a\n");
  fprintf(stderr,"In that case, we strip any wasm from the ROM, do not include wasm-micro-runtime, and produce a 'true-native' executable.\n");
  fprintf(stderr,"\n");
}

static void eggdev_print_help_unbundle() {
  fprintf(stderr,"\nUsage: %s unbundle -oROM EXE|HTML\n\n",eggdev.exename);
  fprintf(stderr,"Extract the ROM file embedded in an executable or web page.\n");
  fprintf(stderr,"For true-native executables, the ROM will be invalid due to missing wasm. But you can still pull data out of it.\n");
  fprintf(stderr,"Don't expect it to work for executables built for some other architecture; only ones that were built by this Egg installation.\n");
  fprintf(stderr,"\n");
}

static void eggdev_print_help_serve() {
  fprintf(stderr,"\nUsage: %s serve [ROMS|OPTIONS...]\n\n",eggdev.exename);
  fprintf(stderr,"Launch an HTTP server for editing data or running the game.\n");
  fprintf(stderr,"Normally you supply one positional argument: The ROM file to play.\n");
  fprintf(stderr,"If the working directory contains 'Makefile', we invoke 'make' before serving the ROM each time, and respond 599 if make fails.\n");
  fprintf(stderr,"\nOPTIONS:\n");
  fprintf(stderr,"  --port=8080\n");
  fprintf(stderr,"  --external=0       Nonzero to serve on all interfaces instead of just localhost.\n");
  fprintf(stderr,"  --htdocs=PATH      Files for default service. egg/src/www or egg/src/editor\n");
  fprintf(stderr,"  --override=PATH    Anything under here is served in preference to --htdocs, for per-game overrides.\n");
  fprintf(stderr,"  --runtime=PATH     If --htdocs=egg/src/editor, supply egg/src/www here to enable the runtime too.\n");
  fprintf(stderr,"  --data=PATH        Directory of your data files to serve read/write, for editor.\n");
  fprintf(stderr,"  --types=PATH       Names of custom types.\n");
  fprintf(stderr,"\n");
}

/* --help, dispatch
 */
 
void eggdev_print_help(const char *topic,int topicc) {
  if (!topic) topicc=0; else if (topicc<0) { topicc=0; while (topic[topicc]) topicc++; }
  if (!topicc) eggdev_print_help_commands();
  else if ((topicc==1)&&(topic[0]=='1')) eggdev_print_help_commands(); // generic layer turns empty into "1"
  #define _(tag) else if ((topicc==sizeof(#tag)-1)&&!memcmp(topic,#tag,topicc)) eggdev_print_help_##tag();
  _(commands)
  _(pack)
  _(unpack)
  _(list)
  _(toc)
  _(tocflag)
  _(bundle)
  _(unbundle)
  _(serve)
  #undef _
  else {
    fprintf(stderr,"%s: Unknown help topic '%.*s'. Printing default instead.\n",eggdev.exename,topicc,topic);
    eggdev_print_help_commands();
  }
}

/* Key=value field.
 */
 
static int eggdev_config_kv(const char *k,int kc,const char *v,int vc) {
  if (!k) kc=0; else if (kc<0) { kc=0; while (k[kc]) kc++; }
  if (!v) vc=0; else if (vc<0) { vc=0; while (v[vc]) vc++; }
  
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
  
  if ((kc==4)&&!memcmp(k,"help",4)) {
    eggdev_print_help(v,vc);
    eggdev.command="help";
    return 0;
  }
  
  if ((kc==1)&&(k[0]=='o')) {
    if (eggdev.dstpath) {
      fprintf(stderr,"%s: Multiple output paths.\n",eggdev.exename);
      return -2;
    }
    eggdev.dstpath=v;
    return 0;
  }
  
  if ((kc==1)&&(k[0]=='f')) {
    if (eggdev.format) {
      fprintf(stderr,"%s: Multiple formats.\n",eggdev.exename);
      return -2;
    }
    eggdev.format=v;
    return 0;
  }
  
  if ((kc==3)&&!memcmp(k,"rom",3)) {
    if (eggdev.rompath) {
      fprintf(stderr,"%s: Multiple input ROMs.\n",eggdev.exename);
      return -2;
    }
    eggdev.rompath=v;
    return 0;
  }
  
  if ((kc==4)&&!memcmp(k,"code",4)) {
    if (eggdev.codepath) {
      fprintf(stderr,"%s: Multiple input libraries.\n",eggdev.exename);
      return -2;
    }
    eggdev.codepath=v;
    return 0;
  }
  
  if ((kc==4)&&!memcmp(k,"port",4)) {
    if (eggdev.port) {
      fprintf(stderr,"%s: Multiple ports.\n",eggdev.exename);
      return -2;
    }
    if ((sr_int_eval(&eggdev.port,v,vc)<2)||(eggdev.port<1)||(eggdev.port>65535)) {
      fprintf(stderr,"%s: Invalid port '%.*s'\n",eggdev.exename,vc,v);
      return -2;
    }
    return 0;
  }
  
  if ((kc==8)&&!memcmp(k,"external",8)) {
    if (sr_int_eval(&eggdev.external,v,vc)<2) {
      fprintf(stderr,"%s: Expected '0' or '1' for '--external'\n",eggdev.exename);
      return -2;
    }
    return 0;
  }
  
  #define REALPATH_STRING(fldname,optname) \
    if ((kc==sizeof(optname)-1)&&!memcmp(k,optname,sizeof(optname)-1)) { \
      if (eggdev.fldname) { \
        fprintf(stderr,"%s: Multiple --%s\n",eggdev.exename,optname); \
        return -2; \
      } \
      if (!(eggdev.fldname=realpath(v,0))) { \
        fprintf(stderr,"%s --%s '%s' not found\n",eggdev.exename,optname,v); \
        return -2; \
      } \
      eggdev.fldname##c=0; \
      while (eggdev.fldname[eggdev.fldname##c]) eggdev.fldname##c++; \
      return 0; \
    }
  REALPATH_STRING(htdocs,"htdocs")
  REALPATH_STRING(override,"override")
  REALPATH_STRING(runtime,"runtime")
  REALPATH_STRING(datapath,"data")
  #undef REALPATH_STRING
  
  if ((kc==5)&&!memcmp(k,"types",5)) {
    if (eggdev.typespath) {
      fprintf(stderr,"%s: Multiple types files.\n",eggdev.exename);
      return -2;
    }
    eggdev.typespath=v;
    return 0;
  }
  
  if ((kc==10)&&!memcmp(k,"named-only",10)) {
    if (sr_int_eval(&eggdev.named_only,v,vc)<2) {
      fprintf(stderr,"%s: Expected '0' or '1' for '--named-only'\n",eggdev.exename);
      return -2;
    }
    return 0;
  }
  
  fprintf(stderr,"%s: Unknown option '%.*s' = '%.*s'\n",eggdev.exename,kc,k,vc,v);
  return -2;
}

/* argv.
 */
 
static int eggdev_config_argv(int argc,char **argv) {
  int err;
  int argi=1; while (argi<argc) {
    const char *arg=argv[argi++];
    if (!arg||!arg[0]) continue;
    
    // Positional arguments: command ...srcpathv
    if (arg[0]!='-') {
      if (!eggdev.command) {
        eggdev.command=arg;
      } else {
        if (eggdev.srcpathc>=eggdev.srcpatha) {
          int na=eggdev.srcpatha+8;
          if (na>INT_MAX/sizeof(void*)) return -1;
          void *nv=realloc(eggdev.srcpathv,sizeof(void*)*na);
          if (!nv) return -1;
          eggdev.srcpathv=nv;
          eggdev.srcpatha=na;
        }
        eggdev.srcpathv[eggdev.srcpathc++]=arg;
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
      if ((err=eggdev_config_kv(&k,1,v,-1))<0) return err;
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
    if ((err=eggdev_config_kv(k,kc,v,-1))<0) return err;
    continue;
    
   _unexpected_:;
    fprintf(stderr,"%s: Unexpected argument '%s'.\n",eggdev.exename,arg);
    return -2;
  }
  return 0;
}

/* Configure, main entry point.
 */
 
int eggdev_configure(int argc,char **argv) {
  int err;
  eggdev.exename=((argc>=1)&&argv&&argv[0]&&argv[0][0])?argv[0]:"eggdev";
  if ((err=eggdev_config_argv(argc,argv))<0) return err;
  return 0;
}
