/* macos.h
 * Glue for NSApplicationMain().
 */

#ifndef MACOS_H
#define MACOS_H

int macos_prerun_argv(int argc,char **argv);

int macos_main(
  int argc,char **argv,
  void (*cb_quit)(),
  int (*cb_init)(int argc,char **argv),
  int (*cb_update)()
);

void macioc_terminate(int status);

#endif
