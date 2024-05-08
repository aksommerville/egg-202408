#include "eggdev_internal.h"

/* Compile song.
 */
 
int eggdev_song_compile(struct romw *romw,struct romw_res *res) {
  // I think it's better to move image and song processing out of the archive packing stage.
  // Let `eggdev pack` receive files already digested.
  //fprintf(stderr,"TODO %s %s\n",__func__,res->path);
  return 0;
}
