#include "egg_runner_internal.h"

/* Public API.
 */
 
int egg_event_get(union egg_event *v,int a) {
  return 0;
}

int egg_event_enable(int type,int enable) {
  return 0;
}

void egg_show_cursor(int show) {
}

int egg_lock_cursor(int lock) {
  return 0;
}

int egg_joystick_devid_by_index(int p) {
  return 0;
}

void egg_joystick_get_ids(int *vid,int *pid,int *version,int devid) {
}

int egg_joystick_get_name(char *dst,int dsta,int devid) {
  return 0;
}

/* Window manager callbacks.
 */
 
void egg_cb_close(struct hostio_video *driver) {
  fprintf(stderr,"%s\n",__func__);
  egg.terminate=1;
}

void egg_cb_focus(struct hostio_video *driver,int focus) {
  fprintf(stderr,"%s %d\n",__func__,focus);
  //TODO auto pause the game on blur?
}

void egg_cb_resize(struct hostio_video *driver,int w,int h) {
  fprintf(stderr,"%s %d,%d\n",__func__,w,h);
}

/* Keyboard.
 */
 
int egg_cb_key(struct hostio_video *driver,int keycode,int value) {
  fprintf(stderr,"%s %08x=%d\n",__func__,keycode,value);
  return 0;
}

void egg_cb_text(struct hostio_video *driver,int codepoint) {
  fprintf(stderr,"%s U+%x\n",__func__,codepoint);
}

/* Mouse.
 */
 
void egg_cb_mmotion(struct hostio_video *driver,int x,int y) {
  fprintf(stderr,"%s %d,%d\n",__func__,x,y);
}

void egg_cb_mbutton(struct hostio_video *driver,int btnid,int value) {
  fprintf(stderr,"%s %d=%d\n",__func__,btnid,value);
}

void egg_cb_mwheel(struct hostio_video *driver,int dx,int dy) {
  fprintf(stderr,"%s %+d,%+d\n",__func__,dx,dy);
}

/* Ready for PCM.
 */
 
void egg_cb_pcm_out(int16_t *v,int c,struct hostio_audio *driver) {
  synth_updatei(v,c,egg.synth);
}

/* Joysticks.
 */
 
void egg_cb_connect(struct hostio_input *driver,int devid) {
  fprintf(stderr,"%s %d\n",__func__,devid);
}

void egg_cb_disconnect(struct hostio_input *driver,int devid) {
  fprintf(stderr,"%s %d\n",__func__,devid);
}

void egg_cb_button(struct hostio_input *driver,int devid,int btnid,int value) {
  fprintf(stderr,"%s %d.0x%08x=%d\n",__func__,devid,btnid,value);
}
