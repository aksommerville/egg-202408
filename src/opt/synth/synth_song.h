/* synth_song.h
 */
 
#ifndef SYNTH_SONG_H
#define SYNTH_SONG_H

struct synth_song {
  const uint8_t *src;
  int srcc;
  void *keepalive;
  int repeat;
  int srcp;
  int delay; // frames
  float frames_per_ms;
  int tempo; // ms/qnote, advisory only
  int qual,songid;
  int startp,loopp;
  int playhead_ms; // We make an effort to keep current at each advance.
  int playhead_ms_next; // Force (playhead_ms) here when delay depletes. Avoids some rounding error.
};

void synth_song_del(struct synth_song *song);

struct synth_song *synth_song_new(
  struct synth *synth,
  const void *src,int srcc,
  int safe_to_borrow,
  int repeat,
  int qual,int songid
);

static inline int synth_song_is_resource(const struct synth_song *song,int qual,int songid) {
  return ((song->qual==qual)&&(song->songid==songid));
}

static inline double synth_song_get_playhead(const struct synth *synth,const struct synth_song *song,double adjust) {
  return ((double)song->playhead_ms-(adjust*1000.0))/(double)song->tempo;
}

/* Commit new channels and procs to (synth), make ready to play (song).
 * All song channels must be null before calling.
 */
int synth_song_init_channels(struct synth *synth,struct synth_song *song);

/* Trigger any events at current playhead, then return frame count to next event.
 * Returns 0 at EOF (if not repeating), or <0 for real errors.
 * (skip) nonzero to suppress actual event delivery -- only synth_song_set_playhead() should use that.
 */
int synth_song_update(struct synth *synth,struct synth_song *song,int skip);

/* Advance playhead by so many frames.
 * This must not be more than the last thing we returned at synth_song_update().
 */
void synth_song_advance(struct synth_song *song,int framec);

/* Reset to start, then discard events until we're close to the requested beat.
 */
void synth_song_set_playhead(struct synth *synth,struct synth_song *song,double beats);

// Sum of all delays. We have to read the whole song each time you ask for it.
double synth_song_get_duration(const struct synth_song *song);

#endif
