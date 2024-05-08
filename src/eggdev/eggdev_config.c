#include "eggdev_internal.h"

/* --help
 */
 
void eggdev_print_help(const char *topic,int topicc) {
  fprintf(stderr,"\nUsage: %s COMMAND [OPTIONS]\n\n",eggdev.exename);
  fprintf(stderr,
    "%s pack -oROM [--types=PATH] [INPUTS...]\n"
    "  Generate a ROM file from the provided directories and files.\n"
    "  '--types=PATH' is a text file of 'TID NAME', for your custom types.\n"
    "\n",eggdev.exename
  );
  fprintf(stderr,
    "%s unpack -oDIRECTORY ROM [--types=PATH]\n"
    "  Extract resources each to its own file, under the provided DIRECTORY, which we create.\n"
    "\n",eggdev.exename
  );
  fprintf(stderr,
    "%s list ROM [-fFORMAT] [--types=PATH]\n"
    "  List contents of a ROM file.\n"
    "  FORMAT: default machine summary\n"
    "\n",eggdev.exename
  );
  fprintf(stderr,
    "%s toc [INPUTS...] [--named-only] [--types=PATH]\n"
    "  Generate a machine-readable list of resources under the given directories.\n"
    "  Helpful for generating a header to map resource names to IDs.\n"
    "\n",eggdev.exename
  );
  fprintf(stderr,
    "%s tocflag -oFLAGFILE [INPUTS...] [--types=PATH]\n"
    "  Rewrite FLAGFILE *only* if the set of input files has changed.\n"
    "  This means addition, removal, or renaming. Not changes to file content.\n"
    "  Intended to produce flag files for make, to ask whether a TOC needs regenerated.\n"
    "  You'll need to add multi-resource string and sound files as a prereq.\n"
    "\n",eggdev.exename
  );
  fprintf(stderr,
    "%s bundle -oEXE --rom=ROM [--code=LIB]\n"
    "  Generate a native executable from a ROM file and optional native code (static library).\n"
    "\n",eggdev.exename
  );
  fprintf(stderr,
    "%s unbundle -oROM EXE\n"
    "  Extract ROM file from a bundled executable.\n"
    "\n",eggdev.exename
  );
  fprintf(stderr,
    "%s serve [ROMS...] [--port=8080] [--external=0] [--htdocs=PATH]\n"
    "  Launch a web server providing access to the given ROM files.\n"
    "  If there is a Makefile in the working directory, we invoke 'make' before serving a ROM each time.\n"
    "\n",eggdev.exename
  );
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
  
  if ((kc==6)&&!memcmp(k,"htdocs",6)) {
    if (eggdev.htdocs) {
      fprintf(stderr,"%s: Multiple htdocs\n",eggdev.exename);
      return -2;
    }
    if (!(eggdev.htdocs=realpath(v,0))) {
      fprintf(stderr,"%s: htdocs '%s' not found\n",eggdev.exename,v);
      return -2;
    }
    eggdev.htdocsc=0;
    while (eggdev.htdocs[eggdev.htdocsc]) eggdev.htdocsc++;
    return 0;
  }
  
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
