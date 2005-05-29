#include <unistd.h>

#include "sys/syscalls.h"

int _execve(const char *name, char * const argv[], char * const env[])
{
  uint16_t retcode;
  int res;

  res = dos_exec(name, env, argv, 0, 0, &retcode);
  if (res < 0) {
    return res;
  }

  return retcode;
}
