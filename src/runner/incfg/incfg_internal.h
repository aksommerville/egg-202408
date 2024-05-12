#ifndef INCFG_INTERNAL_H
#define INCFG_INTERNAL_H

#include "runner/egg_runner_internal.h"
#include "runner/egg_inmap.h"

#define INCFG_FOCUS_BLINK_PERIOD 0.750
#define INCFG_GIVEUP_TIME 7.0
#define INCFG_GIVEUP_WARNING_TIME 4.0

#define INCFG_STATE_FAULT 1
#define INCFG_STATE_WAIT1 2
#define INCFG_STATE_HOLD1 3
#define INCFG_STATE_WAIT2 4
#define INCFG_STATE_HOLD2 5

extern const unsigned char incfg_font_tilesheet[];
extern const int incfg_font_tilesheet_size;

struct incfg {
  int screenw,screenh;
  int texid_font;
  double focus_blink_clock;
  int devid; // Zero if we haven't picked one yet.
  int btnix;
  int state;
  int btnid; // Button currently tracking, in (HOLD1,WAIT2,HOLD2)
  int part; // Qualifier against (btnid) for axes and hats.
  int finished;
  double giveup_clock;
  struct incfg_source {
    int btnid;
    int part;
  } sourcev[21];
  uint8_t ignoreix[21]; // Anything nonzero here, that btnix was obviated by some prior axis assignment.
};

#endif
