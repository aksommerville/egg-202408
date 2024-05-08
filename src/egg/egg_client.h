/* egg_client.h
 * The functions here are for games to implement, not actually part of the Egg Platform API.
 * All hooks are required.
 */
 
#ifndef EGG_CLIENT_H
#define EGG_CLIENT_H

/* Last time you'll hear from Egg.
 * Usually there's no need for this.
 * But maybe you need to flush a cache out to storage, or print stats, or something.
 */
void egg_client_quit();

/* First thing Egg calls.
 * If you return <0, the runtime will abort and you won't be called again (not even egg_client_quit).
 */
int egg_client_init();

/* Called normally at video cadence, expect 60 Hz.
 * (elapsed) is the time in seconds since the last update, or zero on the very first.
 * There can be slippage against real time. We regulate reported timing here so as to keep in a reasonable range.
 * A typical update involves draining the event queue, then advance your model state.
 */
void egg_client_update(double elapsed);

/* Normally called once for every update, but the platform has some discretion there.
 * I recommend doing all rendering here, and nothing that changes your model.
 * Rendering is not guaranteed to work outside of this call.
 */
void egg_client_render();

#endif
