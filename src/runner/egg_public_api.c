/* egg_public_api.c
 * This doesn't contain the entire public API.
 * Simple things, and things that just call out to some manager,
 * are implemented independently by egg_romsrc_external.c and egg_romsrc_native.c.
 */

#include "egg_runner_internal.h"

/* Get user languages.
 */

int egg_get_user_languages(int *dst,int dsta) {
  if (dsta<1) return 0;
  
  /* If we were launched with --lang, that's the one and only answer.
   */
  if (egg.config.lang) {
    dst[0]=egg.config.lang;
    return 1;
  }
  
  /* POSIX systems typically have LANG as the single preferred locale, which starts with a language code.
   * There can also be LANGUAGE, which is multiple language codes separated by colons.
   */
  int dstc=0;
  const char *src;
  if (src=getenv("LANG")) {
    if ((src[0]>='a')&&(src[0]<='z')&&(src[1]>='a')&&(src[1]<='z')) {
      if (dstc<dsta) dst[dstc++]=rom_qual_eval(src,2);
    }
  }
  if (dstc>=dsta) return dstc;
  if (src=getenv("LANGUAGE")) {
    int srcp=0;
    while (src[srcp]&&(dstc<dsta)) {
      const char *token=src+srcp;
      int tokenc=0;
      while (src[srcp]&&(src[srcp++]!=':')) tokenc++;
      if ((tokenc>=2)&&(token[0]>='a')&&(token[0]<='z')&&(token[1]>='a')&&(token[1]<='z')) {
        int lang=rom_qual_eval(token,2);
        int already=0,i=dstc;
        while (i-->0) if (dst[i]==lang) { already=1; break; }
        if (!already) dst[dstc++]=lang;
      }
    }
  }
  
  //TODO I'm sure there are other mechanisms for MacOS and Windows. Find those.
  return dstc;
}
