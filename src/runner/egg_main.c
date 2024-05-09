#include "egg_runner_internal.h"

struct egg egg={0};

/* Main.
 */
 
int main(int argc,char **argv) {
  int err;
  if ((err=egg_configure(argc,argv))<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error during configuration.\n",egg.exename);
    return 1;
  }
  if (egg.terminate) return 0; // eg --help
  if ((err=egg_romsrc_load())<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error acquiring ROM file.\n",egg.exename);
    return 1;
  }
  //TODO...
  if (egg_romsrc_call_client_init()<0) {
    fprintf(stderr,"%s: Error in egg_client_init()\n",egg.exename);
    return 1;
  }
  egg_romsrc_call_client_update(123.456);
  egg_romsrc_call_client_render();
  egg_romsrc_call_client_quit();
  return 0;
}
