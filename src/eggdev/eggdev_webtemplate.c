#include "eggdev_internal.h"

/* Line number.
 */
 
static int lineno(const char *src,int srcc) {
  int lineno=1,srcp=0;
  while (srcp<srcc) {
    if (src[srcp]==0x0a) lineno++;
    srcp++;
  }
  return lineno;
}

/* Identifiers.
 * Javascript allows a crazy set of things in identifiers. We'll be more restrained about it.
 */
 
static inline int js_isident(char ch) {
  if ((ch>='0')&&(ch<='9')) return 1;
  if ((ch>='a')&&(ch<='z')) return 1;
  if ((ch>='A')&&(ch<='Z')) return 1;
  if (ch=='_') return 1;
  if (ch=='$') return 1;
  return 0;
}

/* Whitespace.
 * Not sure exactly what Javascript's rules are, they're probably insane, but we'll say 0x00..0x20 are whitespace and nothing else.
 */
 
static int js_measure_space(const char *src,int srcc) {
  int srcp=0;
  for (;;) {
    if (srcp>=srcc) return srcp;
    
    if ((unsigned char)src[srcp]<=0x20) {
      srcp++;
      continue;
    }
    
    if (srcp>=srcc-2) return srcp;
    if (src[srcp]!='/') return srcp;
    if (src[srcp+1]=='/') {
      srcp+=2;
      while ((srcp<srcc)&&(src[srcp]!=0x0a)) srcp++;
      continue;
    }
    if (src[srcp+1]=='*') {
      srcp+=2;
      int stopp=srcc-2;
      for (;;) {
        if (srcp>stopp) return -1;
        if ((src[srcp]=='*')&&(src[srcp+1]=='/')) { srcp+=2; break; }
        srcp++;
      }
      continue;
    }
    return srcp;
  }
}

/* Measure token.
 * Caller ensures that we're not at space.
 * We need the previous token, to identify inline regex.
 * We only allow inline regex after '(' or '='.
 * There must be a smarter way to identify them but I can't think of one.
 */
 
static int js_measure_token(const char *src,int srcc,const char *pv,int pvc) {
  if (srcc<1) return 0;
  
  // Plain strings or inline regex. Forbid newline.
  if ((src[0]=='"')||(src[0]=='\'')||(
    ((src[0]=='/')&&(pvc==1)&&((pv[0]=='(')||(pv[0]=='=')))
  )) {
    int srcp=1;
    for (;;) {
      if (srcp>=srcc) return -1;
      if (src[srcp]==0x0a) return -1;
      if (src[srcp]==src[0]) return srcp+1;
      if (src[srcp]=='\\') srcp+=2;
      else srcp+=1;
    }
  }
  
  // Grave string. Allow newline.
  if (src[0]=='`') {
    int srcp=1;
    for (;;) {
      if (srcp>=srcc) return -1;
      if (src[srcp]==src[0]) return srcp+1;
      if (src[srcp]=='\\') srcp+=2;
      else srcp+=1;
    }
  }
  
  // Identifier, keyword, or integer literal.
  if (js_isident(src[0])) {
    int srcp=1;
    while ((srcp<srcc)&&js_isident(src[srcp])) srcp++;
    return srcp;
  }
  
  // Everything else can emit as one byte.
  return 1;
}

/* Minify one Javascript file.
 * As currently structured, each file can be processed independently of the others.
 */
 
static int eggdev_webtemplate_minify_javascript_1(struct sr_encoder *dst,const char *src,int srcc,const char *path) {
  int dstc_linestart=dst->c;
  int srcp=0;
  const char *pvtoken=0;
  int pvtokenc=0;
  for (;;) {
    int err=js_measure_space(src+srcp,srcc-srcp);
    if (err<0) {
      fprintf(stderr,"%s:%d: Tokenization error (unclosed comment?)\n",path,lineno(src,srcp));
      return -2;
    }
    srcp+=err;
    if (srcp>=srcc) break;
    const char *token=src+srcp;
    int tokenc=js_measure_token(token,srcc-srcp,pvtoken,pvtokenc);
    if (tokenc<1) {
      fprintf(stderr,"%s:%d: Tokenization error starting at '%c'\n",path,lineno(src,srcp),src[srcp]);
      return -2;
    }
    
    // "export" can be blindly dropped.
    if ((tokenc==6)&&!memcmp(token,"export",6)) {
    
    // "import", we can drop the whole line.
    } else if ((tokenc==6)&&!memcmp(token,"import",6)) {
      srcp+=tokenc;
      tokenc=0;
      while ((srcp<srcc)&&(src[srcp++]!=0x0a)) ;
      
    // Everything else: Space if required, verbatim token, newline if we feel like it.
    } else {
      if ((dst->c>0)&&js_isident(((char*)dst->v)[dst->c-1])&&js_isident(token[0])) {
        if (sr_encode_u8(dst,0x20)<0) return -1;
      }
      if (sr_encode_raw(dst,token,tokenc)<0) return -1;
      // I'd like to just dump a newline every 100 bytes or so.
      // Can't for two reasons: (1) We split multibyte operators, and (2) we'd cause semicolon insertion in undesired places.
      // So instead, reach 100 bytes and then wait for a semicolon or closing brace.
      if ((dst->c-dstc_linestart>=100)&&(tokenc==1)&&((token[0]==';')||(token[0]=='}'))) {
        if (sr_encode_u8(dst,0x0a)<0) return -1;
        dstc_linestart=dst->c;
      }
    }
    
    pvtoken=token;
    pvtokenc=tokenc;
    srcp+=tokenc;
  }
  // Finally, a newline at the end of each file, for no particular reason.
  if (dst->c&&(((char*)dst->v)[dst->c-1]!=0x0a)) {
    if (sr_encode_u8(dst,0x0a)<0) return -1;
  }
  return 0;
}

/* Minify Javascript from (eggdev.srcpathv).
 * Output begins immediately at the end of (dst).
 * We always produce a final newline.
 */
 
static int eggdev_webtemplate_minify_javascript(struct sr_encoder *dst) {

  /* For now I'm keeping it simple: 
   *  - Drop "export" (the keyword only).
   *  - Drop import statements.
   *  - Concatenate in the order provided.
   *  - Eliminate comments and whitespace.
   *  - Single space where needed to break tokens.
   *  - Newline at the end, and at tasteful intervals between tokens.
   * It might be a problem if constructions like "</script>" appear in a string in the Javascript.
   * I don't expect that to come up so I'm not accounting for it.
   * We're able to satisfy the rules above with a stateless token stream, easy as pie.
   * Might need something more involved in the future, eg if we want to inline constants or reduce the size of identifiers.
   */
   
  int i=0; for (;i<eggdev.srcpathc;i++) {
    char *src=0;
    int srcc=file_read(&src,eggdev.srcpathv[i]);
    if (srcc<0) {
      fprintf(stderr,"%s: Failed to read file.\n",eggdev.srcpathv[i]);
      return -2;
    }
    int err=eggdev_webtemplate_minify_javascript_1(dst,src,srcc,eggdev.srcpathv[i]);
    free(src);
    if (err<0) {
      if (err!=-2) fprintf(stderr,"%s: Unspecified error during Javascript minification.\n",eggdev.srcpathv[i]);
      return -2;
    }
  }
  return 0;
}

/* Generate template.
 */
 
int eggdev_webtemplate_generate(struct sr_encoder *dst) {
  
  //TODO We probably need a third insertion point, for extra <head> things like <meta> and <title>.
  if (sr_encode_raw(dst,"<!DOCTYPE html>\n<html><head><style>\ncanvas {\n",-1)<0) return -1;
  if (sr_encode_raw(dst,"INSERT CANVAS STYLE HERE\n",-1)<0) return -1;
  if (sr_encode_raw(dst,"}\n</style><egg-rom style=\"display:none\">\n",-1)<0) return -1;
  if (sr_encode_raw(dst,"INSERT ROM HERE\n",-1)<0) return -1;
  if (sr_encode_raw(dst,"</egg-rom><script>\n",-1)<0) return -1;
  
  int err=eggdev_webtemplate_minify_javascript(dst);
  if (err<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error minifying Javascript.\n",eggdev.exename);
    return -2;
  }
  
  if (sr_encode_raw(dst,"</script></head><body></body></html>\n",-1)<0) return -1;
  return 0;
}
