/* egg_romsrc_native.c
 * Only one of the egg_romsrc_*.c files actually get used in a given target.
 * This one uses a bundled ROM file for assets, and native functions for code.
 */

#include "egg_runner_internal.h"
#include <stdarg.h>

const int egg_romsrc=EGG_ROMSRC_NATIVE;

extern const char egg_rom_bundled[];
extern const int egg_rom_bundled_length;

/* egg_log: Not part of the general API because it's variadic, so egg_romsrc_external does its own thing,
 * but we can do a much easier (and better) thing.
 */
 
void egg_log(const char *fmt,...) {
  va_list vargs;
  va_start(vargs,fmt);
  char tmp[1024];
  int tmpc=vsnprintf(tmp,sizeof(tmp),fmt,vargs);
  if ((tmpc<0)||(tmpc>sizeof(tmp))) tmpc=0;
  fprintf(stderr,"%.*s\n",tmpc,tmp);
}

/* Load.
 */
 
int egg_romsrc_load() {
  if (rom_init_borrow(&egg.rom,egg_rom_bundled,egg_rom_bundled_length)<0) {
    fprintf(stderr,"%s: Failed to decode built-in ROM.\n",egg.exename);
    return -2;
  }
  return 0;
}

/* Client hooks.
 */
 
void egg_romsrc_call_client_quit() {
  egg_client_quit();
}

int egg_romsrc_call_client_init() {
  return egg_client_init();
}

void egg_romsrc_call_client_update(double elapsed) {
  egg_client_update(elapsed);
}

void egg_romsrc_call_client_render() {
  egg_client_render();
}
