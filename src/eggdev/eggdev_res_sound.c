#include "eggdev_internal.h"

/* Slice sounds.
 */
 
int eggdev_sounds_slice(struct romw *romw,const char *src,int srcc,const struct eggdev_rpath *rpath) {
  return 0;//TODO
}

/* Compile one sound.
 */
 
int eggdev_sound_compile(struct romw *romw,struct romw_res *res) {
  fprintf(stderr,"TODO %s %s\n",__func__,res->path);
  return 0;
}
