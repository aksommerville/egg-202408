/* hid_keycode.h
 * Selected symbols from USB HID Usage Tables 1.12, page 7.
 */

#ifndef HID_KEYCODE_H
#define HID_KEYCODE_H

/* Note that keys are named based on the US English layout, as in the spec.
 * But it's only a name, because we have to call them something.
 * eg KEY_Q is the top-left letter key, regardless of what letter is printed on it or how it's mapped.
 * 0x00..0x03 are special codes that are unlikely to reach Egg.
 */
#define KEY_A          0x00070004
#define KEY_B          0x00070005
#define KEY_C          0x00070006
#define KEY_D          0x00070007
#define KEY_E          0x00070008
#define KEY_F          0x00070009
#define KEY_G          0x0007000a
#define KEY_H          0x0007000b
#define KEY_I          0x0007000c
#define KEY_J          0x0007000d
#define KEY_K          0x0007000e
#define KEY_L          0x0007000f
#define KEY_M          0x00070010
#define KEY_N          0x00070011
#define KEY_O          0x00070012
#define KEY_P          0x00070013
#define KEY_Q          0x00070014
#define KEY_R          0x00070015
#define KEY_S          0x00070016
#define KEY_T          0x00070017
#define KEY_U          0x00070018
#define KEY_V          0x00070019
#define KEY_W          0x0007001a
#define KEY_X          0x0007001b
#define KEY_Y          0x0007001c
#define KEY_Z          0x0007001d
#define KEY_1          0x0007001e
#define KEY_2          0x0007001f
#define KEY_3          0x00070020
#define KEY_4          0x00070021
#define KEY_5          0x00070022
#define KEY_6          0x00070023
#define KEY_7          0x00070024
#define KEY_8          0x00070025
#define KEY_9          0x00070026
#define KEY_0          0x00070027
#define KEY_ENTER      0x00070028
#define KEY_ESCAPE     0x00070029
#define KEY_BACKSPACE  0x0007002a
#define KEY_TAB        0x0007002b
#define KEY_SPACE      0x0007002c
#define KEY_DASH       0x0007002d
#define KEY_EQUAL      0x0007002e
#define KEY_OPEN       0x0007002f
#define KEY_CLOSE      0x00070030
#define KEY_BACKSLASH  0x00070031
#define KEY_HASH       0x00070032 /* "Keyboard Non-US # and ~" */
#define KEY_SEMICOLON  0x00070033
#define KEY_APOSTROPHE 0x00070034
#define KEY_GRAVE      0x00070035
#define KEY_COMMA      0x00070036
#define KEY_DOT        0x00070037
#define KEY_SLASH      0x00070038
#define KEY_CAPSLOCK   0x00070039
#define KEY_F1         0x0007003a
#define KEY_F2         0x0007003b
#define KEY_F3         0x0007003c
#define KEY_F4         0x0007003d
#define KEY_F5         0x0007003e
#define KEY_F6         0x0007003f
#define KEY_F7         0x00070040
#define KEY_F8         0x00070041
#define KEY_F9         0x00070042
#define KEY_F10        0x00070043
#define KEY_F11        0x00070044
#define KEY_F12        0x00070045
#define KEY_PRINT      0x00070046
#define KEY_SCROLLLOCK 0x00070047
#define KEY_PAUSE      0x00070048
#define KEY_INSERT     0x00070049
#define KEY_HOME       0x0007004a
#define KEY_PAGEUP     0x0007004b
#define KEY_DELETE     0x0007004c
#define KEY_END        0x0007004d
#define KEY_PAGEDOWN   0x0007004e
#define KEY_RIGHT      0x0007004f
#define KEY_LEFT       0x00070050
#define KEY_DOWN       0x00070051
#define KEY_UP         0x00070052
#define KEY_NUMLOCK    0x00070053
#define KEY_KPSLASH    0x00070054
#define KEY_KPSTAR     0x00070055
#define KEY_KPDASH     0x00070056
#define KEY_KPPLUS     0x00070057
#define KEY_KPENTER    0x00070058
#define KEY_KP1        0x00070059
#define KEY_KP2        0x0007005a
#define KEY_KP3        0x0007005b
#define KEY_KP4        0x0007005c
#define KEY_KP5        0x0007005d
#define KEY_KP6        0x0007005e
#define KEY_KP7        0x0007005f
#define KEY_KP8        0x00070060
#define KEY_KP9        0x00070061
#define KEY_KP0        0x00070062
#define KEY_KPDOT      0x00070063
// Many uncommon keys from 0x64..0xdd (and 0xa5..0xaf are reserved, dunno why).
#define KEY_LCONTROL   0x000700e0
#define KEY_LSHIFT     0x000700e1
#define KEY_LALT       0x000700e2
#define KEY_LGUI       0x000700e3 /* aka "Super" or "Windows". HID calls it "GUI". */
#define KEY_RCONTROL   0x000700e4
#define KEY_RSHIFT     0x000700e5
#define KEY_RALT       0x000700e6
#define KEY_RGUI       0x000700e7

#endif
