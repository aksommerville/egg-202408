#include "egg_runner_internal.h"
#include <signal.h>
#include <unistd.h>

struct egg egg={0};

/* Quit.
 */
 
static void egg_quit() {
  if (!egg.config.configure_input) {
    if (egg.client_initted) egg_romsrc_call_client_quit();
  }
  egg_store_quit();
  egg_timer_report(&egg.timer);
  render_del(egg.render);
  hostio_del(egg.hostio);
  synth_del(egg.synth);
  egg_inmgr_del(egg.inmgr);
  incfg_del(egg.incfg);
}

/* Init: Video driver and renderer.
 */
 
static int egg_init_video() {

  // Title, icon, and framebuffer size come from the ROM.
  int err;
  struct egg_rom_startup_props props={0};
  if (egg.config.configure_input) {
    err=incfg_startup_props(&props,egg.incfg);
  } else {
    err=egg_rom_startup_props(&props);
  }
  if (err<0) return err;
  egg.directgl=props.directgl;

  // Find a driver and start it up.
  struct hostio_video_setup setup={
    .title=props.title,
    .iconrgba=props.iconrgba,
    .iconw=props.iconw,
    .iconh=props.iconh,
    .w=egg.config.windoww,
    .h=egg.config.windowh,
    .fullscreen=egg.config.fullscreen,
    .fbw=props.fbw,
    .fbh=props.fbh,
    .device=egg.config.video_device,
  };
  err=hostio_init_video(egg.hostio,egg.config.video_driver,&setup);
  egg_rom_startup_props_cleanup(&props);
  if (err<0) {
    fprintf(stderr,"%s: Failed to initialize video driver.\n",egg.exename);
    return -2;
  }
  if ((props.fbw<1)||(props.fbh<1)) {
    // This would fail sensibly if we just let it go, but we can spare some trouble and also provide a more meaningful message.
    const char *refname=(egg_romsrc==EGG_ROMSRC_EXTERNAL)?egg.config.rompath:egg.exename;
    fprintf(stderr,"%s: ROM does not declare a valid framebuffer size.\n",refname);
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
  if (egg.directgl&&!egg.config.configure_input) {
    // Don't create the framebuffer in a direct-render situation.
  } else {
    if (render_texture_new(egg.render)!=1) {
      fprintf(stderr,"%s: Failed to allocate main framebuffer.\n",egg.exename);
      return -2;
    }
    if (render_texture_load(egg.render,1,props.fbw,props.fbh,0,EGG_TEX_FMT_RGBA,0,0)<0) {
      fprintf(stderr,"%s: Failed to initialize %dx%d framebuffer.\n",egg.exename,props.fbw,props.fbh);
      return -2;
    }
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
  if (!(egg.inmgr=egg_inmgr_new())) {
    fprintf(stderr,"%s: Failed to initialize input manager.\n",egg.exename);
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
  // Or with --configure-input, stand that controller.
  if (egg.config.configure_input) {
    if (!(egg.incfg=incfg_new())) {
      fprintf(stderr,"%s: Failed to initialize input configurator.\n",egg.exename);
      return -2;
    }
  } else {
    if ((err=egg_romsrc_load())<0) {
      if (err!=-2) fprintf(stderr,"%s: Unspecified error acquiring ROM file.\n",egg.exename);
      return -2;
    }
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
  if (egg.config.configure_input) {
    if ((err=incfg_start(egg.incfg))<0) {
      if (err!=-2) fprintf(stderr,"%s: Unspecified error starting input configurator.\n",egg.exename);
      return -2;
    }
  } else {
    if (egg_romsrc_call_client_init()<0) {
      fprintf(stderr,"%s: Error in egg_client_init()\n",egg.exename);
      return -2;
    }
  }
  egg.client_initted=1;
  
  // And now time begins.
  egg_timer_init(&egg.timer,60.0,0.250);
  
  return 0;
}

/* Sticky audio lock.
 */
 
int egg_lock_audio() {
  if (egg.audio_locked) return 0;
  if (hostio_audio_lock(egg.hostio)<0) return -1;
  egg.audio_locked=1;
  return 0;
}

void egg_unlock_audio() {
  hostio_audio_unlock(egg.hostio);
  egg.audio_locked=0;
}

/* Update.
 * Block as needed.
 */
 
static int egg_update() {
  int err;
  
  // Pump the upstream event queue.
  if (hostio_update(egg.hostio)<0) {
    fprintf(stderr,"%s: Error updating I/O drivers.\n",egg.exename);
    return -2;
  }
  if ((err=egg_inmgr_update(egg.inmgr))<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error updating input manager.\n",egg.exename);
    return -2;
  }
  
  // Sleep if needed, then update the client.
  double elapsed=egg_timer_tick(&egg.timer);
  if (egg.config.configure_input) {
    if ((err=incfg_update(egg.incfg,elapsed))<0) {
      if (err!=-2) fprintf(stderr,"%s: Unspecified error updating input configurator.\n",egg.exename);
      return -2;
    }
    if (incfg_is_finished(egg.incfg)) {
      //TODO If we make incfg accessible to client games, don't terminate here -- return to the game.
      egg.terminate=1;
      return 0;
    }
  } else {
    egg_romsrc_call_client_update(elapsed);
  }
  if (egg.audio_locked) egg_unlock_audio();
  
  // Render.
  if (egg.hostio->video->type->gx_begin(egg.hostio->video)<0) {
    fprintf(stderr,"%s: Video driver failed to begin frame.\n",egg.exename);
    return -2;
  }
  if (egg.directgl&&!egg.config.configure_input) {
    egg_romsrc_call_client_render();
  } else {
    render_tint(egg.render,0);
    render_alpha(egg.render,0xff);
    if (egg.config.configure_input) {
      incfg_render(egg.incfg);
    } else {
      egg_romsrc_call_client_render();
    }
    egg_inmgr_render(egg.inmgr);
    render_draw_to_main(egg.render,egg.hostio->video->w,egg.hostio->video->h,1);
  }
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
