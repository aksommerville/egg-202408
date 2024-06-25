#ifndef EGGDEV_INTERNAL_H
#define EGGDEV_INTERNAL_H

#include "opt/serial/serial.h"
#include "opt/fs/fs.h"
#include "opt/rom/rom.h"
#include "opt/http/http.h"
#include "egg/egg.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

#ifndef WABT_SDK /* Set via compiler, if you have it. */
  #define WABT_SDK ""
#endif

extern struct eggdev {
  const char *exename;
  const char *command;
  const char **srcpathv;
  int srcpathc,srcpatha;
  const char *dstpath;
  const char *format;
  int port;
  int external;
  const char *htdocs;
  int htdocsc;
  const char *override;
  int overridec;
  const char *runtime;
  int runtimec;
  const char *datapath;
  int datapathc;
  const char *rompath;
  const char *codepath;
  const char *typespath;
  char **name_by_tid; // 64 entries, if not null
  int named_only;
  struct http_context *http;
  int has_wd_makefile;
} eggdev;
 
struct eggdev_rpath {
  int tid,qual,rid;
  const char *name;
  int namec;
  char sfx[8];
  int sfxc;
  const char *path;
};

int eggdev_configure(int argc,char **argv);
void eggdev_print_help(const char *topic,int topicc);

int eggdev_type_repr(char *dst,int dsta,int tid);
const char *eggdev_type_repr_static(int tid);
int eggdev_type_eval(const char *src,int srcc);

int eggdev_pack_add_file(struct romw *romw,const char *path);
int eggdev_pack_digest(struct romw *romw,int toc_only);

int eggdev_bundle_html(const char *dstpath,const char *srcpath);
int eggdev_unbundle_html(void *dstpp,const char *srcpath);
int eggdev_unbundle_exe(void *dstpp,const char *srcpath);

int eggdev_tocflag_generate(struct sr_encoder *dst);

int eggdev_webtemplate_generate(struct sr_encoder *dst);

int eggdev_shell_script_(const char *scriptpath,...);
#define eggdev_shell_script(script,...) eggdev_shell_script_(script,##__VA_ARGS__,(void*)0)

int eggdev_command_sync(struct sr_encoder *dst,const char *cmd);

/* Read the given ROM file and if it has any wasm resources, create a temporary new one without them.
 * Returns (path) exactly if the file is OK as is.
 * Null if we tried and failed, unlikely.
 * Or the path to the temp file, which you should unlink when you're done.
 */
const char *eggdev_rewrite_rom_if_wasm(const char *path);

int eggdev_http_serve(struct http_xfer *req,struct http_xfer *rsp,void *userdata);

/* Split a file into multiple resources.
 * Add the resources to (romw) and return >0 on success.
 * 0 if it doesn't look sliceable; caller should proceed as a single resource.
 */
int eggdev_strings_slice(struct romw *romw,const char *src,int srcc,const struct eggdev_rpath *rpath);
int eggdev_sounds_slice(struct romw *romw,const char *src,int srcc,const struct eggdev_rpath *rpath);

/* Process individual resources.
 * We are not allowed to add or remove resources from the store by this point.
 * It's OK to zero the resource's ID if we determine it should be omitted.
 * Mostly these are expected to rewrite (res->serial).
 */
int eggdev_metadata_compile(struct romw *romw,struct romw_res *res);
int eggdev_wasm_compile(struct romw *romw,struct romw_res *res);
int eggdev_image_compile(struct romw *romw,struct romw_res *res);
int eggdev_song_compile(struct romw *romw,struct romw_res *res);
int eggdev_sound_compile(struct romw *romw,struct romw_res *res);

#endif
