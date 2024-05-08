/* egg_romsrc_external.c
 * Only one of the egg_romsrc_*.c files actually get used in a given target.
 * This one is for the universal runner; ROM file must be provided at runtime.
 */

#include "egg_runner_internal.h"
#include "opt/fs/fs.h"

const int egg_romsrc=EGG_ROMSRC_EXTERNAL;

/* Load.
 */
 
int egg_romsrc_load() {
  if (!egg.config.rompath) {
    fprintf(stderr,"%s: Please specify a ROM file.\n",egg.exename);
    return -2;
  }
  void *serial=0;
  int serialc=file_read(&serial,egg.config.rompath);
  if (serialc<0) {
    fprintf(stderr,"%s: Failed to read file.\n",egg.config.rompath);
    return -2;
  }
  if (rom_init_handoff(&egg.rom,serial,serialc)<0) {
    fprintf(stderr,"%s: Invalid ROM file.\n",egg.config.rompath);
    free(serial);
    return -2;
  }
  //TODO Initialize wamr.
  return 0;
}
