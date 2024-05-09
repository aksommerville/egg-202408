/* egg_store.h
 * Access to the ROM resources, and persistent storage.
 */
 
#ifndef EGG_STORE_H
#define EGG_STORE_H

#define EGG_RESTYPE_metadata 1
#define EGG_RESTYPE_wasm 2
#define EGG_RESTYPE_string 3
#define EGG_RESTYPE_image 4
#define EGG_RESTYPE_song 5
#define EGG_RESTYPE_sound 6

#define EGG_RESTYPE_FOR_EACH \
  _(metadata) \
  _(wasm) \
  _(string) \
  _(image) \
  _(song) \
  _(sound)

/* Copy a resource from the ROM to client memory.
 * Always returns the actual length, never negative.
 * If >dsta, nothing is written to (dst).
 */
int egg_res_get(void *dst,int dsta,int tid,int qual,int rid);

/* Trigger (cb) for every resource in the store, in order.
 * You shouldn't need this, after all they are your own resources.
 */
int egg_res_for_each(int (*cb)(int tid,int qual,int rid,int len,void *userdata),void *userdata);

/* Access to a persistent key=value text store.
 * The user gets control over this. It might be limited in size and might not be allowed at all.
 * If we know a change can't be stored, we fail fast, we won't fake it.
 * To delete a field, set its value empty. Empty and absent fields are indistinguishable.
 * Keys and values must be UTF-8. We don't guarantee intact storage if misencoded.
 * Keys have a hard length limit of 255 bytes. Values, no definite limit.
 */
int egg_store_get(char *dst,int dsta,const char *k,int kc);
int egg_store_set(const char *k,int kc,const char *v,int vc);
int egg_store_key_by_index(char *dst,int dsta,int p);

#endif
