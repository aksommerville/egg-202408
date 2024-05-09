/* rom.h
 * Read-only interface to Egg ROM files.
 */
 
#ifndef ROM_H
#define ROM_H

#include <stdint.h>

struct sr_encoder;

/* Standard resource types.
 *XXX move to egg_store.h
#define RESTYPE_metadata 1
#define RESTYPE_wasm 2
#define RESTYPE_string 3
#define RESTYPE_image 4
#define RESTYPE_song 5
#define RESTYPE_sound 6

#define RESTYPE_FOR_EACH \
  _(metadata) \
  _(wasm) \
  _(string) \
  _(image) \
  _(song) \
  _(sound)
/**/

/* Normal read-only interface.
 **********************************************************/

struct rom {
  const uint8_t *serial;
  int serialc;
  int ownserial;
  struct rom_res {
    uint32_t fqrid;
    const void *v; // WEAK, points into (serial)
    int c;
  } *resv;
  int resc,resa;
};

void rom_cleanup(struct rom *rom);

int rom_init_borrow(struct rom *rom,const void *src,int srcc);
int rom_init_handoff(struct rom *rom,const void *src,int srcc);
int rom_init_copy(struct rom *rom,const void *src,int srcc);

int rom_get(void *dstpp,struct rom *rom,int tid,int qual,int rid);

void rom_unpack_fqrid(int *tid,int *qual,int *rid,uint32_t fqrid);
void rom_qual_repr(char *dst/*2*/,int qual);
int rom_qual_eval(const char *src,int srcc);

/* Read/write interface for build-time tools.
 **************************************************************/
 
struct romw {
  struct romw_res {
    int tid,qual,rid; // If (0,0,0), we'll quietly ignore at encode.
    char *name; // Names don't persist in the archive.
    int namec;
    char *path;
    int pathc;
    int lineno0; // If it came from a multi-resource text file.
    void *serial;
    int serialc;
    int hint; // For formatters, no generic meaning.
  } *resv;
  int resc,resa;
};

void romw_cleanup(struct romw *romw);

/* Resources are added to the end with no regard to order or uniqueness.
 * You have to filter and sort before encoding -- that can come later.
 */
struct romw_res *romw_res_add(struct romw *romw);
int romw_res_set_name(struct romw_res *res,const char *src,int srcc);
int romw_res_set_path(struct romw_res *res,const char *src,int srcc);
int romw_res_set_serial(struct romw_res *res,const void *src,int srcc);
void romw_res_handoff_serial(struct romw_res *res,void *v,int c);

/* We check first for the (qual) you ask for.
 * If that's not found, but a qual zero exists, we return that instead.
 * Name searches are exact.
 */
struct romw_res *romw_res_get_by_id(const struct romw *romw,int tid,int qual,int rid);
struct romw_res *romw_res_get_by_name(const struct romw *romw,int tid,int qual,const char *name,int namec);

/* Remove a range of resources, or remove all those where your callback returns zero.
 */
void romw_remove(struct romw *romw,int p,int c);
void romw_filter(struct romw *romw,int (*cb)(const struct romw_res *res,void *userdata),void *userdata);

/* You must sort before encoding.
 */
void romw_sort(struct romw *romw);

/* Produce the final serial archive.
 * Ignores any resources with ids (0,0,0).
 * All others must be sorted (tid,qual,rid). We check, and fail loudly on any violation.
 */
int romw_encode(struct sr_encoder *dst,const struct romw *romw);

#endif
