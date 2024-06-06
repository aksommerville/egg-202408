/* Webgl.js
 * Adapter that exposes GLES2 to the Wasm client, and implements in a WebGL context.
 */
 
export class Webgl {
  constructor(egg, gl) {
    this.egg = egg;
    this.gl = gl;
    this.o = ["neverZero"]; // GL objects (mixed types), indexed by ID. Can be sparse.
    this.nextId = 1;
    this.glstrp = 0; // Storage for glGetString, set by Render.js.
    this.glstra = 0;
    this.locv = ["neverZero"]; // Uniform locations. TODO We never remove them. Will that be a problem?
  }
  
  generatePublicApi() {
    return {
      // Anything with no pointers or object names converts trivially.
      glActiveTexture: (a) => this.gl.activeTexture(a),
      glBlendColor: (a,b,c,d) => this.gl.blendColor(a,b,c,d),
      glBlendEquation: (a) => this.gl.blendEquation(a),
      glBlendEquationSeparate: (a,b) => this.gl.blendEquationSeparate(a,b),
      glBlendFunc: (a,b) => this.gl.blendFunc(a,b),
      glBlendFuncSeparate: (a,b,c,d) => this.gl.blendFuncSeparate(a,b,c,d),
      glCheckFramebufferStatus: (a) => this.gl.checkFramebufferStatus(a),
      glClear: (a) => this.gl.clear(a),
      glClearColor: (a,b,c,d) => this.gl.clearColor(a,b,c,d),
      glClearDepthf: (a) => this.gl.clearDepth(a),
      glClearStencil: (a) => this.gl.clearStencil(a),
      glColorMask: (a,b,c,d) => this.gl.colorMask(a,b,c,d),
      glCullFace: (a) => this.gl.cullFace(a),
      glDepthFunc: (a) => this.gl.depthFunc(a),
      glDepthMask: (a) => this.gl.depthMask(a),
      glDepthRangef: (a,b) => this.gl.depthRangef(a,b),
      glDisable: (a) => this.gl.disable(a),
      glDisableVertexAttribArray: (a) => this.gl.disableVertexAttribArray(a),
      glDrawArrays: (a,b,c) => this.gl.drawArrays(a,b,c),
      glDrawElements: (a,b,c,d) => this.gl.drawElements(a,b,c,d),
      glEnable: (a) => this.gl.enable(a),
      glEnableVertexAttribArray: (a) => this.gl.enableVertexAttribArray(a),
      glFinish: () => this.gl.finish(),
      glFlush: () => this.gl.flush(),
      glFrontFace: (a) => this.gl.frontFace(a),
      glGenerateMipmap: (a) => this.gl.generateMipmap(a),
      glGetError: () => this.gl.getError(),
      glHint: (a,b) => this.gl.hint(a,b),
      glIsEnabled: (a) => this.gl.isEnabled(a),
      glLineWidth: (a) => this.gl.lineWidth(a),
      glPixelStorei: (a,b) => this.gl.pixelStore(a,b),
      glPolygonOffset: (a,b) => this.gl.polygonOffset(a,b),
      glReleaseShaderCompiler: () => this.gl.releaseShaderCompiler(),
      glRenderbufferStorage: (a,b,c,d) => this.gl.renderbufferStorage(a,b,c,d),
      glSampleCoverage: (a,b) => this.gl.sampleCoverage(a,b),
      glScissor: (a,b,c,d) => this.gl.scissor(a,b,c,d),
      glStencilFunc: (a,b,c) => this.gl.stencilFunc(a,b,c),
      glStencilFuncSeparate: (a,b,c,d) => this.gl.stencilFuncSeparate(a,b,c,d),
      glStencilMask: (a) => this.gl.stencilMask(a),
      glStencilMaskSeparate: (a,b) => this.gl.stencilMaskSeparate(a,b),
      glStencilOp: (a,b,c) => this.gl.stencilOp(a,b,c),
      glStencilOpSeparate: (a,b,c,d) => this.gl.stencilOpSeparate(a,b,c,d),
      glTexParameterf: (a,b,c) => this.gl.texParameterf(a,b,c),
      glTexParameteri: (a,b,c) => this.gl.texParameteri(a,b,c),
      glUniform1f: (a,b) => this.gl.uniform1f(this.locv[a],b),
      glUniform1i: (a,b) => this.gl.uniform1i(this.locv[a],b),
      glUniform2f: (a,b,c) => this.gl.uniform2f(this.locv[a],b,c),
      glUniform2i: (a,b,c) => this.gl.uniform2i(this.locv[a],b,c),
      glUniform3f: (a,b,c,d) => this.gl.uniform3f(this.locv[a],b,c,d),
      glUniform3i: (a,b,c,d) => this.gl.uniform3i(this.locv[a],b,c,d),
      glUniform4f: (a,b,c,d,e) => this.gl.uniform4f(this.locv[a],b,c,d,e),
      glUniform4i: (a,b,c,d,e) => this.gl.uniform4i(this.locv[a],b,c,d,e),
      glVertexAttrib1f: (a,b) => this.gl.vertexAttrib1f(a,b),
      glVertexAttrib2f: (a,b,c) => this.gl.vertexAttrib2f(a,b,c),
      glVertexAttrib3f: (a,b,c,d) => this.gl.vertexAttrib3f(a,b,c,d),
      glVertexAttrib4f: (a,b,c,d,e) => this.gl.vertexAttrib4f(a,b,c,d,e),
      glViewport: (a,b,c,d) => this.gl.viewport(a,b,c,d),
      // All the rest are a little more interesting:
      glAttachShader: (a,b) => this.glAttachShader(a,b),
      glBindAttribLocation: (a,b,c) => this.glBindAttribLocation(a,b,c),
      glBindBuffer: (a,b) => this.glBindBuffer(a,b),
      glBindFramebuffer: (a,b) => this.glBindFramebuffer(a,b),
      glBindRenderbuffer: (a,b) => this.glBindRenderbuffer(a,b),
      glBindTexture: (a,b) => this.glBindTexture(a,b),
      glBufferData: (a,b,c,d) => this.glBufferData(a,b,c,d),
      glBufferSubData: (a,b,c,d) => this.glBufferSubData(a,b,c,d),
      glCompileShader: (a) => this.glCompileShader(a),
      glCompressedTexImage2D: (a,b,c,d,e,f,g,h) => this.glCompressedTexImage2D(a,b,c,d,e,f,g,h),
      glCompressedTexSubImage2D: (a,b,c,d,e,f,g,h,i) => this.glCompressedTexSubImage2D(a,b,c,d,e,f,g,h,i),
      glCopyTexImage2D: (a,b,c,d,e,f,g,h) => this.glCopyTexImage2D(a,b,c,d,e,f,g,h),
      glCopyTexSubImage2D: (a,b,c,d,e,f,g,h) => this.glCopyTexSubImage2D(a,b,c,d,e,f,g,h),
      glCreateProgram: () => this.glCreateProgram(),
      glCreateShader: (a) => this.glCreateShader(a),
      glDeleteBuffers: (a,b) => this.glDeleteBuffers(a,b),
      glDeleteFramebuffers: (a,b) => this.glDeleteFramebuffers(a,b),
      glDeleteProgram: (a) => this.glDeleteProgram(a),
      glDeleteRenderbuffers: (a,b) => this.glDeleteRenderbuffers(a,b),
      glDeleteShader: (a) => this.glDeleteShader(a),
      glDeleteTextures: (a,b) => this.glDeleteTextures(a,b),
      glDetachShader: (a,b) => this.glDetachShader(a,b),
      glFramebufferRenderbuffer: (a,b,c,d) => this.glFramebufferRenderbuffer(a,b,c,d),
      glFramebufferTexture2D: (a,b,c,d,e) => this.glFramebufferTexture2D(a,b,c,d,e),
      glGenBuffers: (a,b) => this.glGenBuffers(a,b),
      glGenFramebuffers: (a,b) => this.glGenFramebuffers(a,b),
      glGenRenderbuffers: (a,b) => this.glGenRenderbuffers(a,b),
      glGenTextures: (a,b) => this.glGenTextures(a,b),
      glGetActiveAttrib: (a,b,c,d,e,f,g) => this.glGetActiveAttrib(a,b,c,d,e,f,g),
      glGetActiveUniform: (a,b,c,d,e,f,g) => this.glGetActiveUniform(a,b,c,d,e,f,g),
      glGetAttachedShaders: (a,b,c,d) => this.glGetAttachedShaders(a,b,c,d),
      glGetAttribLocation: (a,b) => this.glGetAttribLocation(a,b),
      glGetBooleanv: (a,b) => this.glGetBooleanv(a,b),
      glGetBufferParameteriv: (a,b,c) => this.glGetBufferParameteriv(a,b,c),
      glGetFloatv: (a,b) => this.glGetFloatv(a,b),
      glGetFramebufferAttachmentParameteriv: (a,b,c,d) => this.glGetFramebufferAttachmentParameteriv(a,b,c,d),
      glGetIntegerv: (a,b) => this.glGetIntegerv(a,b),
      glGetProgramiv: (a,b,c) => this.glGetProgramiv(a,b,c),
      glGetProgramInfoLog: (a,b,c,d) => this.glGetProgramInfoLog(a,b,c,d),
      glGetRenderbufferParameteriv: (a,b,c) => this.glGetRenderbufferParameteriv(a,b,c),
      glGetShaderiv: (a,b,c) => this.glGetShaderiv(a,b,c),
      glGetShaderInfoLog: (a,b,c,d) => this.glGetShaderInfoLog(a,b,c,d),
      glGetShaderPrecisionFormat: (a,b,c,d) => this.glGetShaderPrecisionFormat(a,b,c,d),
      glGetShaderSource: (a,b,c,d) => this.glGetShaderSource(a,b,c,d),
      glGetString: (a) => this.glGetString(a),
      glGetTexParameterfv: (a,b,c) => this.glGetTexParameterfv(a,b,c),
      glGetTexParameteriv: (a,b,c) => this.glGetTexParameteriv(a,b,c),
      glGetUniformfv: (a,b,c) => this.glGetUniformfv(a,b,c),
      glGetUniformiv: (a,b,c) => this.glGetUniformiv(a,b,c),
      glGetUniformLocation: (a,b) => this.glGetUniformLocation(a,b),
      glGetVertexAttribfv: (a,b,c) => this.glGetVertexAttribfv(a,b,c),
      glGetVertexAttribiv: (a,b,c) => this.glGetVertexAttribiv(a,b,c),
      glGetVertexAttribPointerv: (a,b,c) => this.glGetVertexAttribPointerv(a,b,c),
      glIsBuffer: (a) => this.glIsBuffer(a),
      glIsFramebuffer: (a) => this.glIsFramebuffer(a),
      glIsProgram: (a) => this.glIsProgram(a),
      glIsRenderbuffer: (a) => this.glIsRenderbuffer(a),
      glIsShader: (a) => this.glIsShader(a),
      glIsTexture: (a) => this.glIsTexture(a),
      glLinkProgram: (a) => this.glLinkProgram(a),
      glReadPixels: (a,b,c,d,e,f,g) => this.glReadPixels(a,b,c,d,e,f,g),
      glShaderBinary: (a,b,c,d,e) => this.glShaderBinary(a,b,c,d,e),
      glShaderSource: (a,b,c,d) => this.glShaderSource(a,b,c,d),
      glTexImage2D: (a,b,c,d,e,f,g,h,i) => this.glTexImage2D(a,b,c,d,e,f,g,h,i),
      glTexParameterfv: (a,b,c) => this.glTexParameterfv(a,b,c),
      glTexParameteriv: (a,b,c) => this.glTexParameteriv(a,b,c),
      glTexSubImage2D: (a,b,c,d,e,f,g,h,i) => this.glTexSubImage2D(a,b,c,d,e,f,g,h,i),
      glUniform1fv: (a,b,c) => this.glUniform1fv(a,b,c),
      glUniform1iv: (a,b,c) => this.glUniform1iv(a,b,c),
      glUniform2fv: (a,b,c) => this.glUniform2fv(a,b,c),
      glUniform2iv: (a,b,c) => this.glUniform2iv(a,b,c),
      glUniform3fv: (a,b,c) => this.glUniform3fv(a,b,c),
      glUniform3iv: (a,b,c) => this.glUniform3iv(a,b,c),
      glUniform4fv: (a,b,c) => this.glUniform4fv(a,b,c),
      glUniform4iv: (a,b,c) => this.glUniform4iv(a,b,c),
      glUniformMatrix2fv: (a,b,c,d) => this.glUniformMatrix2fv(a,b,c,d),
      glUniformMatrix3fv: (a,b,c,d) => this.glUniformMatrix3fv(a,b,c,d),
      glUniformMatrix4fv: (a,b,c,d) => this.glUniformMatrix4fv(a,b,c,d),
      glUseProgram: (a) => this.glUseProgram(a),
      glValidateProgram: (a) => this.glValidateProgram(a),
      glVertexAttrib1fv: (a,b) => this.glVertexAttrib1fv(a,b),
      glVertexAttrib2fv: (a,b) => this.glVertexAttrib2fv(a,b),
      glVertexAttrib3fv: (a,b) => this.glVertexAttrib3fv(a,b),
      glVertexAttrib4fv: (a,b) => this.glVertexAttrib4fv(a,b),
      glVertexAttribPointer: (a,b,c,d,e,f) => this.glVertexAttribPointer(a,b,c,d,e,f),
    };
  }
  
  /* Create and delete objects.
   **********************************************************/
   
  objAlloc() {
    const p = this.o.indexOf(null);
    if (p >= 0) return p;
    return this.nextId++;
  }
  
  objDel(id) {
    if (id < 1) return;
    this.o[id] = null;
  }

  glCreateProgram() {
    const id = this.objAlloc();
    if (!(this.o[id] = this.gl.createProgram())) return 0;
    return id;
  }

  glCreateShader(a) {
    const id = this.objAlloc();
    if (!(this.o[id] = this.gl.createShader(a))) return 0;
    return id;
  }

  glGenBuffers(a,b) {
    let p = b >> 2;
    for (; a-->0; p++) {
      const id = this.objAlloc();
      this.o[id] = this.gl.createBuffer();
      this.egg.exec.mem32[p] = id;
    }
  }

  glGenFramebuffers(a,b) {
    let p = b >> 2;
    for (; a-->0; p++) {
      const id = this.objAlloc();
      this.o[id] = this.gl.createFramebuffer();
      this.egg.exec.mem32[p] = id;
    }
  }

  glGenRenderbuffers(a,b) {
    let p = b >> 2;
    for (; a-->0; p++) {
      const id = this.objAlloc();
      this.o[id] = this.gl.createRenderbuffer();
      this.egg.exec.mem32[p] = id;
    }
  }

  glGenTextures(a,b) {
    let p = b >> 2;
    for (; a-->0; p++) {
      const id = this.objAlloc();
      this.o[id] = this.gl.createTexture();
      this.egg.exec.mem32[p] = id;
    }
  }

  glDeleteBuffers(a,b) {
    let p = b >> 2;
    for (; a-->0; p++) {
      const id = this.egg.exec.mem32[p];
      this.gl.deleteBuffer(this.o[id]);
      this.objDel(id);
    }
  }

  glDeleteFramebuffers(a,b) {
    let p = b >> 2;
    for (; a-->0; p++) {
      const id = this.egg.exec.mem32[p];
      this.gl.deleteFramebuffer(this.o[id]);
      this.objDel(id);
    }
  }

  glDeleteProgram(a) {
    this.gl.deleteProgram(this.o[a]);
    this.objDel(a);
  }

  glDeleteRenderbuffers(a,b) {
    let p = b >> 2;
    for (; a-->0; p++) {
      const id = this.egg.exec.mem32[p];
      this.gl.deleteRenderbuffer(this.o[id]);
      this.objDel(id);
    }
  }

  glDeleteShader(a) {
    this.gl.deleteShader(this.o[a]);
    this.objDel(a);
  }

  glDeleteTextures(a,b) {
    let p = b >> 2;
    for (; a-->0; p++) {
      const id = this.egg.exec.mem32[p];
      this.gl.deleteTexture(this.o[id]);
      this.objDel(id);
    }
  }

  glIsBuffer(a) {
    return this.gl.isBuffer(this.o[a]);
  }

  glIsFramebuffer(a) {
    return this.gl.isFramebuffer(this.o[a]);
  }

  glIsProgram(a) {
    return this.gl.isProgram(this.o[a]);
  }

  glIsRenderbuffer(a) {
    return this.gl.isRenderbuffer(this.o[a]);
  }

  glIsShader(a) {
    return this.gl.isShader(this.o[a]);
  }

  glIsTexture(a) {
    return this.gl.isTexture(this.o[a]);
  }
   
  /* Miscellaneous.
   ***********************************************************/
  
  glAttachShader(pid, sid) {
    this.gl.attachShader(this.o[pid], this.o[sid]);
  }

  glBindAttribLocation(pid, index, namep) {
    const program = this.o[pid];
    const name = this.egg.exec.readCString(namep);
    this.gl.bindAttribLocation(program, index, name);
  }

  glBindBuffer(target, id) {
    const buffer = this.o[id];
    this.gl.bindBuffer(target, buffer);
  }

  glBindFramebuffer(target, id) {
    const fb = this.o[id];
    this.gl.bindFramebuffer(target, fb);
  }

  glBindRenderbuffer(target, id) {
    const rb = this.o[id];
    this.gl.bindRenderbuffer(target, rb);
  }

  glBindTexture(target, id) {
    const texture = this.o[id];
    this.gl.bindTexture(target, texture);
  }

  glBufferData(target, size, datap, usage) {
    if (datap) {
      const data = this.egg.exec.getView(datap, size);
      if (!data) return;
      this.gl.bufferData(target, data, usage);
    } else {
      this.gl.bufferData(target, size, usage);
    }
  }

  glBufferSubData(target, offset, size, datap) {
    const data = this.egg.exec.getView(datap, size);
    if (!data) return;
    this.gl.bufferSubData(target, offset, data);
  }

  glCompileShader(id) {
    const shader = this.o[id];
    this.gl.compileShader(shader);
  }

  glDetachShader(pid, sid) {
    const program = this.o[pid];
    const shader = this.o[sid];
    this.gl.detachShader(program, shader);
  }

  glFramebufferRenderbuffer(target, attachment, rbtarget, rbid) {
    const rb = this.o[rbid];
    this.gl.framebufferRenderbuffer(target, attachment, rbtarget, rb);
  }

  glFramebufferTexture2D(target, attachment, textarget, texid, level) {
    const texture = this.o[texid];
    this.gl.framebufferTexture2D(target, attachment, textarget, texture, level);
  }

  glGetActiveAttrib(pid, index, bufsize, lenp, sizep, typep, namep) {
    const program = this.o[pid];
    const attr = this.gl.getActiveAttrib(program, index);
    if (!attr) return;
    if (sizep) this.egg.exec.mem32[sizep >> 2] = attr.size;
    if (typep) this.egg.exec.mem32[typep >> 2] = attr.type;
    if (namep && (bufsize > 0)) {
      this.egg.exec.safeWrite(namep, bufsize, attr.name);
    }
  }

  glGetActiveUniform(pid, index, bufsize, lenp, sizep, typep, namep) {
    const program = this.o[pid];
    const attr = this.gl.getActiveUniform(program, index);
    if (!attr) return;
    if (sizep) this.egg.exec.mem32[sizep >> 2] = attr.size;
    if (typep) this.egg.exec.mem32[typep >> 2] = attr.type;
    if (namep && (bufsize > 0)) {
      this.egg.exec.safeWrite(namep, bufsize, attr.name);
    }
  }

  glGetAttachedShaders(pid, max, countp, dstp) {
    const program = this.o[pid];
    const shaders = this.gl.getAttachedShaders(program) || [];
    if (countp) this.egg.exec.mem32[countp >> 2] = shaders.length;
    const cpc = Math.min(shaders.length, max);
    for (let i=0; i<cpc; i++, dstp+=4) {
      let id = this.o.indexOf(shaders[i]);
      if (id < 0) id = 0;
      this.egg.exec.mem32[dstp >> 2] = id;
    }
  }

  glGetAttribLocation(pid, namep) {
    const program = this.o[pid];
    const name = this.egg.exec.readCString(namep);
    return this.gl.getAttribLocation(program, name);
  }

  glGetProgramInfoLog(pid, bufsize, lenp, dstp) {
    const program = this.o[pid];
    const src = this.gl.getProgramInfoLog(program) || "";
    this.egg.exec.mem32[lenp >> 2] = this.egg.exec.safeWrite(dstp, bufsize, src);
  }

  glGetShaderInfoLog(sid, bufsize, lenp, dstp) {
    const shader = this.o[sid];
    const src = this.gl.getShaderInfoLog(shader) || "";
    this.egg.exec.mem32[lenp >> 2] = this.egg.exec.safeWrite(dstp, bufsize, src);
  }

  glGetShaderPrecisionFormat(stype, ptype, rangep, precp) {
    const rsp = this.gl.getShaderPrecisionFormat(stype, ptype);
    if (!rsp) return;
    rangep >>= 2;
    this.egg.exec.mem32[rangep++] = rsp.rangeMin;
    this.egg.exec.mem32[rangep] = rsp.rangeMax;
    this.egg.exec.mem32[precp >> 2] = rsp.precision;
  }

  glGetShaderSource(sid, bufsize, lenp, dstp) {
    const shader = this.o[sid];
    const src = this.gl.getShaderSource(shader) || "";
    this.egg.exec.mem32[lenp >> 2] = this.egg.exec.safeWrite(dstp, bufsize, src);
  }

  glGetUniformLocation(pid, namep) {
    const program = this.o[pid];
    const name = this.egg.exec.readCString(namep);
    const loc = this.gl.getUniformLocation(program, name);
    if (!loc) return 0;
    const id = this.locv.length;
    this.locv.push(loc);
    return id;
  }

  glLinkProgram(pid) {
    const program = this.o[pid];
    this.gl.linkProgram(program);
  }

  glShaderBinary(count, dstp, bfmt, src, srcc) {
    // GL_APICALL void GL_APIENTRY glShaderBinary (GLsizei count, const GLuint *shaders, GLenum binaryFormat, const void *binary, GLsizei length);
    // Evidently there is no such thing in WebGL.
  }

  glShaderSource(sid, count, stringsp, lensp) {
    const shader = this.o[sid];
    let glsl = "";
    stringsp >>= 2;
    lensp >>= 2;
    for (let i=0; i<count; i++, stringsp++, lensp++) {
      const srcp = this.egg.exec.mem32[stringsp];
      const srcc = this.egg.exec.mem32[lensp];
      const sub = this.egg.exec.readLimitedString(srcp, srcc);
      glsl += sub;
    }
    this.gl.shaderSource(shader, glsl);
  }

  glUseProgram(a) {
    this.gl.useProgram(this.o[a]);
  }

  glValidateProgram(a) {
    this.gl.validateProgram(this.o[a]);
  }
  
  /* Generic parameters.
   ******************************************************************************/
   
  paramAsInts(src) {
    if (!src) return [0];
    if (typeof(src) === "number") return [~~src];
    const p = this.o.indexOf(src);
    if (p > 0) return [p];
    if (src.length) return src.map(v => ~~v);
    return [0];
  }
  
  paramAsFloats(src) {
    if (!src) return [0];
    if (typeof(src) === "number") return [src];
    if (src.length) return src;
    return [0];
  }
  
  paramAsString(src) {
    if (!src) return "";
    if (typeof(src) === "string") return src;
    return "";
  }

  glGetBooleanv(pname, dstp) {
    const rsp = this.paramAsInts(this.gl.getParameter(pname));
    dstp >>= 2;
    for (let i=0; i<rsp.length; i++, dstp++) {
      this.egg.exec.mem32[dstp] = rsp[i];
    }
  }

  glGetIntegerv(pname, dstp) {
    const rsp = this.paramAsInts(this.gl.getParameter(pname));
    dstp >>= 2;
    for (let i=0; i<rsp.length; i++, dstp++) {
      this.egg.exec.mem32[dstp] = rsp[i];
    }
  }

  glGetFloatv(pname, dstp) {
    const rsp = this.paramAsFloats(this.gl.getParameter(pname));
    dstp >>= 2;
    for (let i=0; i<rsp.length; i++, dstp++) {
      this.egg.exec.memf32[dstp] = rsp[i];
    }
  }
  
  glGetString(pname) {
    const rsp = this.paramAsString(this.gl.getParameter(pname));
    this.egg.exec.safeWrite(this.glstrp, this.glstra, rsp + "\0");
    return this.glstrp;
  }

  glGetBufferParameteriv(target, pname, dstp) {
    // Every defined field is a single int.
    const rsp = this.gl.getBufferParameter(target, pname);
    this.egg.exec.mem32[dstp >> 2] = rsp;
  }

  glGetFramebufferAttachmentParameteriv(target, attachment, pname, dstp) {
    const rsp = this.paramAsInts(this.gl.getFramebufferAttachmentParameter(target, attachment, pname));
    dstp >>= 2;
    for (let i=0; i<rsp.length; i++, dstp++) {
      this.egg.exec.mem32[dstp] = rsp[i];
    }
  }

  glGetProgramiv(pid, pname, dstp) {
    // Every defined field is a single int.
    const program = this.o[pid];
    const rsp = this.gl.getProgramParameter(program, pname);
    this.egg.exec.mem32[dstp >> 2] = rsp;
  }

  glGetRenderbufferParameteriv(target, pname, dstp) {
    // Every defined field is a single int.
    const rsp = this.gl.getRenderbufferParameter(target, pname);
    this.egg.exec.mem32[dstp >> 2] = rsp;
  }

  glGetShaderiv(sid, pname, dstp) {
    // Every defined field is a single int.
    const shader = this.o[sid];
    const rsp = this.gl.getShaderParameter(shader, pname);
    this.egg.exec.mem32[dstp >> 2] = rsp;
  }

  glGetTexParameterfv(target, pname, dstp) {
    const rsp = this.paramAsInts(this.gl.getTexParameter(target, pname));
    dstp >>= 2;
    for (let i=0; i<rsp.length; i++, dstp++) {
      this.egg.exec.memf32[dstp] = rsp[i];
    }
  }

  glGetTexParameteriv(target, pname, dstp) {
    const rsp = this.paramAsFloats(this.gl.getTexParameter(target, pname));
    dstp >>= 2;
    for (let i=0; i<rsp.length; i++, dstp++) {
      this.egg.exec.mem32[dstp] = rsp[i];
    }
  }

  glGetUniformfv(pid, id, dstp) {
    const program = this.o[pid];
    const rsp = this.paramAsFloats(this.gl.getUniform(program, this.locv[id]));
    dstp >>= 2;
    for (let i=0; i<rsp.length; i++, dstp++) {
      this.egg.exec.memf32[dstp] = rsp[i];
    }
  }

  glGetUniformiv(pid, id, dstp) {
    const program = this.o[pid];
    const rsp = this.paramAsInts(this.gl.getUniform(program, this.locv[id]));
    dstp >>= 2;
    for (let i=0; i<rsp.length; i++, dstp++) {
      this.egg.exec.mem32[dstp] = rsp[i];
    }
  }

  glGetVertexAttribfv(index, pname, dstp) {
    const rsp = this.paramAsFloats(this.gl.getVertexAttrib(index, pname));
    dstp >>= 2;
    for (let i=0; i<rsp.length; i++, dstp++) {
      this.egg.exec.memf32[dstp] = rsp[i];
    }
  }

  glGetVertexAttribiv(index, pname, dstp) {
    const rsp = this.paramAsInts(this.gl.getVertexAttrib(index, pname));
    dstp >>= 2;
    for (let i=0; i<rsp.length; i++, dstp++) {
      this.egg.exec.mem32[dstp] = rsp[i];
    }
  }

  glGetVertexAttribPointerv(index, pname, dstp) {
    const offset = this.gl.getVertexAttribOffset(index, pname);
    this.egg.exec.mem32[dstp >> 2] = offset;
  }

  glTexParameterfv(target, pname, srcp) {
    // No defined texture field uses a vector.
    this.gl.texParameterf(target, pname, this.egg.exec.memf32[srcp >> 2]);
  }

  glTexParameteriv(target, pname, srcp) {
    // No defined texture field uses a vector.
    this.gl.texParameteri(target, pname, this.egg.exec.mem32[srcp >> 2]);
  }

  glUniform1fv(id, c, p) {
    this.gl.uniform1fv(this.locv[id], this.egg.exec.memf32.slice(p >> 2, c));
  }

  glUniform1iv(id, c, p) {
    this.gl.uniform1iv(this.locv[id], this.egg.exec.mem32.slice(p >> 2, c));
  }

  glUniform2fv(id, c, p) {
    this.gl.uniform2fv(this.locv[id], this.egg.exec.memf32.slice(p >> 2, c * 2)); 
  }

  glUniform2iv(id, c, p) {
    this.gl.uniform2iv(this.locv[id], this.egg.exec.mem32.slice(p >> 2, c * 2));
  }

  glUniform3fv(id, c, p) {
    this.gl.uniform3fv(this.locv[id], this.egg.exec.memf32.slice(p >> 2, c * 3)); 
  }

  glUniform3iv(id, c, p) {
    this.gl.uniform3iv(this.locv[id], this.egg.exec.mem32.slice(p >> 2, c * 3));
  }

  glUniform4fv(id, c, p) {
    this.gl.uniform4fv(this.locv[id], this.egg.exec.memf32.slice(p >> 2, c * 4)); 
  }

  glUniform4iv(id, c, p) {
    this.gl.uniform4iv(this.locv[id], this.egg.exec.mem32.slice(p >> 2, c * 4));
  }

  glUniformMatrix2fv(id, c, trans, p) {
    this.gl.uniformMatrix2fv(this.locv[id], trans, this.egg.exec.memf32.slice(p >> 2, c * 4));
  }

  glUniformMatrix3fv(id, c, trans, p) {
    this.gl.uniformMatrix3fv(this.locv[id], trans, this.egg.exec.memf32.slice(p >> 2, c * 9));
  }

  glUniformMatrix4fv(id, c, trans, p) {
    this.gl.uniformMatrix4fv(this.locv[id], trans, this.egg.exec.memf32.slice(p >> 2, c * 16));
  }

  glVertexAttrib1fv(index, p) {
    this.gl.vertexAttrib1fv(index, this.egg.exec.memf32.slice(p >> 2, 1));
  }

  glVertexAttrib2fv(index, p) {
    this.gl.vertexAttrib2fv(index, this.egg.exec.memf32.slice(p >> 2, 2));
  }

  glVertexAttrib3fv(index, p) {
    this.gl.vertexAttrib3fv(index, this.egg.exec.memf32.slice(p >> 2, 3));
  }

  glVertexAttrib4fv(index, p) {
    this.gl.vertexAttrib4fv(index, this.egg.exec.memf32.slice(p >> 2, 4));
  }

  glVertexAttribPointer(index, size, type, norm, stride, p) {
    //TODO WebGL (p) is an offset in the bound vbo. GLES2 it can be an absolute pointer in client memory.
    this.gl.vertexAttribPointer(index, size, type, norm, stride, p);
  }
  
  /* Image operations.
   ****************************************************************************/
   
  measureImage(w, h, fmt, type) {
    if ((w < 1) || (w > 4096)) return 0;
    if ((h < 1) || (h > 4096)) return 0;
    let chanc = 0;
    switch (fmt) {
      case this.gl.STENCIL_INDEX8:
      case this.gl.DEPTH_COMPONENT:
      case this.gl.LUMINANCE:
      case this.gl.ALPHA: chanc=1; break;
      case this.gl.LUMINANCE_ALPHA: chanc=2; break;
      case this.gl.RGB: chanc=3; break;
      case this.gl.RGBA: chanc=4; break;
    }
    if (chanc < 1) return 0;
    let wordlen = 0;
    switch (type) {
      case this.gl.UNSIGNED_BYTE:
      case this.gl.BYTE:
        wordlen=1;
        break;
      case this.gl.UNSIGNED_SHORT:
      case this.gl.SHORT:
      case this.gl.UNSIGNED_SHORT_5_6_5:
      case this.gl.UNSIGNED_SHORT_4_4_4_4:
      case this.gl.UNSIGNED_SHORT_5_5_5_1:
        wordlen=2;
        break;
      case this.gl.UNSIGNED_INT:
      case this.gl.INT:
      case this.gl.FLOAT:
        wordlen=4;
        break;
    }
    if (wordlen < 1) return 0;
    return wordlen * chanc * w * h;
  }

  glCompressedTexImage2D(target, level, ifmt, w, h, border, size, srcp) {
    const src = this.egg.exec.getView(srcp, size);
    if (!src) return;
    this.gl.compressedTexImage2D(target, level, ifmt, w, h, border, src);
  }

  glCompressedTexSubImage2D(target, level, xo, yo, w, h, fmt, size, srcp) {
    const src = this.egg.exec.getView(srcp, size);
    if (!src) return;
    this.gl.compressedTexImage2D(target, level, xo, yo, w, h, fmt, src);
  }

  glCopyTexImage2D(target, level, ifmt, x, y, w, h, border) {
    this.gl.copyTexImage2D(target, level, ifmt, x, y, w, h, border);
  }

  glCopyTexSubImage2D(target, level, xo, yo, x, y, w, h) {
    this.gl.copyTexSubImage2D(target, level, xo, yo, x, y, w, h);
  }

  glReadPixels(x, y, w, h, fmt, type, dstp) {
    const len = this.measureImage(w, h, fmt, type);
    if (len < 1) return;
    const dst = this.egg.exec.getView(dstp, len);
    if (!dst) return;
    this.gl.readPixels(x, y, w, h, fmt, type, dst);
  }

  glTexImage2D(target, level, ifmt, w, h, border, fmt, type, srcp) {
    const len = this.measureImage(w, h, fmt, type);
    if (len < 1) return;
    const src = this.egg.exec.getView(srcp, len);
    if (!src) return;
    this.gl.texImage2D(target, level, ifmt, w, h, border, fmt, type, src);
  }

  glTexSubImage2D(target, level, xo, yo, w, h, fmt, type, srcp) {
    const len = this.measureImage(w, h, fmt, type);
    if (len < 1) return;
    const src = this.egg.exec.getView(srcp, len);
    if (!src) return;
    this.gl.texSubImage2D(target, level, xo, yo, w, h, fmt, type, src);
  }

}
