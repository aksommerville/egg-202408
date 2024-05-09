/* egg_input.h
 * Input bus and event queue.
 */
 
#ifndef EGG_INPUT_H
#define EGG_INPUT_H

/* Event type.
 */
#define EGG_EVENT_JOY 1
#define EGG_EVENT_KEY 2
#define EGG_EVENT_TEXT 3
#define EGG_EVENT_MMOTION 4
#define EGG_EVENT_MBUTTON 5
#define EGG_EVENT_MWHEEL 6
#define EGG_EVENT_TOUCH 7
#define EGG_EVENT_ACCEL 8

/* Joystick button, for standard mapping.
 * We may report other buttons outside this range, if we didn't know how to map them.
 */
#define EGG_JOYBTN_LX    0x40
#define EGG_JOYBTN_LY    0x41
#define EGG_JOYBTN_RX    0x42
#define EGG_JOYBTN_RY    0x43
#define EGG_JOYBTN_SOUTH 0x80
#define EGG_JOYBTN_EAST  0x81
#define EGG_JOYBTN_WEST  0x82
#define EGG_JOYBTN_NORTH 0x83
#define EGG_JOYBTN_L1    0x84
#define EGG_JOYBTN_R1    0x85
#define EGG_JOYBTN_L2    0x86
#define EGG_JOYBTN_R2    0x87
#define EGG_JOYBTN_AUX2  0x88
#define EGG_JOYBTN_AUX1  0x89
#define EGG_JOYBTN_LP    0x8a
#define EGG_JOYBTN_RP    0x8b
#define EGG_JOYBTN_UP    0x8c
#define EGG_JOYBTN_DOWN  0x8d
#define EGG_JOYBTN_LEFT  0x8e
#define EGG_JOYBTN_RIGHT 0x8f
#define EGG_JOYBTN_AUX3  0x90

/* Mouse button.
 * btnid >= 4 are legal, but no definite meaning assigned.
 */
#define EGG_MBUTTON_LEFT 1
#define EGG_MBUTTON_RIGHT 2
#define EGG_MBUTTON_MIDDLE 3

/* Touch state.
 */
#define EGG_TOUCH_END 0
#define EGG_TOUCH_BEGIN 1
#define EGG_TOUCH_MOVE 2

struct egg_event_joy {
  int type;
  int devid;
  int btnid; // 0 is special, it means device connection state.
  int value;
};

struct egg_event_key {
  int type;
  int keycode;
  int value;
};

struct egg_event_text {
  int type;
  int codepoint;
};

struct egg_event_mmotion {
  int type;
  int x;
  int y;
};

struct egg_event_mbutton {
  int type;
  int x;
  int y;
  int btnid;
  int value;
};

struct egg_event_mwheel {
  int type;
  int x;
  int y;
  int dx;
  int dy;
};

struct egg_event_touch {
  int type;
  int touchid;
  int state;
  int x;
  int y;
};

struct egg_event_accel {
  int type;
  int x;
  int y;
  int z;
};

union egg_event {
  int type;
  struct egg_event_joy joy;
  struct egg_event_key key;
  struct egg_event_text text;
  struct egg_event_mmotion mmotion;
  struct egg_event_mbutton mbutton;
  struct egg_event_mwheel mwheel;
  struct egg_event_touch touch;
  struct egg_event_accel accel;
};

/* Write up to (a) events at (v) and return the count written.
 * If we return exactly (a), you should process them and then call again.
 */
int egg_event_get(union egg_event *v,int a);

/* Request to enable or disable one event type.
 * Defaults:
 *   JOY enabled
 *   KEY enabled
 *   TEXT *** disabled
 *   MMOTION *** disabled
 *   MBUTTON *** disabled
 *   MWHEEL *** disabled
 *   TOUCH enabled
 *   ACCEL *** disabled
 * Returns the new state for this event, which is not always what you asked for.
 * If the platform knows that an event type is not possible, it should refuse to enable.
 */
int egg_event_enable(int event_type,int enable);

/* Enabled by default.
 * If true, and the platform supplies a default mouse cursor, show it when enabled.
 * The cursor is always hidden when (MMOTION,MBUTTON,MWHEEL) are all disabled.
 * You might want it hidden when enabled, if you're supplying a fake cursor.
 * We do not allow setting custom system cursors, we instead encourage using fake ones.
 */
void egg_show_cursor(int show);

/* If the platform allows it, prevent normal mouse processing and return only unadjusted deltas.
 * While locked, you will receive MMOTION events with signed (x,y).
 * TODO Can we provide a predictable range for those?
 */
int egg_lock_cursor(int lock);

/* Examine connected joysticks.
 * (NB These can be any kind of input device, "joystick" is just a word we use).
 * TODO Confirm we can trigger callbacks in both web and wamr.
 */
int egg_joystick_devid_by_index(int p);
void egg_joystick_get_ids(int *vid,int *pid,int *version,int devid);
int egg_joystick_get_name(char *dst,int dsta,int devid);
int egg_joystick_for_each_button(int devid,int (*cb)(int btnid,int usage,int lo,int hi,int value,void *userdata),void *userdata);

#endif
