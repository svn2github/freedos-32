#include <unistd.h>

#include <ll/i386/hw-data.h>
#include <filesys.h>

int unlink(const char *name)
{
  int res;
  
  res = fd32_unlink(name, FD32_FAALL | FD32_FRNONE);

  return res;
}
