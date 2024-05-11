#include "egg_runner_internal.h"

/* Window manager callbacks.
 */
 
void egg_cb_close(struct hostio_video *driver) {
  egg.terminate=1;
}

void egg_cb_focus(struct hostio_video *driver,int focus) {
  //TODO auto pause the game on blur?
}

void egg_cb_resize(struct hostio_video *driver,int w,int h) {
  // I think we don't need anything for this. Render polls the video driver's size at each frame.
}

/* Ready for PCM.
 */
 
void egg_cb_pcm_out(int16_t *v,int c,struct hostio_audio *driver) {
  synth_updatei(v,c,egg.synth);
}
