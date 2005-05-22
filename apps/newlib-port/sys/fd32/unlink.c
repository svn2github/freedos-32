#include <unistd.h>

#include <syscalls.h>

int _unlink(const char *name)
{
  int res;
  
  res = fd32_unlink(name, FD32_FAALL | FD32_FRNONE);

  return res;
}
