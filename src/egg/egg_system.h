/* egg_system.h
 * Global odds and ends.
 */
 
#ifndef EGG_SYSTEM_H
#define EGG_SYSTEM_H

/* Dump text to a log for debug purposes.
 * This text is generally not visible to the user. (but certainly can be)
 * Do not end with a newline, the platform will add one.
 * Accepts the same format as printf.
 */
void egg_log(const char *fmt,...);

/* Real time in seconds since some undefined epoch.
 */
double egg_time_real();

/* Split current time, suitable for display to the user.
 * Fills (v) up to (a): [year,month,day,hour,minute,second,millisecond]
 * Values are all normalized for display, eg on 8 May 2024 it returns [2024,5,8].
 * Hour is in 0..23.
 */
void egg_time_local(int *v,int a);

/* Ask that the platform shut down at its convenience.
 * Not guaranteed to terminate at all, and it will usually not be immediate.
 */
void egg_request_termination();

/* Fill (v) with language qualifiers, in the order of the user's preference.
 * We may return (>a) if more are available, but in that case we do populate up to (a).
 * A language qualifier is ten bits, the way we use them in ROM files.
 * For presentation: [
 *   "abcdefghijklmnopqrstuvwxyz123456"[(v>>5)&31],
 *   "abcdefghijklmnopqrstuvwxyz123456"[v&31],
 * ]
 */
int egg_get_user_languages(int *v,int a);

#endif
