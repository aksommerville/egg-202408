/* egg_inmap.h
 * This belongs to inmgr. Split out only for the sake of neatness.
 * inmap is responsible for mapping devices from their driver format to Standard Mapping if possible.
 */
 
#ifndef EGG_INMAP_H
#define EGG_INMAP_H

/* egg_button and egg_device more properly belong in egg_inmgr.c.
 * We need them for synthesizing map.
 */
struct egg_button {
// Constant, will match the driver:
  int btnid;
  int hidusage; // dstbtnid after mapping
  int lo,hi; // thresholds for HORZ,VERT,DPAD after mapping; or nominal range for LX,LY,RX,RY
  int value;
// Available for mappers' use:
  int dstbtnid;
  int srclo,srchi;
  int dstvalue;
};
 
struct egg_device {
  int devid;
  int rptcls; // What are we calling it, to the public? An EGG_EVENT_* or zero to ignore. (MMOTION for mouse)
  struct hostio_input *driver;
  char *name;
  int namec;
  int vid,pid,version;
  struct egg_button *buttonv;
  int buttonc,buttona;
  int x,y,w,h; // For fake mouse only.
};

/* EGG_JOYBTN_* are legal for dstbtnid of course.
 * We also define these special aggregate symbols.
 */
#define EGG_INMAP_BTN_HORZ  0x20 /* dpad left+right */
#define EGG_INMAP_BTN_VERT  0x21 /* dpad up+down */
#define EGG_INMAP_BTN_DPAD  0x22 /* dpad as hat */
#define EGG_INMAP_BTN_NHORZ 0x23 /* dpad left+right reversed */
#define EGG_INMAP_BTN_NVERT 0x24
#define EGG_INMAP_BTN_NLX   0x25 /* analogue axes reversed */
#define EGG_INMAP_BTN_NLY   0x26
#define EGG_INMAP_BTN_NRX   0x27
#define EGG_INMAP_BTN_NRY   0x28

struct egg_inmap_button {
  int srcbtnid;
  int dstbtnid;
};

struct egg_inmap_rules {
  int vid,pid,version; // Zero matches all.
  char *name;
  int namec;
  struct egg_inmap_button *buttonv;
  int buttonc,buttona;
};

struct egg_inmap {
  struct egg_inmap_rules **rulesv; // Order matters. First match wins.
  int rulesc,rulesa;
  char *cfgpath;
};

void egg_inmap_cleanup(struct egg_inmap *inmap);

int egg_inmap_load(struct egg_inmap *inmap);

// Find an existing rules.
struct egg_inmap_rules *egg_inmap_rules_for_device(
  const struct egg_inmap *inmap,int vid,int pid,int version,const char *name,int namec
);

int egg_inmap_rules_get_button(const struct egg_inmap_rules *rules,int srcbtnid);

/* Generate and install a rules for this device, by guessing off its reported capabilities.
 * If we have a config file, we rewrite it before returning.
 */
struct egg_inmap_rules *egg_inmap_synthesize_rules(struct egg_inmap *inmap,struct egg_device *device);

/* Support for adding rules.
 */
struct egg_inmap_rules *egg_inmap_rewrite_rules(struct egg_inmap *inmap,int vid,int pid,int version,const char *name,int namec);
int egg_inmap_rules_add_button(struct egg_inmap_rules *rules,int srcbtnid,int dstbtnid);
int egg_inmap_ready(struct egg_inmap *inmap);

#endif
