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

// Our button list is not exactly the standard list: We put LP and RP at the end, so LX and RX can be tested first.
#define INCFG_IX_SOUTH 0
#define INCFG_IX_EAST 1
#define INCFG_IX_WEST 2
#define INCFG_IX_NORTH 3
#define INCFG_IX_L1 4
#define INCFG_IX_R1 5
#define INCFG_IX_L2 6
#define INCFG_IX_R2 7
#define INCFG_IX_AUX2 8
#define INCFG_IX_AUX1 9
#define INCFG_IX_UP 10
#define INCFG_IX_DOWN 11
#define INCFG_IX_LEFT 12
#define INCFG_IX_RIGHT 13
#define INCFG_IX_AUX3 14
#define INCFG_IX_LX 15
#define INCFG_IX_LY 16
#define INCFG_IX_RX 17
#define INCFG_IX_RY 18
#define INCFG_IX_LP 19
#define INCFG_IX_RP 20
#define INCFG_IX_COUNT 21

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
  } sourcev[INCFG_IX_COUNT];
  uint8_t ignoreix[21]; // Anything nonzero here, that btnix was obviated by some prior axis assignment.
};

#endif
