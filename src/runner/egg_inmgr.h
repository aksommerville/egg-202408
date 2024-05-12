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
struct egg_inmap;

void egg_inmgr_del(struct egg_inmgr *inmgr);
struct egg_inmgr *egg_inmgr_new();

int egg_inmgr_update(struct egg_inmgr *inmgr);

/* inmgr doesn't do any heavy faking like a virtual keyboard -- that's up to the client.
 * But we do produce our own cursor when there's a generic mouse but no window manager.
 */
void egg_inmgr_render(struct egg_inmgr *inmgr);

/* incfg needs this. No one else should depend on it, our button list gets mangled for mapped devices.
 */
void egg_inmgr_get_button_range(int *lo,int *hi,int *rest,const struct egg_inmgr *inmgr,int devid,int btnid);

const char *egg_inmgr_get_device_ids(int *vid,int *pid,int *version,const struct egg_inmgr *inmgr,int devid);
struct egg_inmap *egg_inmgr_get_inmap(struct egg_inmgr *inmgr);

#endif
