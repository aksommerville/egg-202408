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
  return 0;
}
