#ifndef EGG_RUNNER_INTERNAL_H
#define EGG_RUNNER_INTERNAL_H

#include "opt/rom/rom.h"
#include "opt/wamr/wamr.h"
#include "opt/png/png.h"
#include "opt/render/render.h"
#include "opt/hostio/hostio.h"
#include "opt/synth/synth.h"
#include "egg_config.h"
#include "egg/egg.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

extern struct egg {
  const char *exename;
  struct egg_config config;
  int terminate;
  int client_initted;
  struct rom rom;
  struct wamr *wamr;
  struct render *render;
  struct synth *synth;
  struct hostio *hostio;
  volatile int sigc;
  char *store;
  int storec;
  int store_dirty;
} egg;

extern const int egg_romsrc;
#define EGG_ROMSRC_EXTERNAL 1 /* Expect a ROM file at the command line, we are the general runner. */
#define EGG_ROMSRC_BUNDLED  2 /* The ROM file is linked against us at build time, we're a fake-native executable. */
#define EGG_ROMSRC_NATIVE   3 /* ROM is bundled but has no wasm resource, and the client entry points are linked natively. True-native. */

int egg_romsrc_load();

void egg_romsrc_call_client_quit();
int egg_romsrc_call_client_init();
void egg_romsrc_call_client_update(double elapsed);
void egg_romsrc_call_client_render();

int egg_store_init();
void egg_store_quit();
void egg_store_flush();

void egg_cb_close(struct hostio_video *driver);
void egg_cb_focus(struct hostio_video *driver,int focus);
void egg_cb_resize(struct hostio_video *driver,int w,int h);
int egg_cb_key(struct hostio_video *driver,int keycode,int value);
void egg_cb_text(struct hostio_video *driver,int codepoint);
void egg_cb_mmotion(struct hostio_video *driver,int x,int y);
void egg_cb_mbutton(struct hostio_video *driver,int btnid,int value);
void egg_cb_mwheel(struct hostio_video *driver,int dx,int dy);
void egg_cb_pcm_out(int16_t *v,int c,struct hostio_audio *driver);
void egg_cb_connect(struct hostio_input *driver,int devid);
void egg_cb_disconnect(struct hostio_input *driver,int devid);
void egg_cb_button(struct hostio_input *driver,int devid,int btnid,int value);

#endif
