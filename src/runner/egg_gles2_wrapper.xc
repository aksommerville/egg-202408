/* egg_gles2_wrapper.xc
 * Included by egg_romsrc_external.c
 * We implement all 142 functions of GLES2,with a suffix "_wasm", and parameters slightly modified for transit.
 * We're as close as we can get to a direct pass-through to GLES2. No extra behavior.
 */
 
#include <GLES2/gl2.h>

static int egg_gles2_buffer_bound=0;

/* How many elements in a glGet or glSet of this parameter?
 * glTexParameter are also accepted here.
 */
 
static int egg_gl_get_tuplesize(int pname) {
  switch (pname) {
    case GL_ACTIVE_TEXTURE:
    case GL_ALPHA_BITS:
    case GL_ARRAY_BUFFER_BINDING:
    case GL_BLEND:
    case GL_BLEND_DST_ALPHA:
    case GL_BLEND_DST_RGB:
    case GL_BLEND_EQUATION_ALPHA:
    case GL_BLEND_EQUATION_RGB:
    case GL_BLEND_SRC_ALPHA:
    case GL_BLEND_SRC_RGB:
    case GL_BLUE_BITS:
    case GL_CULL_FACE:
    case GL_CULL_FACE_MODE:
    case GL_CURRENT_PROGRAM:
    case GL_DEPTH_BITS:
    case GL_DEPTH_CLEAR_VALUE:
    case GL_DEPTH_FUNC:
    case GL_DEPTH_TEST:
    case GL_DEPTH_WRITEMASK:
    case GL_DITHER:
    case GL_ELEMENT_ARRAY_BUFFER_BINDING:
    case GL_FRONT_FACE:
    case GL_GENERATE_MIPMAP_HINT:
    case GL_GREEN_BITS:
    case GL_IMPLEMENTATION_COLOR_READ_FORMAT:
    case GL_IMPLEMENTATION_COLOR_READ_TYPE:
    case GL_LINE_WIDTH:
    case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:
    case GL_MAX_CUBE_MAP_TEXTURE_SIZE:
    case GL_MAX_FRAGMENT_UNIFORM_VECTORS:
    case GL_MAX_RENDERBUFFER_SIZE:
    case GL_MAX_TEXTURE_IMAGE_UNITS:
    case GL_MAX_TEXTURE_SIZE:
    case GL_MAX_VARYING_VECTORS:
    case GL_MAX_VERTEX_ATTRIBS:
    case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
    case GL_MAX_VERTEX_UNIFORM_VECTORS:
    case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
    case GL_NUM_SHADER_BINARY_FORMATS:
    case GL_PACK_ALIGNMENT:
    case GL_POLYGON_OFFSET_FACTOR:
    case GL_POLYGON_OFFSET_FILL:
    case GL_POLYGON_OFFSET_UNITS:
    case GL_RED_BITS:
    case GL_RENDERBUFFER_BINDING:
    case GL_SAMPLE_ALPHA_TO_COVERAGE:
    case GL_SAMPLE_BUFFERS:
    case GL_SAMPLE_COVERAGE:
    case GL_SAMPLE_COVERAGE_INVERT:
    case GL_SAMPLE_COVERAGE_VALUE:
    case GL_SAMPLES:
    case GL_SCISSOR_TEST:
    case GL_SHADER_COMPILER:
    case GL_STENCIL_BACK_FAIL:
    case GL_STENCIL_BACK_FUNC:
    case GL_STENCIL_BACK_PASS_DEPTH_FAIL:
    case GL_STENCIL_BACK_PASS_DEPTH_PASS:
    case GL_STENCIL_BACK_REF:
    case GL_STENCIL_BACK_VALUE_MASK:
    case GL_STENCIL_BACK_WRITEMASK:
    case GL_STENCIL_BITS:
    case GL_STENCIL_CLEAR_VALUE:
    case GL_STENCIL_FAIL:
    case GL_STENCIL_FUNC:
    case GL_STENCIL_PASS_DEPTH_FAIL:
    case GL_STENCIL_PASS_DEPTH_PASS:
    case GL_STENCIL_REF:
    case GL_STENCIL_TEST:
    case GL_STENCIL_VALUE_MASK:
    case GL_STENCIL_WRITEMASK:
    case GL_SUBPIXEL_BITS:
    case GL_TEXTURE_BINDING_2D:
    case GL_TEXTURE_BINDING_CUBE_MAP:
    case GL_UNPACK_ALIGNMENT:
    case GL_TEXTURE_MAG_FILTER:
    case GL_TEXTURE_MIN_FILTER:
    case GL_TEXTURE_WRAP_S:
    case GL_TEXTURE_WRAP_T:
    case GL_BUFFER_SIZE:
    case GL_BUFFER_USAGE:
    case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
    case GL_RENDERBUFFER_WIDTH:
    case GL_RENDERBUFFER_HEIGHT:
    case GL_RENDERBUFFER_INTERNAL_FORMAT:
    case GL_RENDERBUFFER_RED_SIZE:
    case GL_RENDERBUFFER_GREEN_SIZE:
    case GL_RENDERBUFFER_BLUE_SIZE:
    case GL_RENDERBUFFER_ALPHA_SIZE:
    case GL_RENDERBUFFER_DEPTH_SIZE:
    case GL_RENDERBUFFER_STENCIL_SIZE:
      return 1;
    case GL_ALIASED_LINE_WIDTH_RANGE:
    case GL_ALIASED_POINT_SIZE_RANGE:
    case GL_DEPTH_RANGE:
    case GL_MAX_VIEWPORT_DIMS:
      return 2;
    case GL_BLEND_COLOR:
    case GL_COLOR_CLEAR_VALUE:
    case GL_COLOR_WRITEMASK:
    case GL_SCISSOR_BOX:
    case GL_VIEWPORT:
      return 4;
    case GL_COMPRESSED_TEXTURE_FORMATS: {
        GLint n=0;
        glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS,&n);
        return n;
      }
    case GL_SHADER_BINARY_FORMATS: {
        GLint n=0;
        glGetIntegerv(GL_NUM_SHADER_BINARY_FORMATS,&n);
        return n;
      }
  }
  return 0;
}

/* Calculate client memory length of image for glReadPixels or glTexImage2D.
 */
 
static int egg_gl_measure_image(int format,int type,int w,int h) {
  if ((w<1)||(h<1)) return 0;
  if ((w>4096)||(h>4096)) return 0;
  int chanc=0;
  switch (format) {
    case GL_STENCIL_INDEX8:
    case GL_DEPTH_COMPONENT:
    case GL_LUMINANCE:
    case GL_ALPHA: chanc=1; break;
    case GL_LUMINANCE_ALPHA: chanc=2; break;
    case GL_RGB: chanc=3; break;
    case GL_RGBA: chanc=4; break;
  }
  if (chanc<1) return 0;
  int wordlen=0;
  switch (type) {
    // GL_HALF_FLOAT is legal here. Not sure wasm has such a thing.
    case GL_UNSIGNED_BYTE:
    case GL_BYTE:
      wordlen=1;
      break;
    case GL_UNSIGNED_SHORT:
    case GL_SHORT:
    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_5_5_5_1:
      wordlen=2;
      break;
    case GL_UNSIGNED_INT:
    case GL_INT:
    case GL_FLOAT:
      wordlen=4;
      break;
  }
  if (wordlen<1) return 0;
  return wordlen*chanc*w*h;
}

/* glGetString is unique in its complexity, because it needs to return strings in client memory.
 * I'm not sure how we'll go about this.
 */
 
static void egg_wasm_video_set_string_buffer(wasm_exec_env_t ee,char *v,int c) {
  egg.glstr=v;
  egg.glstra=c;
  egg.glstrp=0;
}

static int glGetString_wasm(wasm_exec_env_t ee,int name) {
  if (!egg.glstr) return 0;
  const char *src=glGetString(name);
  int srcc=0;
  if (src) while (src[srcc]) srcc++;
  if (srcc>=egg.glstra) srcc=egg.glstra-1;
  if (egg.glstrp>=egg.glstra-srcc) egg.glstrp=0;
  char *dst=egg.glstr+egg.glstrp;
  memcpy(dst,src,srcc);
  dst[srcc]=0;
  egg.glstrp+=srcc+1;
  return wamr_host_to_client_pointer(egg.wamr,1,dst);
}

/* Functions requiring measurement that I need to figure out how to do.
 */

static void glDrawElements_wasm(wasm_exec_env_t ee,int mode,int count,int type,int addr) {
  int indexsize=0;
  switch (type) {
    case GL_UNSIGNED_BYTE: indexsize=1; break;
    case GL_UNSIGNED_SHORT: indexsize=2; break;
    case GL_UNSIGNED_INT: indexsize=4; break;
  }
  void *p=wamr_validate_pointer(egg.wamr,1,addr,indexsize*count);
  if (!p) return;
  glDrawElements(mode,count,type,p);
}

static void glGetBooleanv_wasm(wasm_exec_env_t ee,int pname,int addr) {
  int tuplesize=egg_gl_get_tuplesize(pname);
  if (tuplesize<1) return;
  void *p=wamr_validate_pointer(egg.wamr,1,addr,tuplesize*sizeof(int));
  if (!p) return;
  glGetBooleanv(pname,p);
}

static void glGetBufferParameteriv_wasm(wasm_exec_env_t ee,int target,int pname,int addr) {
  int tuplesize=egg_gl_get_tuplesize(pname);
  if (tuplesize<1) return;
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(int)*tuplesize);
  if (!p) return;
  glGetBufferParameteriv(target,pname,p);
}

static void glGetFloatv_wasm(wasm_exec_env_t ee,int pname,int addr) {
  int tuplesize=egg_gl_get_tuplesize(pname);
  if (tuplesize<1) return;
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(float)*tuplesize);
  if (!p) return;
  glGetFloatv(pname,p);
}

static void glGetFramebufferAttachmentParameteriv_wasm(wasm_exec_env_t ee,int target,int attachment,int pname,int addr) {
  int tuplesize=egg_gl_get_tuplesize(pname);
  if (tuplesize<1) return;
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(int)*tuplesize);
  if (!p) return;
  glGetFramebufferAttachmentParameteriv(target,attachment,pname,p);
}

static void glGetIntegerv_wasm(wasm_exec_env_t ee,int pname,int addr) {
  int tuplesize=egg_gl_get_tuplesize(pname);
  if (tuplesize<1) return;
  void *p=wamr_validate_pointer(egg.wamr,1,addr,tuplesize*sizeof(int));
  if (p) return;
  glGetIntegerv(pname,p);
}

static void glGetProgramiv_wasm(wasm_exec_env_t ee,int program,int pname,int addr) {
  int c=0;
  switch (pname) {
    case GL_DELETE_STATUS:
    case GL_LINK_STATUS:
    case GL_VALIDATE_STATUS:
    case GL_INFO_LOG_LENGTH:
    case GL_ATTACHED_SHADERS:
    case GL_ACTIVE_ATTRIBUTES:
    case GL_ACTIVE_ATTRIBUTE_MAX_LENGTH:
    case GL_ACTIVE_UNIFORMS:
    case GL_ACTIVE_UNIFORM_MAX_LENGTH:
      c=1;
      break;
  }
  if (c<1) return;
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(int)*c);
  if (!p) return;
  glGetProgramiv(program,pname,p);
}

static void glGetRenderbufferParameteriv_wasm(wasm_exec_env_t ee,int target,int pname,int addr) {
  int tuplesize=egg_gl_get_tuplesize(pname);
  if (tuplesize<1) return;
  void *p=wamr_validate_pointer(egg.wamr,1,addr,tuplesize*sizeof(int));
  if (!p) return;
  glGetRenderbufferParameteriv(target,pname,p);
}

static void glGetShaderiv_wasm(wasm_exec_env_t ee,int shader,int pname,int addr) {
  int c=0;
  switch (pname) {
    case GL_SHADER_TYPE:
    case GL_DELETE_STATUS:
    case GL_COMPILE_STATUS:
    case GL_INFO_LOG_LENGTH:
    case GL_SHADER_SOURCE_LENGTH:
      c=1;
      break;
  }
  if (c<1) return;
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(int)*c);
  if (!p) return;
  glGetShaderiv(shader,pname,p);
}

static void glGetTexParameterfv_wasm(wasm_exec_env_t ee,int target,int pname,int addr) {
  int tuplesize=egg_gl_get_tuplesize(pname);
  if (tuplesize<1) return;
  void *p=wamr_validate_pointer(egg.wamr,1,addr,tuplesize*sizeof(float));
  if (p) return;
  glGetTexParameterfv(target,pname,p);
}

static void glGetTexParameteriv_wasm(wasm_exec_env_t ee,int target,int pname,int addr) {
  int tuplesize=egg_gl_get_tuplesize(pname);
  if (tuplesize<1) return;
  void *p=wamr_validate_pointer(egg.wamr,1,addr,tuplesize*sizeof(int));
  if (p) return;
  glGetTexParameteriv(target,pname,p);
}

// For uniforms and vertex attributes, assume 16 elements for validation purposes.
#define GET_UNKNOWN_ATTRIBUTE_SIZE 16

static void glGetUniformfv_wasm(wasm_exec_env_t ee,int program,int location,int addr) {
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(float)*GET_UNKNOWN_ATTRIBUTE_SIZE);
  if (!p) return;
  glGetUniformfv(program,location,p);
}

static void glGetUniformiv_wasm(wasm_exec_env_t ee,int program,int location,int addr) {
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(int)*GET_UNKNOWN_ATTRIBUTE_SIZE);
  if (!p) return;
  glGetUniformiv(program,location,p);
}

static void glGetVertexAttribfv_wasm(wasm_exec_env_t ee,int index,int pname,int addr) {
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(float)*GET_UNKNOWN_ATTRIBUTE_SIZE);
  if (!p) return;
  glGetVertexAttribfv(index,pname,p);
}

static void glGetVertexAttribiv_wasm(wasm_exec_env_t ee,int index,int pname,int addr) {
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(int)*GET_UNKNOWN_ATTRIBUTE_SIZE);
  if (!p) return;
  glGetVertexAttribiv(index,pname,p);
}

static void glGetVertexAttribPointerv_wasm(wasm_exec_env_t ee,int index,int pname,int addr) {
  //TODO This is supposed to return pointers to... something? It can't work without intervention from us, and I'm not clear on what it means.
}

static void glReadPixels_wasm(wasm_exec_env_t ee,int x,int y,int width,int height,int format,int type,int addr) {
  int len=egg_gl_measure_image(format,type,width,height);
  if (len<1) return;
  void *p=wamr_validate_pointer(egg.wamr,1,addr,len);
  if (!p) return;
  glReadPixels(x,y,width,height,format,type,p);
  //TODO Can UNPACK parameters be abused to cause this to write OOB?
}

static void glTexImage2D_wasm(wasm_exec_env_t ee,int target,int level,int internalformat,int width,int height,int border,int format,int type,int addr) {
  int len=egg_gl_measure_image(format,type,width,height);
  if (len<1) return;
  void *p=0;
  if (addr) {
    if (!(p=wamr_validate_pointer(egg.wamr,1,addr,len))) return;
  }
  glTexImage2D(target,level,internalformat,width,height,border,format,type,p);
}

static void glTexParameterfv_wasm(wasm_exec_env_t ee,int target,int pname,int addr) {
  int tuplesize=egg_gl_get_tuplesize(pname);
  if (tuplesize<1) return;
  void *p=wamr_validate_pointer(egg.wamr,1,addr,tuplesize*sizeof(float));
  if (p) return;
  glTexParameterfv(target,pname,p);
}

static void glTexParameteriv_wasm(wasm_exec_env_t ee,int target,int pname,int addr) {
  int tuplesize=egg_gl_get_tuplesize(pname);
  if (tuplesize<1) return;
  void *p=wamr_validate_pointer(egg.wamr,1,addr,tuplesize*sizeof(int));
  if (p) return;
  glGetTexParameteriv(target,pname,p);
}

static void glTexSubImage2D_wasm(wasm_exec_env_t ee,int target,int level,int xoffset,int yoffset,int width,int height,int format,int type,int addr) {
  int len=egg_gl_measure_image(format,type,width,height);
  if (len<1) return;
  void *p=wamr_validate_pointer(egg.wamr,1,addr,len);
  if (!p) return;
  glTexSubImage2D(target,level,xoffset,yoffset,width,height,format,type,p);
}

/* Functions requiring simple buffer validation.
 */

static void glBufferData_wasm(wasm_exec_env_t ee,int target,int size,int data_addr,int usage) {
  void *data=wamr_validate_pointer(egg.wamr,1,data_addr,size);
  if (!data) return;
  glBufferData(target,size,data,usage);
}

static void glBufferSubData_wasm(wasm_exec_env_t ee,int target,int offset,int size,int data_addr) {
  void *data=wamr_validate_pointer(egg.wamr,1,data_addr,size);
  if (!data) return;
  glBufferSubData(target,offset,size,data);
}

static void glCompressedTexImage2D_wasm(wasm_exec_env_t ee,int target,int level,int internalformat,int width,int height,int border,int imageSize,int data_addr) {
  void *data=wamr_validate_pointer(egg.wamr,1,data_addr,imageSize);
  if (!data) return;
  glCompressedTexImage2D(target,level,internalformat,width,height,border,imageSize,data);
}

static void glCompressedTexSubImage2D_wasm(wasm_exec_env_t ee,int target,int level,int xoffset,int yoffset,int width,int height,int format,int imageSize,int data_addr) {
  void *data=wamr_validate_pointer(egg.wamr,1,data_addr,imageSize);
  if (!data) return;
  glCompressedTexSubImage2D(target,level,xoffset,yoffset,width,height,format,imageSize,data);
}

static void glDeleteBuffers_wasm(wasm_exec_env_t ee,int n,int addr) {
  if (n<1) return;
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(GLuint)*n);
  if (!p) return;
  glDeleteBuffers(n,p);
}

static void glDeleteFramebuffers_wasm(wasm_exec_env_t ee,int n,int addr) {
  if (n<1) return;
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(GLuint)*n);
  if (!p) return;
  glDeleteFramebuffers(n,p);
}

static void glDeleteRenderbuffers_wasm(wasm_exec_env_t ee,int n,int addr) {
  if (n<1) return;
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(GLuint)*n);
  if (!p) return;
  glDeleteRenderbuffers(n,p);
}

static void glDeleteTextures_wasm(wasm_exec_env_t ee,int n,int addr) {
  if (n<1) return;
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(GLuint)*n);
  if (!p) return;
  glDeleteTextures(n,p);
}

static void glGenBuffers_wasm(wasm_exec_env_t ee,int n,int addr) {
  if (n<1) return;
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(GLuint)*n);
  if (!p) return;
  glGenBuffers(n,p);
}

static void glGenFramebuffers_wasm(wasm_exec_env_t ee,int n,int addr) {
  if (n<1) return;
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(GLuint)*n);
  if (!p) return;
  glGenFramebuffers(n,p);
}

static void glGenRenderbuffers_wasm(wasm_exec_env_t ee,int n,int addr) {
  if (n<1) return;
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(GLuint)*n);
  if (!p) return;
  glGenRenderbuffers(n,p);
}

static void glGenTextures_wasm(wasm_exec_env_t ee,int n,int addr) {
  if (n<1) return;
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(GLuint)*n);
  if (!p) return;
  glGenTextures(n,p);
}

static void glGetActiveAttrib_wasm(wasm_exec_env_t ee,int program,int index,int bufSize,int length_addr,int size_addr,int type_addr,int name_addr) {
  int *lengthp=wamr_validate_pointer(egg.wamr,1,length_addr,sizeof(int));
  int *sizep=wamr_validate_pointer(egg.wamr,1,size_addr,sizeof(int));
  int *typep=wamr_validate_pointer(egg.wamr,1,type_addr,sizeof(int));
  char *namep=wamr_validate_pointer(egg.wamr,1,name_addr,bufSize);
  if (!lengthp||!sizep||!typep) return;
  if (!namep) bufSize=0;
  glGetActiveAttrib(program,index,bufSize,lengthp,sizep,typep,namep);
}

static void glGetActiveUniform_wasm(wasm_exec_env_t ee,int program,int index,int bufSize,int length_addr,int size_addr,int type_addr,int name_addr) {
  int *lengthp=wamr_validate_pointer(egg.wamr,1,length_addr,sizeof(int));
  int *sizep=wamr_validate_pointer(egg.wamr,1,size_addr,sizeof(int));
  int *typep=wamr_validate_pointer(egg.wamr,1,type_addr,sizeof(int));
  char *namep=wamr_validate_pointer(egg.wamr,1,name_addr,bufSize);
  if (!lengthp||!sizep||!typep) return;
  if (!namep) bufSize=0;
  glGetActiveUniform(program,index,bufSize,lengthp,sizep,typep,namep);
}

static void glGetAttachedShaders_wasm(wasm_exec_env_t ee,int program,int maxCount,int count_addr,int shaders_addr) {
  int *countp=wamr_validate_pointer(egg.wamr,1,count_addr,sizeof(int));
  int *shadersp=wamr_validate_pointer(egg.wamr,1,shaders_addr,maxCount*sizeof(int));
  if (!shadersp) maxCount=0;
  glGetAttachedShaders(program,maxCount,countp,shadersp);
}

static void glGetProgramInfoLog_wasm(wasm_exec_env_t ee,int program,int bufSize,int length_addr,int infoLog_addr) {
  if (bufSize<1) return;
  int *lengthp=wamr_validate_pointer(egg.wamr,1,length_addr,sizeof(int));
  char *logp=wamr_validate_pointer(egg.wamr,1,infoLog_addr,bufSize);
  if (!lengthp||!logp) return;
  glGetProgramInfoLog(program,bufSize,lengthp,logp);
}

static void glGetShaderInfoLog_wasm(wasm_exec_env_t ee,int shader,int bufSize,int length_addr,int infoLog_addr) {
  if (bufSize<1) return;
  int *lengthp=wamr_validate_pointer(egg.wamr,1,length_addr,sizeof(int));
  char *logp=wamr_validate_pointer(egg.wamr,1,infoLog_addr,bufSize);
  if (!lengthp||!logp) return;
  glGetShaderInfoLog(shader,bufSize,lengthp,logp);
}

static void glGetShaderPrecisionFormat_wasm(wasm_exec_env_t ee,int shadertype,int precisiontype,int range_addr,int precision_addr) {
  int *rangep=wamr_validate_pointer(egg.wamr,1,range_addr,sizeof(int)*2);
  int *precisionp=wamr_validate_pointer(egg.wamr,1,precision_addr,sizeof(int));
  if (!rangep||!precisionp) return;
  glGetShaderPrecisionFormat(shadertype,precisiontype,rangep,precisionp);
}

static void glGetShaderSource_wasm(wasm_exec_env_t ee,int shader,int bufSize,int length_addr,int source_addr) {
  int *lengthp=wamr_validate_pointer(egg.wamr,1,length_addr,sizeof(int));
  char *sourcep=wamr_validate_pointer(egg.wamr,1,source_addr,bufSize);
  if (!lengthp||!sourcep) return;
  glGetShaderSource(shader,bufSize,lengthp,sourcep);
}

static void glShaderBinary_wasm(wasm_exec_env_t ee,int count,int shaders_addr,int binaryFormat,const void *binary,int length) {
  // (binary,length) is validated by wamr. We're on the hook to validate (shaders_addr).
  int *shadersp=wamr_validate_pointer(egg.wamr,1,shaders_addr,sizeof(int)*count);
  if (!shadersp) return;
  glShaderBinary(count,shadersp,binaryFormat,binary,length);
}
  
static void glShaderSource_wasm(wasm_exec_env_t ee,int shader,int count,int strings_addr,int lens_addr) {
  if ((count<1)||(count>1024)) return; // Arbitrary sanity limit.
  int *stringsp=wamr_validate_pointer(egg.wamr,1,strings_addr,sizeof(int)*count);
  int *lensp=wamr_validate_pointer(egg.wamr,1,lens_addr,sizeof(int)*count);
  if (!stringsp||!lensp) return;
  GLchar **srcv=malloc(sizeof(void*)*count);
  if (!srcv) return;
  int i=0; for (;i<count;i++) {
    if (!(srcv[i]=wamr_validate_pointer(egg.wamr,1,stringsp[i],lensp[i]))) {
      free(srcv);
      return;
    }
  }
  glShaderSource(shader,count,(void*)srcv,lensp);
  free(srcv);
}

static void glUniform1fv_wasm(wasm_exec_env_t ee,int location,int count,int addr) {
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(float)*1*count);
  if (!p) return;
  glUniform1fv(location,count,p);
}

static void glUniform1iv_wasm(wasm_exec_env_t ee,int location,int count,int addr) {
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(int)*1*count);
  if (!p) return;
  glUniform1iv(location,count,p);
}

static void glUniform2fv_wasm(wasm_exec_env_t ee,int location,int count,int addr) {
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(float)*2*count);
  if (!p) return;
  glUniform2fv(location,count,p);
}

static void glUniform2iv_wasm(wasm_exec_env_t ee,int location,int count,int addr) {
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(int)*2*count);
  if (!p) return;
  glUniform2iv(location,count,p);
}

static void glUniform3fv_wasm(wasm_exec_env_t ee,int location,int count,int addr) {
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(float)*3*count);
  if (!p) return;
  glUniform3fv(location,count,p);
}

static void glUniform3iv_wasm(wasm_exec_env_t ee,int location,int count,int addr) {
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(int)*3*count);
  if (!p) return;
  glUniform3iv(location,count,p);
}

static void glUniform4fv_wasm(wasm_exec_env_t ee,int location,int count,int addr) {
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(float)*4*count);
  if (!p) return;
  glUniform4fv(location,count,p);
}

static void glUniform4iv_wasm(wasm_exec_env_t ee,int location,int count,int addr) {
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(int)*4*count);
  if (!p) return;
  glUniform4iv(location,count,p);
}

static void glUniformMatrix2fv_wasm(wasm_exec_env_t ee,int location,int count,int transpose,int addr) {
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(float)*2*2*count);
  if (!p) return;
  glUniformMatrix2fv(location,count,transpose,p);
}

static void glUniformMatrix3fv_wasm(wasm_exec_env_t ee,int location,int count,int transpose,int addr) {
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(float)*3*3*count);
  if (!p) return;
  glUniformMatrix3fv(location,count,transpose,p);
}

static void glUniformMatrix4fv_wasm(wasm_exec_env_t ee,int location,int count,int transpose,int addr) {
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(float)*4*4*count);
  if (!p) return;
  glUniformMatrix4fv(location,count,transpose,p);
}

/* We don't know how many vertices to expect at a pointer, because that actually isn't known until the next glDrawArrays().
 * So we have to take the count on faith.
 * TODO This introduces the possibility of games reading outside their heap and dumping that data into GL. Is there any risk in that?
 */
#define VTXATTR_VERTEX_COUNT 1

static void glVertexAttrib1fv_wasm(wasm_exec_env_t ee,int index,int addr) {
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(float)*1*VTXATTR_VERTEX_COUNT);
  if (!p) return;
  glVertexAttrib1fv(index,p);
}

static void glVertexAttrib2fv_wasm(wasm_exec_env_t ee,int index,int addr) {
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(float)*2*VTXATTR_VERTEX_COUNT);
  if (!p) return;
  glVertexAttrib2fv(index,p);
}

static void glVertexAttrib3fv_wasm(wasm_exec_env_t ee,int index,int addr) {
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(float)*3*VTXATTR_VERTEX_COUNT);
  if (!p) return;
  glVertexAttrib3fv(index,p);
}

static void glVertexAttrib4fv_wasm(wasm_exec_env_t ee,int index,int addr) {
  void *p=wamr_validate_pointer(egg.wamr,1,addr,sizeof(float)*4*VTXATTR_VERTEX_COUNT);
  if (!p) return;
  glVertexAttrib4fv(index,p);
}

static void glVertexAttribPointer_wasm(wasm_exec_env_t ee,int index,int size,int type,int normalized,int stride,int addr) {
  if (egg_gles2_buffer_bound) {
    glVertexAttribPointer(index,size,type,normalized,stride,(void*)(uintptr_t)addr);
  } else {
    void *p=wamr_validate_pointer(egg.wamr,1,addr,stride*VTXATTR_VERTEX_COUNT);
    if (!p) return;
    glVertexAttribPointer(index,size,type,normalized,stride,p);
  }
}

/* All functions below here are dead simple, straight pass-thru to GLES.
 */
 
static void glActiveTexture_wasm(wasm_exec_env_t ee,int texture) {
  glActiveTexture(texture);
}

static void glAttachShader_wasm(wasm_exec_env_t ee,int program,int shader) {
  glAttachShader(program,shader);
}

static void glBindAttribLocation_wasm(wasm_exec_env_t ee,int program,int index,const GLchar *name) {
  glBindAttribLocation(program,index,name);
}

static void glBindBuffer_wasm(wasm_exec_env_t ee,int target,int buffer) {
  glBindBuffer(target,buffer);
  if (buffer) egg_gles2_buffer_bound=1;
  else egg_gles2_buffer_bound=0;
}

static void glBindFramebuffer_wasm(wasm_exec_env_t ee,int target,int framebuffer) {
  glBindFramebuffer(target,framebuffer);
}

static void glBindRenderbuffer_wasm(wasm_exec_env_t ee,int target,int renderbuffer) {
  glBindRenderbuffer(target,renderbuffer);
}

static void glBindTexture_wasm(wasm_exec_env_t ee,int target,int texture) {
  glBindTexture(target,texture);
}

static void glBlendColor_wasm(wasm_exec_env_t ee,float red,float green,float blue,float alpha) {
  glBlendColor(red,green,blue,alpha);
}

static void glBlendEquation_wasm(wasm_exec_env_t ee,int mode) {
  glBlendEquation(mode);
}

static void glBlendEquationSeparate_wasm(wasm_exec_env_t ee,int modeRGB,int modeAlpha) {
  glBlendEquationSeparate(modeRGB,modeAlpha);
}

static void glBlendFunc_wasm(wasm_exec_env_t ee,int sfactor,int dfactor) {
  glBlendFunc(sfactor,dfactor);
}

static void glBlendFuncSeparate_wasm(wasm_exec_env_t ee,int sfactorRGB,int dfactorRGB,int sfactorAlpha,int dfactorAlpha) {
  glBlendFuncSeparate(sfactorRGB,dfactorRGB,sfactorAlpha,dfactorAlpha);
}

static int glCheckFramebufferStatus_wasm(wasm_exec_env_t ee,int target) {
  return glCheckFramebufferStatus(target);
}

static void glClear_wasm(wasm_exec_env_t ee,int mask) {
  glClear(mask);
}

static void glClearColor_wasm(wasm_exec_env_t ee,float red,float green,float blue,float alpha) {
  glClearColor(red,green,blue,alpha);
}

static void glClearDepthf_wasm(wasm_exec_env_t ee,float d) {
  glClearDepthf(d);
}

static void glClearStencil_wasm(wasm_exec_env_t ee,int s) {
  glClearStencil(s);
}

static void glColorMask_wasm(wasm_exec_env_t ee,int red,int green,int blue,int alpha) {
  glColorMask(red,green,blue,alpha);
}

static void glCompileShader_wasm(wasm_exec_env_t ee,int shader) {
  glCompileShader(shader);
}

static void glCopyTexImage2D_wasm(wasm_exec_env_t ee,int target,int level,int internalformat,int x,int y,int width,int height,int border) {
  glCopyTexImage2D(target,level,internalformat,x,y,width,height,border);
}

static void glCopyTexSubImage2D_wasm(wasm_exec_env_t ee,int target,int level,int xoffset,int yoffset,int x,int y,int width,int height) {
  glCopyTexSubImage2D(target,level,xoffset,yoffset,x,y,width,height);
}

static int glCreateProgram_wasm(wasm_exec_env_t ee) {
  return glCreateProgram();
}

static int glCreateShader_wasm(wasm_exec_env_t ee,int type) {
  return glCreateShader(type);
}

static void glCullFace_wasm(wasm_exec_env_t ee,int mode) {
  glCullFace(mode);
}

static void glDeleteProgram_wasm(wasm_exec_env_t ee,int program) {
  glDeleteProgram(program);
}

static void glDeleteShader_wasm(wasm_exec_env_t ee,int shader) {
  glDeleteShader(shader);
}

static void glDepthFunc_wasm(wasm_exec_env_t ee,int func) {
  glDepthFunc(func);
}

static void glDepthMask_wasm(wasm_exec_env_t ee,int flag) {
  glDepthMask(flag);
}

static void glDepthRangef_wasm(wasm_exec_env_t ee,float n,float f) {
  glDepthRangef(n,f);
}

static void glDetachShader_wasm(wasm_exec_env_t ee,int program,int shader) {
  glDetachShader(program,shader);
}

static void glDisable_wasm(wasm_exec_env_t ee,int cap) {
  glDisable(cap);
}

static void glDisableVertexAttribArray_wasm(wasm_exec_env_t ee,int index) {
  glDisableVertexAttribArray(index);
}

static void glDrawArrays_wasm(wasm_exec_env_t ee,int mode,int first,int count) {
  glDrawArrays(mode,first,count);
}

static void glEnable_wasm(wasm_exec_env_t ee,int cap) {
  glEnable(cap);
}

static void glEnableVertexAttribArray_wasm(wasm_exec_env_t ee,int index) {
  glEnableVertexAttribArray(index);
}

static void glFinish_wasm(wasm_exec_env_t ee) {
  glFinish();
}

static void glFlush_wasm(wasm_exec_env_t ee) {
  glFlush();
}

static void glFramebufferRenderbuffer_wasm(wasm_exec_env_t ee,int target,int attachment,int renderbuffertarget,int renderbuffer) {
  glFramebufferRenderbuffer(target,attachment,renderbuffertarget,renderbuffer);
}

static void glFramebufferTexture2D_wasm(wasm_exec_env_t ee,int target,int attachment,int textarget,int texture,int level) {
  glFramebufferTexture2D(target,attachment,textarget,texture,level);
}

static void glFrontFace_wasm(wasm_exec_env_t ee,int mode) {
  glFrontFace(mode);
}

static void glGenerateMipmap_wasm(wasm_exec_env_t ee,int target) {
  glGenerateMipmap(target);
}

static int glGetAttribLocation_wasm(wasm_exec_env_t ee,int program,const GLchar *name) {
  return glGetAttribLocation(program,name);
}

static int glGetError_wasm(wasm_exec_env_t ee) {
  return glGetError();
}

static int glGetUniformLocation_wasm(wasm_exec_env_t ee,int program,const GLchar *name) {
  return glGetUniformLocation(program,name);
}

static void glHint_wasm(wasm_exec_env_t ee,int target,int mode) {
  glHint(target,mode);
}

static int glIsBuffer_wasm(wasm_exec_env_t ee,int buffer) {
  return glIsBuffer(buffer);
}

static int glIsEnabled_wasm(wasm_exec_env_t ee,int cap) {
  return glIsEnabled(cap);
}

static int glIsFramebuffer_wasm(wasm_exec_env_t ee,int framebuffer) {
  return glIsFramebuffer(framebuffer);
}

static int glIsProgram_wasm(wasm_exec_env_t ee,int program) {
  return glIsProgram(program);
}

static int glIsRenderbuffer_wasm(wasm_exec_env_t ee,int renderbuffer) {
  return glIsRenderbuffer(renderbuffer);
}

static int glIsShader_wasm(wasm_exec_env_t ee,int shader) {
  return glIsShader(shader);
}

static int glIsTexture_wasm(wasm_exec_env_t ee,int texture) {
  return glIsTexture(texture);
}

static void glLineWidth_wasm(wasm_exec_env_t ee,float width) {
  glLineWidth(width);
}

static void glLinkProgram_wasm(wasm_exec_env_t ee,int program) {
  glLinkProgram(program);
}

static void glPixelStorei_wasm(wasm_exec_env_t ee,int pname,int param) {
  glPixelStorei(pname,param);
}

static void glPolygonOffset_wasm(wasm_exec_env_t ee,float factor,float units) {
  glPolygonOffset(factor,units);
}

static void glReleaseShaderCompiler_wasm(wasm_exec_env_t ee) {
  glReleaseShaderCompiler();
}

static void glRenderbufferStorage_wasm(wasm_exec_env_t ee,int target,int internalformat,int width,int height) {
  glRenderbufferStorage(target,internalformat,width,height);
}

static void glSampleCoverage_wasm(wasm_exec_env_t ee,float value,int invert) {
  glSampleCoverage(value,invert);
}

static void glScissor_wasm(wasm_exec_env_t ee,int x,int y,int width,int height) {
  glScissor(x,y,width,height);
}

static void glStencilFunc_wasm(wasm_exec_env_t ee,int func,int ref,int mask) {
  glStencilFunc(func,ref,mask);
}

static void glStencilFuncSeparate_wasm(wasm_exec_env_t ee,int face,int func,int ref,int mask) {
  glStencilFuncSeparate(face,func,ref,mask);
}

static void glStencilMask_wasm(wasm_exec_env_t ee,int mask) {
  glStencilMask(mask);
}

static void glStencilMaskSeparate_wasm(wasm_exec_env_t ee,int face,int mask) {
  glStencilMaskSeparate(face,mask);
}

static void glStencilOp_wasm(wasm_exec_env_t ee,int fail,int zfail,int zpass) {
  glStencilOp(fail,zfail,zpass);
}

static void glStencilOpSeparate_wasm(wasm_exec_env_t ee,int face,int sfail,int dpfail,int dppass) {
  glStencilOpSeparate(face,sfail,dpfail,dppass);
}

static void glTexParameterf_wasm(wasm_exec_env_t ee,int target,int pname,float param) {
  glTexParameterf(target,pname,param);
}

static void glTexParameteri_wasm(wasm_exec_env_t ee,int target,int pname,int param) {
  glTexParameteri(target,pname,param);
}

static void glUniform1f_wasm(wasm_exec_env_t ee,int location,float v0) {
  glUniform1f(location,v0);
}

static void glUniform1i_wasm(wasm_exec_env_t ee,int location,int v0) {
  glUniform1i(location,v0);
}

static void glUniform2f_wasm(wasm_exec_env_t ee,int location,float v0,float v1) {
  glUniform2f(location,v0,v1);
}

static void glUniform2i_wasm(wasm_exec_env_t ee,int location,int v0,int v1) {
  glUniform2i(location,v0,v1);
}

static void glUniform3f_wasm(wasm_exec_env_t ee,int location,float v0,float v1,float v2) {
  glUniform3f(location,v0,v1,v2);
}

static void glUniform3i_wasm(wasm_exec_env_t ee,int location,int v0,int v1,int v2) {
  glUniform3i(location,v0,v1,v2);
}

static void glUniform4f_wasm(wasm_exec_env_t ee,int location,float v0,float v1,float v2,float v3) {
  glUniform4f(location,v0,v1,v2,v3);
}

static void glUniform4i_wasm(wasm_exec_env_t ee,int location,int v0,int v1,int v2,int v3) {
  glUniform4i(location,v0,v1,v2,v3);
}

static void glUseProgram_wasm(wasm_exec_env_t ee,int program) {
  glUseProgram(program);
}

static void glValidateProgram_wasm(wasm_exec_env_t ee,int program) {
  glValidateProgram(program);
}

static void glVertexAttrib1f_wasm(wasm_exec_env_t ee,int index,float x) {
  glVertexAttrib1f(index,x);
}

static void glVertexAttrib2f_wasm(wasm_exec_env_t ee,int index,float x,float y) {
  glVertexAttrib2f(index,x,y);
}

static void glVertexAttrib3f_wasm(wasm_exec_env_t ee,int index,float x,float y,float z) {
  glVertexAttrib3f(index,x,y,z);
}

static void glVertexAttrib4f_wasm(wasm_exec_env_t ee,int index,float x,float y,float z,float w) {
  glVertexAttrib4f(index,x,y,z,w);
}

static void glViewport_wasm(wasm_exec_env_t ee,int x,int y,int width,int height) {
  glViewport(x,y,width,height);
}
