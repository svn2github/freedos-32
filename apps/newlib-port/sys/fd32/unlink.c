#include <unistd.h>

#include "sys/syscalls.h"

int _unlink(const char *name)
{
  int res;
  
  res = fd32_unlink((char *)name, FD32_FAALL | FD32_FRNONE);

  return res;
}
