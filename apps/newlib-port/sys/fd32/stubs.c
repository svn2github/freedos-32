#include <errno.h>
#include <unistd.h>

#include "sys/syscalls.h"

int _fork(void)
{
  message("Fork called: failing...\n");
  errno=EAGAIN;

  return -1;
}

int _wait(int *status)
{
  message("Wait called: failing...\n");
  errno = ECHILD;

  return -1;
}

int _kill(int pid, int sig)
{
  message("Kill called: failing...\n");
  errno = EINVAL;
  return(-1);
}

pid_t _getpid(void)
{
  message("GetPID called: faking...\n");
  return(1);
}

int _link(const char *__path1, const char *__path2 )
{
  /* We do not have hard links... Just fail for now */

  errno = ENOSYS;
  return -1;
}
