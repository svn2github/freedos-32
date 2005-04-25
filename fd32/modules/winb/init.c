#include <unistd.h>
#include <sys/syslimits.h>
#include "winb.h"

/* The truncate function changes the size of filename to length. */
int ftruncate(int fd, off_t length)
{
  return -1;
}

/* Temporary implemented sbrk 'til the libfd32 is mature */
void *sbrk(int incr)
{
  extern struct psp *current_psp;
  uint32_t prev_heap_end = 0;
  int res;

  if (current_psp->memlimit != 0) {
    prev_heap_end = current_psp->memlimit;
    if (incr > 0) {
      res = mem_get_region(current_psp->memlimit, incr);
      if (res == -1) {
        /* Alternative heap increament (not continuous) */
        prev_heap_end = current_psp->memlimit = mem_get(incr);
        if (prev_heap_end == 0) {
          message("[WINB] SBRK problem: cannot memget(%x %x)\n", current_psp->memlimit, incr);
          mem_dump();
          return 0;
        }
      }
    } else if (incr < 0) {
      mem_free(current_psp->memlimit + incr, -incr);
    }
    current_psp->memlimit += incr;
  } else {
    message("sbrk error: Memory Limit == 0!\n");
    fd32_abort();
  }

#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] SBRK: memget(%x %x)\n", prev_heap_end, incr);
#endif
  return (void *)prev_heap_end;
}

long sysconf(int parameter)
{
  long res;
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] SYSCONF parameter: %x\n", parameter);
#endif
  switch (parameter)
  {
    case _SC_ARG_MAX:
      res = ARG_MAX;
      break;
    case _SC_PAGESIZE:
      res = 0x1000;
      break;
    default:
      res = -1;
      break;
  }
  
  return res;
}

static int isspecial(double d)
{
  /* IEEE standard number rapresentation */
  register struct IEEEdp {
    unsigned long manl:32;
    unsigned long manh:20;
    unsigned long exp:11;
    unsigned long sign:1;
  } *ip = (struct IEEEdp *) &d;

  if (ip->exp != 0x7ff)
    return (0);
  if (ip->manh || ip->manl) {
    return (1);
  } else if (ip->sign) {
    return (2);
  } else {
    return (3);
  }
}

int isinf(double x)
{
  return (isspecial(x) > 1);
}

int isnan(double x)
{
  return (isspecial(x) == 1);
}

void winbase_init(void)
{
  message("Initing WINB module ...\n");
  install_kernel32();
  install_user32();
  install_advapi32();
  install_oleaut32();
  install_msvcrt();
}
