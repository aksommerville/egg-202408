#include "egg_runner_internal.h"
#include <signal.h>
#include <unistd.h>

struct egg egg={0};

/* Quit.
 */
 
static void egg_quit() {
  egg_romsrc_call_client_quit();
  //TODO Log performance etc
  render_del(egg.render);
  //TODO Clean up I/O drivers
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
  
  //TODO Stand I/O drivers.
  
  if (!(egg.render=render_new())) {
    fprintf(stderr,"%s: Failed to initialize GLES2 context.\n",egg.exename);
    return -2;
  }
  
  // Finally, let the client start up.
  if (egg_romsrc_call_client_init()<0) {
    fprintf(stderr,"%s: Error in egg_client_init()\n",egg.exename);
    return -2;
  }
  
  return 0;
}

/* Update.
 * Block as needed.
 */
 
static int egg_update() {
  //TODO
  usleep(100000);
  egg_romsrc_call_client_update(123.456);
  egg_romsrc_call_client_render();
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
