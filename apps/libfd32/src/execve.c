#include <unistd.h>

#include <ll/i386/hw-data.h>
#include <kernel.h>
#include <filesys.h>

int execve(const char *name, char * const argv[], char * const env[])
{
  WORD retcode;
  int res;

  res = dos_exec(name, env, argv, 0, 0, &retcode);
  if (res < 0) {
    return res;
  }

  return retcode;
}
