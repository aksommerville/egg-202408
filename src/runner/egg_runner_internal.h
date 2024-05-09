#ifndef EGG_RUNNER_INTERNAL_H
#define EGG_RUNNER_INTERNAL_H

#include "opt/rom/rom.h"
#include "opt/wamr/wamr.h"
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
  struct rom rom;
  struct wamr *wamr;
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

#endif
