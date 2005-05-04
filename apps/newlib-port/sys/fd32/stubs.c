#include <ll/i386/error.h>
#include <errno.h>
#undef errno
extern int errno;

#include <unistd.h>

int fork(void)
{
  message("Fork called: failing...\n");
  errno=EAGAIN;

  return -1;
}

int wait(int *status)
{
  message("Wait called: failing...\n");
  errno = ECHILD;

  return -1;
}

int kill(int pid, int sig)
{
  message("Kill called: failing...\n");
  errno = EINVAL;
  return(-1);
}

pid_t getpid(void)
{
  message("GetPID called: faking...\n");
  return(1);
}

int link(const char *__path1, const char *__path2 )
{
  /* We do not have hard links... Just fail for now */

  errno = EINVAL;
  return -1;
}
