/* egg_config.h
 */
 
#ifndef EGG_CONFIG_H
#define EGG_CONFIG_H

// These are globals, (egg.config).
struct egg_config {
  const char *rompath;
  int lang;
};

int egg_configure(int argc,char **argv);

#endif
