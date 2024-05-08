/* egg_romsrc_native.c
 * Only one of the egg_romsrc_*.c files actually get used in a given target.
 * This one uses a bundled ROM file for assets, and native functions for code.
 */

#include "egg_runner_internal.h"

const int egg_romsrc=EGG_ROMSRC_NATIVE;

extern const char egg_rom_bundled[];
extern const int egg_rom_bundled_length;

/* Load.
 */
 
int egg_romsrc_load() {
  if (rom_init_borrow(&egg.rom,egg_rom_bundled,egg_rom_bundled_length)<0) {
    fprintf(stderr,"%s: Failed to decode built-in ROM.\n",egg.exename);
    return -2;
  }
  return 0;
}
