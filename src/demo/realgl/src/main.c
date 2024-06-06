#include <egg/egg.h>
#include <GLES2/gl2.h>

static char glstrbuf[8192];

/* Raw shader, borrowed from our 'render' unit.
 */
 
static int pid=0;
static int u_screensize=0;
static int bufid=0;
 
static const char raw_vsrc[]=
  "#version 100\n"
  "precision mediump float;\n"
  "uniform vec2 screensize;\n"
  "attribute vec2 apos;\n"
  "attribute vec4 acolor;\n"
  "varying vec4 vcolor;\n"
  "void main() {\n"
    "vec2 npos=(apos*2.0)/screensize-1.0;\n"
    "gl_Position=vec4(npos,0.0,1.0);\n"
    "vcolor=acolor;\n"
  "}\n"
"";

static const char raw_fsrc[]=
  "#version 100\n"
  "precision mediump float;\n"
  "varying vec4 vcolor;\n"
  "void main() {\n"
    "gl_FragColor=vcolor;\n"
  "}\n"
"";

static int raw_init() {
  GLint sidv=glCreateShader(GL_VERTEX_SHADER);
  GLint sidf=glCreateShader(GL_FRAGMENT_SHADER);
  if (!sidv||!sidf) {
    egg_log("glCreateShader failed");
    return -1;
  }
  GLint raw_vsrcc=sizeof(raw_vsrc)-1;
  const GLchar *src=raw_vsrc;
  glShaderSource(sidv,1,&src,(void*)&raw_vsrcc);
  glCompileShader(sidv);
  GLint status=0;
  glGetShaderiv(sidv,GL_COMPILE_STATUS,&status);
  if (!status) {
    egg_log("GL_COMPILE_STATUS zero (vertex shader)");
    return -1;
  }
  GLint raw_fsrcc=sizeof(raw_fsrc)-1;
  src=raw_fsrc;
  glShaderSource(sidf,1,&src,&raw_fsrcc);
  glCompileShader(sidf);
  status=0;
  glGetShaderiv(sidf,GL_COMPILE_STATUS,&status);
  if (!status) {
    egg_log("GL_COMPILE_STATUS zero (fragment shader)");
    return -1;
  }
  if (!(pid=glCreateProgram())) {
    egg_log("glCreateProgram failed");
    return -1;
  }
  glAttachShader(pid,sidv);
  glAttachShader(pid,sidf);
  glLinkProgram(pid);
  status=0;
  glGetProgramiv(pid,GL_LINK_STATUS,&status);
  if (!status) {
    egg_log("GL_LINK_STATUS zero");
    return -1;
  }
  glDeleteShader(sidv);
  glDeleteShader(sidf);
  return 0;
}
 
/* Public hooks.
 */

void egg_client_quit() {
}

int egg_client_init() {
  egg_log("%s...",__func__);
  egg_video_set_string_buffer(glstrbuf,sizeof(glstrbuf));
  if (raw_init()<0) {
    egg_log("raw_init failed");
    return -1;
  }
  glUseProgram(pid);
  u_screensize=glGetUniformLocation(pid,"screensize");
  glBindAttribLocation(pid,0,"apos");
  glBindAttribLocation(pid,1,"acolor");
  glGenBuffers(1,(GLuint*)&bufid);
  
  egg_log("GL_VENDOR: %s",glGetString(GL_VENDOR));
  egg_log("GL_RENDERER: %s",glGetString(GL_RENDERER));
  egg_log("GL_VERSION: %s",glGetString(GL_VERSION));
  egg_log("GL_SHADING_LANGUAGE_VERSION: %s",glGetString(GL_SHADING_LANGUAGE_VERSION));
  const char *extensions=(const char*)glGetString(GL_EXTENSIONS);
  if (extensions) {
    egg_log("GL_EXTENSIONS:");
    for (;*extensions;) {
      if ((unsigned char)(*extensions)<=0x20) { extensions++; continue; }
      const char *token=extensions;
      int tokenc=0;
      while (*extensions&&((unsigned char)(*extensions++)>0x20)) tokenc++;
      egg_log("  %.*s",tokenc,token);
    }
  } else {
    egg_log("No response for GL_EXTENSIONS.");
  }
  
  return 0;
}

void egg_client_update(double elapsed) {
}

void egg_client_render() {
  int screenw=0,screenh=0;
  egg_video_get_size(&screenw,&screenh);
  glClearColor(1.0f,0.5f,0.0f,1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(pid);
  glViewport(0,0,screenw,screenh);
  glUniform2f(u_screensize,screenw,screenh);
  
  int16_t ax=screenw>>2;
  int16_t bx=screenw>>1;
  int16_t cx=ax*3;
  int16_t ay=screenh>>2;
  int16_t by=ay*3;
  struct egg_draw_line vtxv[]={
    {bx,by,0xff,0x00,0x00,0xff},
    {ax,ay,0x00,0xff,0x00,0xff},
    {cx,ay,0x00,0x00,0xff,0xff},
  };
  
  glBindBuffer(GL_ARRAY_BUFFER,bufid);
  glBufferData(GL_ARRAY_BUFFER,sizeof(vtxv),vtxv,GL_STREAM_DRAW);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(0,2,GL_SHORT,0,sizeof(struct egg_draw_line),(void*)(uintptr_t)0);
  glVertexAttribPointer(1,4,GL_UNSIGNED_BYTE,1,sizeof(struct egg_draw_line),(void*)(uintptr_t)4);
  glDrawArrays(GL_TRIANGLE_STRIP,0,sizeof(vtxv)/sizeof(vtxv[0]));
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
}
