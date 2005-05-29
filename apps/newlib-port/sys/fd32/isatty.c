#include <unistd.h>

#include "sys/syscalls.h"

int _isatty(int fd)
{
  int res;

  res = fd32_get_dev_info(fd);
  if ((res & 0x83) == 0x83) {
    return 1;
  }

  return 0;
}
