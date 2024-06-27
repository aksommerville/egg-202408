/* egg_audio.h
 */
 
#ifndef EGG_AUDIO_H
#define EGG_AUDIO_H

/* Manual PCM API.
 * If you brought your own synthesizer, call this during init, and fill the buffer whenever we call it.
 * Employ the lock from outside the callback to ensure your callback is not running.
 * Depending on the underlying platform, the callback might happen in a separate thread.
 * If you use this manual API, the friendly API below is not available.
 ******************************************************************************************/
 
//XXX 2024-06-26 Confirm that WAMR will be OK with this before implementing.

/* Provide your preferred rate and channel count, and we replace with the actual.
 * You must accept whatever we return.
 * Returns <0 only on hard errors.
 * Playback will not begin during egg_client_init(), so it's safe to init first and then prep your synthesizer.
 * For your callback, (c) is always in samples -- not frames, not bytes.
 */
int egg_audio_init_manual(int *rate,int *chanc,void (*cb)(float *v,int c));

int egg_audio_lock();
void egg_audio_unlock();
 
/* Friendly API.
 * By default, we'll create a synthesizer and take care of all the PCM work for you.
 * This is highly recommended, unless maybe you're porting some native game that already has its own synthesizer.
 *********************************************************************************/

/* Begin playing a song.
 * If the resource is not found, play silence.
 * (force) nonzero to restart from the beginning, if it's already playing.
 */
void egg_audio_play_song(int qual,int songid,int force,int repeat);

/* Play a fire-and-forget sound effect.
 * (trim) in 0..1, (pan) in -1..1 (left..right). Both are s16.16.
 */
void egg_audio_play_sound(int qual,int soundid,int trim,int pan);

/* Force an event onto the MIDI bus.
 * Beware that one bus is shared between the client and the background song.
 */
void egg_audio_event(int chid,int opcode,int a,int b);

/* Read the current song position, or forcibly change it.
 * Position is in beats from the start of the song.
 * When the song repeats, its playhead resets to zero.
 * Returns exactly zero if no song is playing, and never exactly zero if there is a song.
 */
double egg_audio_get_playhead();
void egg_audio_set_playhead(double beats);

#endif
