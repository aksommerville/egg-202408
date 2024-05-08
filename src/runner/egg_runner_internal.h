#ifndef EGG_RUNNER_INTERNAL_H
#define EGG_RUNNER_INTERNAL_H

#include "opt/rom/rom.h"
#include "egg_config.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

extern struct egg {
  const char *exename;
  struct egg_config config;
  int terminate;
  struct rom rom;
} egg;

extern const int egg_romsrc;
#define EGG_ROMSRC_EXTERNAL 1 /* Expect a ROM file at the command line, we are the general runner. */
#define EGG_ROMSRC_BUNDLED  2 /* The ROM file is linked against us at build time, we're a fake-native executable. */
#define EGG_ROMSRC_NATIVE   3 /* ROM is bundled but has no wasm resource, and the client entry points are linked natively. True-native. */

int egg_romsrc_load();

#endif
