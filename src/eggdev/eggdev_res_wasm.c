#include "eggdev_internal.h"

/* Every function available to Egg games.
 * You can rebuild this list manually by copying out of src/runner/egg_romsrc_external.c
 * or src/www/js/Exec.js, they are the authorities.
 */
static const char *egg_platform_functions[]={
  "env.egg_log",
  "env.egg_time_real",
  "env.egg_time_local",
  "env.egg_request_termination",
  "env.egg_get_user_languages",
  "env.egg_video_set_string_buffer",
  "env.egg_video_get_size",
  "env.egg_texture_del",
  "env.egg_texture_new",
  "env.egg_texture_get_header",
  "env.egg_texture_load_image",
  "env.egg_texture_upload",
  "env.egg_texture_clear",
  "env.egg_render_tint",
  "env.egg_render_alpha",
  "env.egg_draw_rect",
  "env.egg_draw_line",
  "env.egg_draw_trig",
  "env.egg_draw_decal",
  "env.egg_draw_decal_mode7",
  "env.egg_draw_tile",
  "env.egg_image_get_header",
  "env.egg_image_decode",
  "env.egg_res_get",
  "env.egg_res_for_each",
  "env.egg_store_get",
  "env.egg_store_set",
  "env.egg_store_key_by_index",
  "env.egg_event_get",
  "env.egg_event_enable",
  "env.egg_show_cursor",
  "env.egg_lock_cursor",
  "env.egg_joystick_devid_by_index",
  "env.egg_joystick_get_ids",
  "env.egg_joystick_get_name",
  "env.egg_joystick_for_each_button",
  "env.egg_audio_play_song",
  "env.egg_audio_play_sound",
  "env.egg_audio_event",
  "env.egg_audio_get_playhead",
  "env.egg_audio_set_playhead",
  
  // ...and of course, the 142 functions of GLES2:
  "env.glActiveTexture",
  "env.glAttachShader",
  "env.glBindAttribLocation",
  "env.glBindBuffer",
  "env.glBindFramebuffer",
  "env.glBindRenderbuffer",
  "env.glBindTexture",
  "env.glBlendColor",
  "env.glBlendEquation",
  "env.glBlendEquationSeparate",
  "env.glBlendFunc",
  "env.glBlendFuncSeparate",
  "env.glBufferData",
  "env.glBufferSubData",
  "env.glCheckFramebufferStatus",
  "env.glClear",
  "env.glClearColor",
  "env.glClearDepthf",
  "env.glClearStencil",
  "env.glColorMask",
  "env.glCompileShader",
  "env.glCompressedTexImage2D",
  "env.glCompressedTexSubImage2D",
  "env.glCopyTexImage2D",
  "env.glCopyTexSubImage2D",
  "env.glCreateProgram",
  "env.glCreateShader",
  "env.glCullFace",
  "env.glDeleteBuffers",
  "env.glDeleteFramebuffers",
  "env.glDeleteProgram",
  "env.glDeleteRenderbuffers",
  "env.glDeleteShader",
  "env.glDeleteTextures",
  "env.glDepthFunc",
  "env.glDepthMask",
  "env.glDepthRangef",
  "env.glDetachShader",
  "env.glDisable",
  "env.glDisableVertexAttribArray",
  "env.glDrawArrays",
  "env.glDrawElements",
  "env.glEnable",
  "env.glEnableVertexAttribArray",
  "env.glFinish",
  "env.glFlush",
  "env.glFramebufferRenderbuffer",
  "env.glFramebufferTexture2D",
  "env.glFrontFace",
  "env.glGenBuffers",
  "env.glGenerateMipmap",
  "env.glGenFramebuffers",
  "env.glGenRenderbuffers",
  "env.glGenTextures",
  "env.glGetActiveAttrib",
  "env.glGetActiveUniform",
  "env.glGetAttachedShaders",
  "env.glGetAttribLocation",
  "env.glGetBooleanv",
  "env.glGetBufferParameteriv",
  "env.glGetError",
  "env.glGetFloatv",
  "env.glGetFramebufferAttachmentParameteriv",
  "env.glGetIntegerv",
  "env.glGetProgramiv",
  "env.glGetProgramInfoLog",
  "env.glGetRenderbufferParameteriv",
  "env.glGetShaderiv",
  "env.glGetShaderInfoLog",
  "env.glGetShaderPrecisionFormat",
  "env.glGetShaderSource",
  "env.glGetString",
  "env.glGetTexParameterfv",
  "env.glGetTexParameteriv",
  "env.glGetUniformfv",
  "env.glGetUniformiv",
  "env.glGetUniformLocation",
  "env.glGetVertexAttribfv",
  "env.glGetVertexAttribiv",
  "env.glGetVertexAttribPointerv",
  "env.glHint",
  "env.glIsBuffer",
  "env.glIsEnabled",
  "env.glIsFramebuffer",
  "env.glIsProgram",
  "env.glIsRenderbuffer",
  "env.glIsShader",
  "env.glIsTexture",
  "env.glLineWidth",
  "env.glLinkProgram",
  "env.glPixelStorei",
  "env.glPolygonOffset",
  "env.glReadPixels",
  "env.glReleaseShaderCompiler",
  "env.glRenderbufferStorage",
  "env.glSampleCoverage",
  "env.glScissor",
  "env.glShaderBinary",
  "env.glShaderSource",
  "env.glStencilFunc",
  "env.glStencilFuncSeparate",
  "env.glStencilMask",
  "env.glStencilMaskSeparate",
  "env.glStencilOp",
  "env.glStencilOpSeparate",
  "env.glTexImage2D",
  "env.glTexParameterf",
  "env.glTexParameterfv",
  "env.glTexParameteri",
  "env.glTexParameteriv",
  "env.glTexSubImage2D",
  "env.glUniform1f",
  "env.glUniform1fv",
  "env.glUniform1i",
  "env.glUniform1iv",
  "env.glUniform2f",
  "env.glUniform2fv",
  "env.glUniform2i",
  "env.glUniform2iv",
  "env.glUniform3f",
  "env.glUniform3fv",
  "env.glUniform3i",
  "env.glUniform3iv",
  "env.glUniform4f",
  "env.glUniform4fv",
  "env.glUniform4i",
  "env.glUniform4iv",
  "env.glUniformMatrix2fv",
  "env.glUniformMatrix3fv",
  "env.glUniformMatrix4fv",
  "env.glUseProgram",
  "env.glValidateProgram",
  "env.glVertexAttrib1f",
  "env.glVertexAttrib1fv",
  "env.glVertexAttrib2f",
  "env.glVertexAttrib2fv",
  "env.glVertexAttrib3f",
  "env.glVertexAttrib3fv",
  "env.glVertexAttrib4f",
  "env.glVertexAttrib4fv",
  "env.glVertexAttribPointer",
  "env.glViewport",

0};

/* Validate imports.
 * wasm-objdump -x -j Import %
 */
 
static void eggdev_wasm_validate_imports(const char *src,int srcc,const char *path) {
  struct sr_decoder decoder={.v=src,.c=srcc};
  const char *line;
  int linec;
  while ((linec=sr_decode_line(&line,&decoder))>0) {
    while (linec&&((unsigned char)line[0]<=0x20)) { line++; linec--; }
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    if (!linec) continue;
    
    // - func[0] sig=0 <egg_log> <- env.egg_log
    if (line[0]!='-') continue;
    int linep=1;
    while ((linep<linec)&&((unsigned char)line[linep]<=0x20)) linep++;
    const char *type=line+linep;
    int typec=0;
    while ((linep<linec)&&(line[linep]>='a')&&(line[linep]<='z')) { typec++; linep++; }
    const char *name=line+linec;
    int namec=0;
    while ((namec<linec)&&((unsigned char)name[-1]>0x20)) { name--; namec++; }
    if (!typec||!namec) continue; // shouldn't happen but this code is quick-n-dirty, who knows.
    
    if ((typec==4)&&!memcmp(type,"func",4)) {
      int ok=0;
      const char **okfn=egg_platform_functions;
      for (;*okfn;okfn++) {
        if (memcmp(*okfn,name,namec)) continue;
        if ((*okfn)[namec]) continue;
        ok=1;
        break;
      }
      if (!ok) {
        fprintf(stderr,
          "%s:WARNING: Unknown import '%.*s'.\n",
          path,namec,name
        );
      }
      
    } else {
      fprintf(stderr,
        "%s:WARNING: Unexpected import type '%.*s' ('%.*s').\n",
        path,typec,type,namec,name
      );
    }
  }
}

/* Validate exports.
 * wasm-objdump -x -j Export %
 */
 
static void eggdev_wasm_validate_exports(const char *src,int srcc,const char *path) {
  int have_memory=0;
  int have_egg_client_quit=0;
  int have_egg_client_init=0;
  int have_egg_client_update=0;
  int have_egg_client_render=0;
  struct sr_decoder decoder={.v=src,.c=srcc};
  const char *line;
  int linec;
  while ((linec=sr_decode_line(&line,&decoder))>0) {
    while (linec&&((unsigned char)line[0]<=0x20)) { line++; linec--; }
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    if (!linec) continue;

    if (line[0]!='-') continue;
    if (line[linec-1]!='"') continue;
    int linep=1;
    while ((linep<linec)&&((unsigned char)line[linep]<=0x20)) linep++;
    const char *type=line+linep;
    int typec=0;
    while ((linep<linec)&&(line[linep]>='a')&&(line[linep]<='z')) { typec++; linep++; }
    const char *name=line+linec-1;
    int namec=0;
    while ((namec<linec-1)&&((unsigned char)name[-1]!='"')) { name--; namec++; }
    if (!typec||!namec) continue; // shouldn't happen but this code is quick-n-dirty, who knows.
    
    if ((typec==6)&&!memcmp(type,"memory",6)&&(namec==6)&&!memcmp(name,"memory",6)) {
      have_memory=1;
    } else if ((typec==5)&&!memcmp(type,"table",5)&&(namec==25)&&!memcmp(name,"__indirect_function_table",25)) {
      // If it doesn't use any callbacks, this wouldn't be exported. No worries.
    } else if ((typec==4)&&!memcmp(type,"func",4)) {
           if ((namec==15)&&!memcmp(name,"egg_client_quit",15)) have_egg_client_quit=1;
      else if ((namec==15)&&!memcmp(name,"egg_client_init",15)) have_egg_client_init=1;
      else if ((namec==17)&&!memcmp(name,"egg_client_update",17)) have_egg_client_update=1;
      else if ((namec==17)&&!memcmp(name,"egg_client_render",17)) have_egg_client_render=1;
      else fprintf(stderr,"%s:WARNING: Unexpected export '%.*s'.\n",path,namec,name);
    } else {
      fprintf(stderr,"%s:WARNING: Unexpected export '%.*s' (type '%.*s').\n",path,namec,name,typec,type);
    }
  }
  if (!have_memory) fprintf(stderr,"%s:WARNING: Module does not export 'memory'\n",path);
  if (!have_egg_client_quit) fprintf(stderr,"%s:WARNING: Missing required entry point 'egg_client_quit'\n",path);
  if (!have_egg_client_init) fprintf(stderr,"%s:WARNING: Missing required entry point 'egg_client_init'\n",path);
  if (!have_egg_client_update) fprintf(stderr,"%s:WARNING: Missing required entry point 'egg_client_update'\n",path);
  if (!have_egg_client_render) fprintf(stderr,"%s:WARNING: Missing required entry point 'egg_client_render'\n",path);
}

/* Validate wasm resource.
 * It's called "compile" for consistency with the rest of eggdev, but that name is misleading here.
 * We don't compile anything to WebAssembly, our caller does that before calling us.
 */
 
int eggdev_wasm_compile(struct romw *romw,struct romw_res *res) {
  if (!WABT_SDK[0]) {
    // Not sure it's worth a warning. One could see that getting old fast.
    //fprintf(stderr,"%s:WARNING: Unable to validate WebAssembly linkage. Rebuild %s with -DWABT_SDK=... to enable this check.\n",res->path,eggdev.exename);
    return 0;
  }
  struct sr_encoder output={0};
  char cmd[1024];
  int cmdc;
  
  if (((cmdc=snprintf(cmd,sizeof(cmd),WABT_SDK"/bin/wasm-objdump -x -j Import %s",res->path))>0)&&(cmdc<sizeof(cmd))) {
    if (eggdev_command_sync(&output,cmd)<0) {
      fprintf(stderr,"%s: Failed to list imports! We can not validate correct linkage at this time.\n",res->path);
    } else {
      eggdev_wasm_validate_imports(output.v,output.c,res->path);
    }
  }
  
  output.c=0;
  if (((cmdc=snprintf(cmd,sizeof(cmd),WABT_SDK"/bin/wasm-objdump -x -j Export %s",res->path))>0)&&(cmdc<sizeof(cmd))) {
    if (eggdev_command_sync(&output,cmd)<0) {
      fprintf(stderr,"%s: Failed to list exports! We can not validate correct linkage at this time.\n",res->path);
    } else {
      eggdev_wasm_validate_exports(output.v,output.c,res->path);
    }
  }
  
  sr_encoder_cleanup(&output);
  return 0;
}
