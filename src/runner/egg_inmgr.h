/* egg_inmgr.h
 * General input manager for the native runner.
 * We are part of the 'runner' unit and we interact with its globals.
 * We implement the public input API:
 *   - egg_event_get
 *   - egg_event_enable
 *   - egg_show_cursor
 *   - egg_lock_cursor
 *   - egg_joystick_devid_by_index
 *   - egg_joystick_get_ids
 *   - egg_joystick_get_name
 *   - egg_joystick_for_each_button
 */
 
#ifndef EGG_INMGR_H
#define EGG_INMGR_H

struct egg_inmgr;

void egg_inmgr_del(struct egg_inmgr *inmgr);
struct egg_inmgr *egg_inmgr_new();

int egg_inmgr_update(struct egg_inmgr *inmgr);

/* inmgr doesn't do any heavy faking like a virtual keyboard -- that's up to the client.
 * But we do produce our own cursor when there's a generic mouse but no window manager.
 */
void egg_inmgr_render(struct egg_inmgr *inmgr);

#endif
