#include "egg_runner_internal.h"
#include <signal.h>
#include <unistd.h>

struct egg egg={0};

/* Quit.
 */
 
static void egg_quit() {
  if (egg.client_initted) egg_romsrc_call_client_quit();
  egg_store_quit();
  //TODO Log performance etc
  render_del(egg.render);
  hostio_del(egg.hostio);
}

/* Init: Video driver and renderer.
 */
 
static int egg_init_video() {

  // Title, icon, and framebuffer size come from the ROM.
  const char *title="TODO title";
  const void *iconrgba=0;
  int iconw=0,iconh=0;
  int fbw=0,fbh=0;

  // Find a driver and start it up.
  struct hostio_video_setup setup={
    .title=title,
    .iconrgba=iconrgba,
    .iconw=iconw,
    .iconh=iconh,
    .w=egg.config.windoww,
    .h=egg.config.windowh,
    .fullscreen=egg.config.fullscreen,
    .fbw=fbw,
    .fbh=fbh,
    .device=egg.config.video_device,
  };
  if (hostio_init_video(egg.hostio,egg.config.video_driver,&setup)<0) {
    fprintf(stderr,"%s: Failed to initialize video driver.\n",egg.exename);
    return -2;
  }
  
  //TODO Might as well remove hostio_video's "fb" hooks and make "gx" mandatory; it's all we're going to use.
  if (!egg.hostio->video->type->gx_begin||!egg.hostio->video->type->gx_end) {
    fprintf(stderr,"%s: Video driver '%s' does not implement GX fences.\n",egg.exename,egg.hostio->video->type->name);
    return -2;
  }
  
  // Make the renderer.
  if (!(egg.render=render_new())) {
    fprintf(stderr,"%s: Failed to initialize GLES2 context.\n",egg.exename);
    return -2;
  }
  
  return 0;
}

/* Init: Input drivers.
 */
 
static int egg_init_input() {
  struct hostio_input_setup setup={
    .path=egg.config.input_path,
  };
  if (hostio_init_input(egg.hostio,egg.config.input_drivers,&setup)<0) {
    fprintf(stderr,"%s: Error initializing input drivers.\n",egg.exename);
    return -2;
  }
  return 0;
}

/* Init: Audio driver and synthesizer.
 */
 
static int egg_init_audio() {
  
  struct hostio_audio_setup setup={
    .rate=egg.config.audio_rate,
    .chanc=egg.config.audio_chanc,
    .device=egg.config.audio_device,
    .buffer_size=egg.config.audio_buffer,
  };
  if (hostio_init_audio(egg.hostio,egg.config.audio_driver,&setup)<0) {
    fprintf(stderr,"%s: Failed to initialize audio driver.\n",egg.exename);
    return -2;
  }
  
  if (!(egg.synth=synth_new(egg.hostio->audio->rate,egg.hostio->audio->chanc,&egg.rom))) {
    fprintf(stderr,
      "%s: Failed to initialize synthesizer. rate=%d chanc=%d\n",
      egg.exename,egg.hostio->audio->rate,egg.hostio->audio->chanc
    );
    return -2;
  }
  
  hostio_audio_play(egg.hostio,1);
  
  return 0;
}

/* Init.
 */
 
static int egg_init(int argc,char **argv) {
  int err;
  
  // Load configuration first, everything else depends on it.
  if ((err=egg_configure(argc,argv))<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error during configuration.\n",egg.exename);
    return -2;
  }
  if (egg.terminate) return 0; // eg --help

  // Acquire ROM file and stand Wasm environment.
  if ((err=egg_romsrc_load())<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error acquiring ROM file.\n",egg.exename);
    return -2;
  }
  
  // Load the store.
  if ((err=egg_store_init())<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error loading store.\n",egg.exename);
    return -2;
  }
  
  // Create hostio context.
  struct hostio_video_delegate video_delegate={
    .cb_close=egg_cb_close,
    .cb_focus=egg_cb_focus,
    .cb_resize=egg_cb_resize,
    .cb_key=egg_cb_key,
    .cb_text=egg_cb_text,
    .cb_mmotion=egg_cb_mmotion,
    .cb_mbutton=egg_cb_mbutton,
    .cb_mwheel=egg_cb_mwheel,
  };
  struct hostio_audio_delegate audio_delegate={
    .cb_pcm_out=egg_cb_pcm_out,
  };
  struct hostio_input_delegate input_delegate={
    .cb_connect=egg_cb_connect,
    .cb_disconnect=egg_cb_disconnect,
    .cb_button=egg_cb_button,
  };
  if (!(egg.hostio=hostio_new(&video_delegate,&audio_delegate,&input_delegate))) {
    fprintf(stderr,"%s: Failed to initialize I/O wrapper.\n",egg.exename);
    return -2;
  }
  
  // Bring up video and renderer.
  if ((err=egg_init_video())<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error initializing video.\n",egg.exename);
    return -2;
  }
  
  // Bring up input drivers.
  if ((err=egg_init_input())<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error initializing input.\n",egg.exename);
    return -2;
  }
  
  // Bring up audio and synthesizer.
  if ((err=egg_init_audio())<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error initializing audio.\n",egg.exename);
    return -2;
  }
  
  // All subsystems are now online. Tell the log.
  hostio_log_driver_names(egg.hostio);
  
  // Run the client's init hook.
  if (egg_romsrc_call_client_init()<0) {
    fprintf(stderr,"%s: Error in egg_client_init()\n",egg.exename);
    return -2;
  }
  egg.client_initted=1;
  
  return 0;
}

/* Update.
 * Block as needed.
 */
 
static int egg_update() {
  int err;
  
  if (hostio_update(egg.hostio)<0) {
    fprintf(stderr,"%s: Error updating I/O drivers.\n",egg.exename);
    return -2;
  }
  
  usleep(100000);//TODO clock
  
  egg_romsrc_call_client_update(0.020);
  
  // Render.
  if (egg.hostio->video->type->gx_begin(egg.hostio->video)<0) {
    fprintf(stderr,"%s: Video driver failed to begin frame.\n",egg.exename);
    return -2;
  }
  render_tint(egg.render,0);
  render_alpha(egg.render,0xff);
  egg_romsrc_call_client_render();
  render_draw_to_main(egg.render,egg.hostio->video->w,egg.hostio->video->h,1);
  if (egg.hostio->video->type->gx_end(egg.hostio->video)<0) {
    fprintf(stderr,"%s: Error submitting video frame.\n",egg.exename);
    return -2;
  }
  
  return 0;
}

/* Signal handler.
 */
 
static void egg_rcvsig(int sigid) {
  switch (sigid) {
    case SIGINT: if (++(egg.sigc)>=3) {
        fprintf(stderr,"%s: Too many unprocessed signals.\n",egg.exename);
        exit(1);
      } break;
  }
}

/* Main.
 */
 
int main(int argc,char **argv) {
  int err;
  signal(SIGINT,egg_rcvsig);
  if ((err=egg_init(argc,argv))<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error bringing runtime online.\n",egg.exename);
    return 1;
  }
  while (!egg.terminate&&!egg.sigc) {
    if ((err=egg_update())<0) {
      if (err!=-2) fprintf(stderr,"%s: Unspecified error during run.\n",egg.exename);
      return 1;
    }
  }
  egg_quit();
  return 0;
}
