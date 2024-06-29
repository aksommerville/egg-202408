#include "egg_runner_internal.h"
#include "egg_inmap.h"
#include "opt/fs/fs.h"
#include "opt/serial/serial.h"

/* Cleanup.
 */
 
static void egg_inmap_rules_del(struct egg_inmap_rules *rules) {
  if (!rules) return;
  if (rules->name) free(rules->name);
  if (rules->buttonv) free(rules->buttonv);
  free(rules);
}
 
void egg_inmap_cleanup(struct egg_inmap *inmap) {
  if (inmap->rulesv) {
    while (inmap->rulesc-->0) egg_inmap_rules_del(inmap->rulesv[inmap->rulesc]);
    free(inmap->rulesv);
  }
  if (inmap->cfgpath) free(inmap->cfgpath);
  memset(inmap,0,sizeof(struct egg_inmap));
}

/* Button names.
 */
 
static int egg_inmap_btnid_eval(int *dst,const char *src,int srcc) {
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  char norm[16];
  if (srcc>sizeof(norm)) return -1;
  int i=srcc; while (i-->0) {
    if ((src[i]>='a')&&(src[i]<='z')) norm[i]=src[i]-0x20;
    else norm[i]=src[i];
  }
  switch (srcc) {
    #define JOYBTN(tag) if (!memcmp(src,#tag,srcc)) { *dst=EGG_JOYBTN_##tag; return 0; }
    #define INMAP_BTN(tag) if (!memcmp(src,#tag,srcc)) { *dst=EGG_INMAP_BTN_##tag; return 0; }
    #define ALIAS(tag,joytag) if (!memcmp(src,#tag,srcc)) { *dst=EGG_JOYBTN_##joytag; return 0; }
    case 2: {
        JOYBTN(LX)
        JOYBTN(LY)
        JOYBTN(RX)
        JOYBTN(RY)
        JOYBTN(L1)
        JOYBTN(R1)
        JOYBTN(L2)
        JOYBTN(R2)
        JOYBTN(LP)
        JOYBTN(RP)
        JOYBTN(UP)
      } break;
    case 3: {
        INMAP_BTN(NLX)
        INMAP_BTN(NLY)
        INMAP_BTN(NRX)
        INMAP_BTN(NRY)
      } break;
    case 4: {
        JOYBTN(AUX1)
        JOYBTN(AUX2)
        JOYBTN(AUX3)
        JOYBTN(DOWN)
        JOYBTN(LEFT)
        JOYBTN(EAST)
        JOYBTN(WEST)
        INMAP_BTN(HORZ)
        INMAP_BTN(VERT)
        INMAP_BTN(DPAD)
        INMAP_BTN(QUIT)
        INMAP_BTN(SAVE)
        INMAP_BTN(LOAD)
        ALIAS(MENU,AUX3)
      } break;
    case 5: {
        JOYBTN(SOUTH)
        JOYBTN(NORTH)
        JOYBTN(RIGHT)
        ALIAS(START,AUX1)
        ALIAS(HEART,AUX3)
        INMAP_BTN(NHORZ)
        INMAP_BTN(NVERT)
        INMAP_BTN(PAUSE)
      } break;
    case 6: {
        ALIAS(SELECT,AUX2)
      } break;
    case 9: {
        INMAP_BTN(SCREENCAP)
      } break;
    case 10: {
        INMAP_BTN(FULLSCREEN)
      } break;
    #undef JOYBTN
    #undef INMAP_BTN
    #undef ALIAS
  }
  if ((sr_int_eval(dst,src,srcc)>=2)&&(*dst>0)) return 0;
  return -1;
}

static const char *egg_inmap_btnid_repr(int btnid) {
  switch (btnid) {
    #define _(tag) case EGG_JOYBTN_##tag: return #tag;
    _(LX)
    _(LY)
    _(RX)
    _(RY)
    _(SOUTH)
    _(EAST)
    _(WEST)
    _(NORTH)
    _(L1)
    _(R1)
    _(L2)
    _(R2)
    _(AUX2)
    _(AUX1)
    _(LP)
    _(RP)
    _(UP)
    _(DOWN)
    _(LEFT)
    _(RIGHT)
    _(AUX3)
    #undef _
    #define _(tag) case EGG_INMAP_BTN_##tag: return #tag;
    _(HORZ)
    _(VERT)
    _(DPAD)
    _(NHORZ)
    _(NVERT)
    _(NLX)
    _(NLY)
    _(NRX)
    _(NRY)
    _(QUIT)
    _(SCREENCAP)
    _(SAVE)
    _(LOAD)
    _(PAUSE)
    _(FULLSCREEN)
    #undef _
  }
  return 0;
}

/* Rules list.
 */
 
static struct egg_inmap_rules *egg_inmap_add_rules(struct egg_inmap *inmap) {
  if (inmap->rulesc>=inmap->rulesa) {
    int na=inmap->rulesa+16;
    if (na>INT_MAX/sizeof(void*)) return 0;
    void *nv=realloc(inmap->rulesv,sizeof(void*)*na);
    if (!nv) return 0;
    inmap->rulesv=nv;
    inmap->rulesa=na;
  }
  struct egg_inmap_rules *rules=calloc(1,sizeof(struct egg_inmap_rules));
  if (!rules) return 0;
  inmap->rulesv[inmap->rulesc++]=rules;
  return rules;
}

static void egg_inmap_remove_rules(struct egg_inmap *inmap,struct egg_inmap_rules *rules) {
  int i=inmap->rulesc;
  while (i-->0) {
    if (inmap->rulesv[i]==rules) {
      inmap->rulesc--;
      memmove(inmap->rulesv+i,inmap->rulesv+i+1,sizeof(void*)*(inmap->rulesc-i));
      egg_inmap_rules_del(rules);
      return;
    }
  }
}

/* Parse block start and add a new rules.
 */
 
static struct egg_inmap_rules *egg_inmap_new_rules(struct egg_inmap *inmap,const char *src,int srcc) {
  int srcp=0,vid,pid,namec,tokenc;
  const char *name,*token;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  token=src+srcp;
  tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
  if ((sr_int_eval(&vid,token,tokenc)<2)||(vid<0)||(vid>0xffff)) return 0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  token=src+srcp;
  tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
  if ((sr_int_eval(&pid,token,tokenc)<2)||(vid<0)||(vid>0xffff)) return 0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  name=src+srcp;
  namec=srcc-srcp;
  while (namec&&((unsigned char)name[namec-1]<=0x20)) namec--;
  
  struct egg_inmap_rules *rules=egg_inmap_add_rules(inmap);
  if (!rules) return 0;
  rules->vid=vid;
  rules->pid=pid;
  if (namec) {
    if (!(rules->name=malloc(namec+1))) {
      egg_inmap_remove_rules(inmap,rules);
      return 0;
    }
    memcpy(rules->name,name,namec);
    rules->name[namec]=0;
    rules->namec=namec;
  }
  return rules;
}

/* Clear an existing rules or create a new one.
 */
 
struct egg_inmap_rules *egg_inmap_rewrite_rules(struct egg_inmap *inmap,int vid,int pid,int version,const char *name,int namec) {
  if (!name) namec=0; else if (namec<0) { namec=0; while (name[namec]) namec++; }
  struct egg_inmap_rules *rules=0;
  // Find exact matches only.
  int i=inmap->rulesc;
  while (i-->0) {
    struct egg_inmap_rules *q=inmap->rulesv[i];
    if ((q->vid==vid)&&(q->pid==pid)&&(q->namec==namec)&&!memcmp(q->name,name,namec)) {
      rules=q;
      break;
    }
  }
  // Clear the matched rules...
  if (rules) {
    rules->buttonc=0;
  // ...or create new blank rules.
  } else {
    if (!(rules=egg_inmap_add_rules(inmap))) return 0;
    rules->vid=vid;
    rules->pid=pid;
    if (!(rules->name=malloc(namec+1))) return 0;
    memcpy(rules->name,name,namec);
    rules->name[namec]=0;
    rules->namec=namec;
  }
  return rules;
}

/* Button list in rules.
 */
 
static int egg_inmap_rules_buttonv_search(const struct egg_inmap_rules *rules,int srcbtnid) {
  int lo=0,hi=rules->buttonc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    int q=rules->buttonv[ck].srcbtnid;
         if (srcbtnid<q) hi=ck;
    else if (srcbtnid>q) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

int egg_inmap_rules_add_button(struct egg_inmap_rules *rules,int srcbtnid,int dstbtnid) {
  if (!rules) return -1;
  int p=egg_inmap_rules_buttonv_search(rules,srcbtnid);
  if (p>=0) return -1;
  p=-p-1;
  if (rules->buttonc>=rules->buttona) {
    int na=rules->buttona+16;
    if (na>INT_MAX/sizeof(struct egg_inmap_button)) return -1;
    void *nv=realloc(rules->buttonv,sizeof(struct egg_inmap_button)*na);
    if (!nv) return -1;
    rules->buttonv=nv;
    rules->buttona=na;
  }
  struct egg_inmap_button *button=rules->buttonv+p;
  memmove(button+1,button,sizeof(struct egg_inmap_button)*(rules->buttonc-p));
  rules->buttonc++;
  button->srcbtnid=srcbtnid;
  button->dstbtnid=dstbtnid;
  return 0;
}

int egg_inmap_rules_get_button(const struct egg_inmap_rules *rules,int srcbtnid) {
  int p=egg_inmap_rules_buttonv_search(rules,srcbtnid);
  if (p<0) return 0;
  return rules->buttonv[p].dstbtnid;
}

/* Parse one line of config file and add to rules.
 */
 
static int egg_inmap_rules_add_line(struct egg_inmap_rules *rules,const char *src,int srcc) {
  int srcp=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  const char *srctoken=src+srcp;
  int srctokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) srctokenc++;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  const char *dsttoken=src+srcp;
  int dsttokenc=srcc-srcp;
  while (dsttokenc&&((unsigned char)dsttoken[dsttokenc-1]<=0x20)) dsttokenc--;
  
  // srctoken must be a positive integer.
  int srcbtnid;
  if (sr_int_eval(&srcbtnid,srctoken,srctokenc)<2) return -1;
  if (srcbtnid<1) return -1;
  
  // dsttoken is usually a Standard Mapping button name.
  // But we might share a config file. If we don't recognize it, no worries, don't even log a warning.
  int dstbtnid;
  if (egg_inmap_btnid_eval(&dstbtnid,dsttoken,dsttokenc)<0) {
    return 0;
  }
  
  return egg_inmap_rules_add_button(rules,srcbtnid,dstbtnid);
}

/* Load config from loose text.
 */
 
static int egg_inmap_load_text(struct egg_inmap *inmap,const char *src,int srcc,const char *refname) {
  struct egg_inmap_rules *rules=0;
  struct sr_decoder decoder={.v=src,.c=srcc};
  const char *line;
  int lineno=0,linec;
  while ((linec=sr_decode_line(&line,&decoder))>0) {
    lineno++;
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { linec--; line++; }
    if (!linec||(line[0]=='#')) continue;
    
    // ">>> VID PID NAME" starts a new block.
    if ((linec>=3)&&!memcmp(line,">>>",3)) {
      rules=egg_inmap_new_rules(inmap,line+3,linec-3);
      if (!rules) {
        fprintf(stderr,"%s:%d: Failed to decode block start.\n",refname,lineno);
      }
      continue;
    }
    
    // No rules, ignore the line.
    if (!rules) continue;
    
    if (egg_inmap_rules_add_line(rules,line,linec)<0) {
      fprintf(stderr,"%s:%d: Error adding input rule.\n",refname,lineno);
      return -1;
    }
  }
  return 0;
}

/* Try loading from a given path.
 * Resolve leading tildes.
 */
 
static int egg_inmap_load_path(struct egg_inmap *inmap,const char *prepath) {
  char path[1024];
  int pathc=path_resolve(path,sizeof(path),prepath,-1);
  if ((pathc<1)||(pathc>=sizeof(path))) return -1;
  char *src=0;
  int srcc=file_read(&src,path);
  if (srcc<0) return -1;
  if (inmap->cfgpath) free(inmap->cfgpath);
  if (inmap->cfgpath=malloc(pathc+1)) {
    memcpy(inmap->cfgpath,path,pathc);
    inmap->cfgpath[pathc]=0;
  }
  int err=egg_inmap_load_text(inmap,src,srcc,path);
  free(src);
  return 0; // Ignore errors. We found the file, and we're keeping it, whether it decoded or not.
}

/* Create default config file, after a load failure.
 */
 
static int egg_inmap_create_default_config(struct egg_inmap *inmap) {

  /* If we get the ROM externally, we're the generic runtime and should create "~/.egg/input.cfg".
   * Otherwise, we're branded as the game and we try not to say "egg". So use "./input.cfg".
   */
  const char *prepath=0;
  if (egg_romsrc==EGG_ROMSRC_EXTERNAL) {
    prepath="~/.egg/input.cfg";
  } else {
    prepath="./input.cfg";
  }
  char path[1024];
  int pathc=path_resolve(path,sizeof(path),prepath,-1);
  if ((pathc<1)||(pathc>=sizeof(path))) return 0;
  if (dir_mkdirp_parent(path)<0) return 0;
  
  /* Write the default content.
   */
  const char initfile[]=
    ">>> 0xffff 0xffff System Keyboard\n"
    "0x00070029 QUIT\n" // esc
    "0x00070042 LOAD\n" // f10
    "0x00070043 SAVE\n" // f9
    "0x00070044 FULLSCREEN\n" // f11
    "0x00070045 PAUSE\n" // f12
    "0x00070049 SCREENCAP\n" // insert
  "";
  if (file_write(path,initfile,sizeof(initfile)-1)<0) return 0;
  
  /* Now read it back and set up as usual.
   */
  if (egg_inmap_load_path(inmap,path)>=0) {
    fprintf(stderr,"%s: Created new input config.\n",inmap->cfgpath);
  }

  return 0;
}

/* Load.
 */

int egg_inmap_load(struct egg_inmap *inmap) {
  const char *pathv[]={
    "./input.cfg",
    "~/.egg/input.cfg",
    "~/.romassist/input.cfg", // A separate project of mine, and I want to share the config. Same format.
  0};
  const char **pathp=pathv;
  for (;*pathp;pathp++) {
    if (egg_inmap_load_path(inmap,*pathp)>=0) break;
  }
  if (inmap->cfgpath) {
    fprintf(stderr,"%s: Loaded input config.\n",inmap->cfgpath);
  } else {
    fprintf(stderr,"%s: No input config file found. Will attempt to create...\n",egg.exename);
    if (egg_inmap_create_default_config(inmap)<0) return -1;
  }
  return 0;
}

/* Encode.
 */
 
static int egg_inmap_encode(struct sr_encoder *dst,const struct egg_inmap *inmap) {
  int ri=0; for (;ri<inmap->rulesc;ri++) {
    struct egg_inmap_rules *rules=inmap->rulesv[ri];
    if (sr_encode_fmt(dst,">>> 0x%04x 0x%04x %.*s\n",rules->vid,rules->pid,rules->namec,rules->name)<0) return -1;
    const struct egg_inmap_button *button=rules->buttonv;
    int bi=rules->buttonc;
    for (;bi-->0;button++) {
      const char *name=egg_inmap_btnid_repr(button->dstbtnid);
      if (name) {
        if (sr_encode_fmt(dst,"0x%08x %s\n",button->srcbtnid,name)<0) return -1;
      } else {
        if (sr_encode_fmt(dst,"0x%08x 0x%08x\n",button->srcbtnid,button->dstbtnid)<0) return -1;
      }
    }
  }
  return 0;
}

/* Save.
 */
 
static int egg_inmap_save(struct egg_inmap *inmap) {
  if (!inmap->cfgpath) {
    const char *src="./input.cfg";
    int srcc=0; while (src[srcc]) srcc++;
    if (!(inmap->cfgpath=malloc(srcc+1))) return -1;
    memcpy(inmap->cfgpath,src,srcc);
    inmap->cfgpath[srcc]=0;
    fprintf(stderr,"%s: Input config path was unset. Saving to '%s'.\n",egg.exename,inmap->cfgpath);
  }
  struct sr_encoder serial={0};
  if (egg_inmap_encode(&serial,inmap)<0) {
    sr_encoder_cleanup(&serial);
    return -1;
  }
  int err=file_write(inmap->cfgpath,serial.v,serial.c);
  if (err<0) {
    // Try mkdir on the parent, but just that one level.
    // It's ok to create "~/.egg", but don't go creating "/home" on a Mac or Windows machine!
    // Also abort if the slash is in position 0 or 1: Don't try to mkdir(".").
    int slashp=-1,i=0;
    for (;inmap->cfgpath[i];i++) {
      if (inmap->cfgpath[i]=='/') slashp=i;
    }
    if (slashp<=1) { sr_encoder_cleanup(&serial); return -1; }
    inmap->cfgpath[slashp]=0;
    err=dir_mkdir(inmap->cfgpath);
    inmap->cfgpath[slashp]='/';
    if (err<0) { sr_encoder_cleanup(&serial); return -1; }
    if (file_write(inmap->cfgpath,serial.v,serial.c)<0) {
      sr_encoder_cleanup(&serial);
      return -1;
    }
  }
  sr_encoder_cleanup(&serial);
  fprintf(stderr,"%s: Rewrote input config.\n",inmap->cfgpath);
  return 0;
}

/* Find rules for device.
 */
 
struct egg_inmap_rules *egg_inmap_rules_for_device(
  const struct egg_inmap *inmap,int vid,int pid,int version,const char *name,int namec
) {
  if (namec<0) { namec=0; while (name[namec]) namec++; }
  int i=0; for (;i<inmap->rulesc;i++) {
    struct egg_inmap_rules *rules=inmap->rulesv[i];
    if (rules->vid&&(rules->vid!=vid)) continue;
    if (rules->pid&&(rules->pid!=pid)) continue;
    if (rules->version&&(rules->version!=version)) continue;
    if (rules->namec&&((rules->namec!=namec)||memcmp(rules->name,name,namec))) continue;
    return rules;
  }
  return 0;
}

/* Synthesize rules.
 */
 
static int egg_inmap_synthesize_rules_inner(struct egg_inmap_rules *rules,struct egg_device *device) {
  int orphan_buttonv[32];
  int orphan_axisv[32];
  int orphan_buttonc=0,orphan_axisc=0;
  int have_dpad_x=0,have_dpad_y=0,have_lx=0,have_ly=0;
  const struct egg_button *button=device->buttonv;
  int i=device->buttonc;
  for (;i-->0;button++) {
    int range=button->hi-button->lo+1;
    
    // Range under 2, there's nothing we can do with it.
    if (range<2) continue;
    
    // Range of 8 could only be a hat.
    if (range==8) {
      if (egg_inmap_rules_add_button(rules,button->btnid,EGG_INMAP_BTN_DPAD)<0) return -1;
      continue;
    }
    
    // Range of at least 3, and value in the middle, it's an axis.
    // 0x00010032,33 seem to be RX and 34,35 RY.
    // 30,31 could be LX/LY or the dpad
    if ((button->lo<button->value)&&(button->value<button->hi)) {
      if ((button->hidusage==0x00010032)||(button->hidusage==0x00010033)) {
        if (egg_inmap_rules_add_button(rules,button->btnid,EGG_JOYBTN_RX)<0) return -1;
        continue;
      }
      if ((button->hidusage==0x00010034)||(button->hidusage==0x00010035)) {
        if (egg_inmap_rules_add_button(rules,button->btnid,EGG_JOYBTN_RY)<0) return -1;
        continue;
      }
      if ((range==3)&&!have_dpad_x&&(button->hidusage==0x00010030)) {
        have_dpad_x=1;
        if (egg_inmap_rules_add_button(rules,button->btnid,EGG_INMAP_BTN_HORZ)<0) return -1;
        continue;
      }
      if ((range==3)&&!have_dpad_y&&(button->hidusage==0x00010031)) {
        have_dpad_y=1;
        if (egg_inmap_rules_add_button(rules,button->btnid,EGG_INMAP_BTN_VERT)<0) return -1;
        continue;
      }
      // No clear answer, so orphan it and we'll examine soon.
      if (orphan_axisc<32) orphan_axisv[orphan_axisc++]=button->btnid;
      continue;
    }
    
    // Low limit of zero, call it a button.
    // There's a few helpful HID usages here, which I have never actually observed in the wild. :(
    if (button->lo==0) {
      switch (button->hidusage) {
        case 0x0001003d: if (egg_inmap_rules_add_button(rules,button->btnid,EGG_JOYBTN_AUX1)<0) return -1; continue; // Start
        case 0x0001003e: if (egg_inmap_rules_add_button(rules,button->btnid,EGG_JOYBTN_AUX2)<0) return -1; continue; // Select
        case 0x00010090: if (egg_inmap_rules_add_button(rules,button->btnid,EGG_JOYBTN_UP)<0) return -1; continue; // Dpad...
        case 0x00010091: if (egg_inmap_rules_add_button(rules,button->btnid,EGG_JOYBTN_DOWN)<0) return -1; continue;
        case 0x00010092: if (egg_inmap_rules_add_button(rules,button->btnid,EGG_JOYBTN_RIGHT)<0) return -1; continue;
        case 0x00010093: if (egg_inmap_rules_add_button(rules,button->btnid,EGG_JOYBTN_LEFT)<0) return -1; continue;
      }
      if (orphan_buttonc<32) orphan_buttonv[orphan_buttonc++]=button->btnid;
      continue;
    }
  }
  // Assign the orphaned axes and buttons in an arbitrarily prioritized order.
  const int axis_order[]={EGG_JOYBTN_LX,EGG_JOYBTN_LY,EGG_JOYBTN_RX,EGG_JOYBTN_RY,EGG_INMAP_BTN_HORZ,EGG_INMAP_BTN_VERT};
  int orderc=sizeof(axis_order)/sizeof(int);
  for (i=0;i<orphan_axisc;i++) {
    if (egg_inmap_rules_add_button(rules,orphan_axisv[i],axis_order[i%orderc])<0) return -1;
  }
  const int button_order[]={
    EGG_JOYBTN_SOUTH,
    EGG_JOYBTN_WEST,
    EGG_JOYBTN_EAST,
    EGG_JOYBTN_NORTH,
    EGG_JOYBTN_AUX1,
    EGG_JOYBTN_L1,
    EGG_JOYBTN_R1,
    EGG_JOYBTN_AUX2,
    EGG_JOYBTN_LP,
    EGG_JOYBTN_RP,
    EGG_JOYBTN_AUX3,
  };
  orderc=sizeof(button_order)/sizeof(int);
  for (i=0;i<orphan_buttonc;i++) {
    if (egg_inmap_rules_add_button(rules,orphan_buttonv[i],button_order[i%orderc])<0) return -1;
  }
  return 0;
}

struct egg_inmap_rules *egg_inmap_synthesize_rules(struct egg_inmap *inmap,struct egg_device *device) {
  if (!inmap||!device) return 0;
  struct egg_inmap_rules *rules=egg_inmap_add_rules(inmap);
  if (!rules) return 0;
  rules->vid=device->vid;
  rules->pid=device->pid;
  if (device->namec) {
    if (!(rules->name=malloc(device->namec+1))) {
      egg_inmap_remove_rules(inmap,rules);
      return 0;
    }
    memcpy(rules->name,device->name,device->namec);
    rules->name[device->namec]=0;
    rules->namec=device->namec;
  }
  if (egg_inmap_synthesize_rules_inner(rules,device)<0) {
    egg_inmap_remove_rules(inmap,rules);
    return 0;
  }
  egg_inmap_save(inmap);
  return rules;
}

/* End of user's edit.
 */
 
int egg_inmap_ready(struct egg_inmap *inmap) {
  return egg_inmap_save(inmap);
}
