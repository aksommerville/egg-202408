/* incfg.h
 * Manages the special "--configure-input" mode where we prompt user to press joystick buttons.
 * Privy to egg runner's globals.
 * We operate as kind of an alternative to the client game, see egg_main.c, it's a 1:1 replacement.
 */
 
#ifndef INCFG_H
#define INCFG_H

struct incfg;
struct egg_rom_startup_props;

void incfg_del(struct incfg *incfg);

struct incfg *incfg_new();

int incfg_startup_props(struct egg_rom_startup_props *props,struct incfg *incfg);
int incfg_start(struct incfg *incfg);
int incfg_update(struct incfg *incfg,double elapsed);
void incfg_render(struct incfg *incfg);
int incfg_is_finished(const struct incfg *incfg);

#endif
