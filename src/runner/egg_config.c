#include "egg_runner_internal.h"

/* --help
 */
 
static void egg_print_help(const char *topic,int topicc) {
  if (egg_romsrc==EGG_ROMSRC_EXTERNAL) {
    fprintf(stderr,"\nUsage: %s [OPTIONS] ROMFILE\n\n",egg.exename);
  } else {
    fprintf(stderr,"\nUsage: %s [OPTIONS]\n\n",egg.exename);
  }
  fprintf(stderr,
    "OPTIONS:\n"
    "  --help             Print this message and exit.\n"
    "  --lang=en          Set language. Overrides LANG and LANGUAGE env vars.\n"
    "\n"
  );
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
  
  if ((kc==4)&&!memcmp(k,"lang",4)) {
    if ((egg.config.lang=rom_qual_eval(v,vc))<=0) {
      fprintf(stderr,"%s: Expected 2-letter ISO 639 language code for --lang, found '%.*s'\n",egg.exename,vc,v);
      return -2;
    }
    return 0;
  }
  
  //TODO Options.
  
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
      if (egg.config.rompath) {
        fprintf(stderr,"%s: Multiple ROM files.\n",egg.exename);
        return -2;
      }
      egg.config.rompath=arg;
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

/* Configure, main entry point.
 */
 
int egg_configure(int argc,char **argv) {
  int err;
  egg.exename=((argc>=1)&&argv&&argv[0]&&argv[0][0])?argv[0]:"egg";
  if ((err=egg_config_argv(argc,argv))<0) return err;
  if (egg.terminate) return 0;
  return 0;
}
