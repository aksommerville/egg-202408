#include "macos.h"
#include "macioc_internal.h"
#include <stdio.h>
#include <Cocoa/Cocoa.h>

struct macioc macioc={0};

/* Prerun argv. I pass a couple args from the build setup, to set working directory and TTY.
 */

int macos_prerun_argv(int argc,char **argv) {
  int i=1;
  while (i<argc) {
  
    if (!memcmp(argv[i],"--chdir=",8)) {
      chdir(argv[i]+8);
      argc--;
      memmove(argv+i,argv+i+1,sizeof(void*)*(argc-i));
      continue;
    }

    if (!memcmp(argv[i],"--reopen-tty=",13)) {
      int fd=open(argv[i]+13,O_RDWR);
      if (fd>=0) {
        dup2(fd,STDOUT_FILENO);
        dup2(fd,STDIN_FILENO);
        dup2(fd,STDERR_FILENO);
        close(fd);
      }
      argc--;
      memmove(argv+i,argv+i+1,sizeof(void*)*(argc-i));
      continue;
    }

    i++;
  }
  return argc;
}

/* Main.
 */

int macos_main(
  int argc,char **argv,
  void (*cb_quit)(),
  int (*cb_init)(int argc,char **argv),
  int (*cb_update)()
) {
  fprintf(stderr,"%s...\n",__func__);
  macioc.delegate.rate=60.0;
  macioc.delegate.quit=cb_quit;
  macioc.delegate.init=cb_init;
  macioc.delegate.update=cb_update;
  macioc.argv=argv;
  macioc.argc=argc;
  return NSApplicationMain(argc,(void*)argv);
}

/* Terminate.
 */

void macioc_terminate(int status) {
  fprintf(stderr,"%s:%d\n",__FILE__,__LINE__);
  macioc.terminate=1;
  fprintf(stderr,"%s:%d\n",__FILE__,__LINE__);
  [NSApplication.sharedApplication terminate:NSApplication.sharedApplication];
  fprintf(stderr,"%s:%d\n",__FILE__,__LINE__);
}
