#include <unistd.h>

#include <syscalls.h>

int _execve(const char *name, char * const argv[], char * const env[])
{
  WORD retcode;
  int res;

  res = dos_exec(name, env, argv, 0, 0, &retcode);
  if (res < 0) {
    return res;
  }

  return retcode;
}
