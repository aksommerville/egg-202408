#include "eggdev_internal.h"
#include <stdarg.h>
#include <unistd.h>
#include <sys/wait.h>

/* Run shell script, main entry point.
 */
 
int eggdev_shell_script_(const char *scriptpath,...) {

  // Scripts must be in the same directory as this executable.
  // Prepend the executable's dirname.
  const char *pfx=eggdev.exename;
  int pfxc=0,slashp=-1;
  for (;pfx[pfxc];pfxc++) {
    if (pfx[pfxc]=='/') slashp=pfxc;
  }
  pfxc=slashp+1;
  char fullpath[1024];
  int fullpathc=snprintf(fullpath,sizeof(fullpath),"%.*s%s",pfxc,pfx,scriptpath);
  if ((fullpathc<1)||(fullpathc>=sizeof(fullpath))) return -1;
  
  va_list vargs;
  va_start(vargs,scriptpath);
  #define ARGA 8
  char *argv[ARGA];
  int argc=0;
  argv[argc++]=fullpath;
  for (;;) {
    char *arg=va_arg(vargs,char*);
    if (!arg) break;
    argv[argc++]=arg;
    if (argc>=ARGA) return -1; // sic: ==ARGA is an error, not just >
  }
  argv[argc]=0;
  #undef ARGA
  
  int pid=fork();
  if (pid<0) return -1;
  
  // In the parent process, wait for it to complete.
  if (pid) {
    int wstatus=0;
    if (waitpid(pid,&wstatus,0)<0) return -1;
    if (!WIFEXITED(wstatus)) {
      fprintf(stderr,"%s: Abnormal exit.\n",fullpath);
      return -2;
    }
    int status=WEXITSTATUS(wstatus);
    if (status) {
      fprintf(stderr,"%s: Script failed with status %d\n",fullpath,status);
      return -2;
    }
    return 0;
  }
  
  // In the child process, exec.
  execv(fullpath,argv);
  fprintf(stderr,"%s: execv() failed.\n",fullpath);
  return -2;
}

/* Execute a shell command and return its output, synchronously.
 */
 
int eggdev_command_sync(struct sr_encoder *dst,const char *cmd) {
  FILE *f=popen(cmd,"r");
  if (!f) return -1;
  for (;;) {
    if (sr_encoder_require(dst,1024)<0) {
      fclose(f);
      return -1;
    }
    int err=fread((char*)dst->v+dst->c,1,dst->a-dst->c,f);
    if (err<0) {
      fclose(f);
      return -1;
    }
    if (!err) break;
    dst->c+=err;
  }
  int wstatus=pclose(f);
  if (!WIFEXITED(wstatus)) return -1;
  return WEXITSTATUS(wstatus);
}
