#include "egg_runner_internal.h"
#include "egg_inmap.h"

/* Trivial actions.
 */
 
static void egg_ua_QUIT() {
  fprintf(stderr,"%s: Terminating due to keystroke.\n",egg.exename);
  egg.terminate=1;
}
 
static void egg_ua_FULLSCREEN() {
  hostio_toggle_fullscreen(egg.hostio);
}
 
static void egg_ua_PAUSE() {
  if (egg.hard_pause) {
    fprintf(stderr,"%s: Resume\n",egg.exename);
    egg.hard_pause=0;
  } else {
    fprintf(stderr,"%s: Hard pause\n",egg.exename);
    egg.hard_pause=1;
  }
}

/* Take and save a screencap.
 */
 
static void egg_ua_SCREENCAP() {
  fprintf(stderr,"%s:%s TODO\n",__FILE__,__func__);
}

/* Save state.
 */
 
static void egg_ua_SAVE() {
  fprintf(stderr,"%s:%s TODO\n",__FILE__,__func__);
}

/* Load state.
 */
 
static void egg_ua_LOAD() {
  fprintf(stderr,"%s:%s TODO\n",__FILE__,__func__);
}

/* User action, dispatch.
 */
 
int egg_perform_user_action(int inmap_btnid) {
  switch (inmap_btnid) {
    #define _(tag) case EGG_INMAP_BTN_##tag: egg_ua_##tag(); return 1;
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
