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

int getpid()
{
  message("GetPID called: faking...\n");
  return(1);
}

