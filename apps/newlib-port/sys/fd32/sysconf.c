#include <unistd.h>
#include <sys/syslimits.h>

#include "sys/syscalls.h"

long sysconf(int parameter)
{
  long res;
#ifdef __DEBUG__
  fd32_log_printf("[WINB] SYSCONF parameter: %x\n", parameter);
#endif
  switch (parameter)
  {
    case _SC_ARG_MAX:
      res = ARG_MAX;
      break;
    case _SC_PAGESIZE:
      res = FD32_PAGE_SIZE;
      break;
    default:
      res = -1;
      break;
  }
  
  return res;
}
