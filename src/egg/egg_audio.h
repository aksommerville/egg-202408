/* egg_audio.h
 */
 
#ifndef EGG_AUDIO_H
#define EGG_AUDIO_H

/* Begin playing a song.
 * If the resource is not found, play silence.
 * (force) nonzero to restart from the beginning, if it's already playing.
 */
void egg_audio_play_song(int qual,int songid,int force,int repeat);

/* Play a fire-and-forget sound effect.
 * (trim) in 0..1, (pan) in -1..1 (left..right).
 */
void egg_audio_play_sound(int qual,int soundid,double trim,double pan);

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
