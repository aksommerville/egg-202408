/* egg_timer.h
 * Tracks and regulates real time.
 */
 
#ifndef EGG_TIMER_H
#define EGG_TIMER_H

double egg_timer_now();
double egg_timer_now_cpu();

struct egg_timer {
  double prevtime;
  double interval;
  double min_update;
  double max_update;
  double starttime_real;
  double starttime_cpu;
  int vframec;
  int faultc;
  int longc;
};

/* (tolerance) is 0..1, how far is the result of timer_tick allowed to deviate from (rate_hz).
 * At zero, tick will return 1/rate_hz every time regardless of real time elapsed.
 * At one, it will never sleep.
 */
void egg_timer_init(struct egg_timer *timer,int rate_hz,double tolerance);

/* Sleep if necessary, to advance the clock by one video frame.
 * Returns the time elapsed, clamped to the tolerance you specified at init.
 */
double egg_timer_tick(struct egg_timer *timer);

/* Dump a one-line report to stderr, whatever detail we have since startup.
 */
void egg_timer_report(struct egg_timer *timer);

#endif
