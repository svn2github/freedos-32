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
