/* egg_config.h
 */
 
#ifndef EGG_CONFIG_H
#define EGG_CONFIG_H

// These are globals, (egg.config).
struct egg_config {
  char *rompath;
  int lang;
  int windoww,windowh;
  int fullscreen;
  char *video_device;
  char *video_driver;
  char *input_path;
  char *input_drivers;
  int audio_rate;
  int audio_chanc;
  int audio_buffer;
  char *audio_device;
  char *audio_driver;
  char *storepath;
  int ignore_required;
};

int egg_configure(int argc,char **argv);

#endif
