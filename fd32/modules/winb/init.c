#include <unistd.h>
#include <sys/stat.h>
#include <sys/syslimits.h>
#include "winb.h"


#define FD32_PAGE_SIZE 0x1000

/* The truncate function changes the size of filename to length. */
int ftruncate(int fd, off_t length)
{
  return -1;
}

int access(const char *fn, int flags)
{
  return -1;
}

static int isspecial(double d)
{
  /* IEEE standard number rapresentation */
  register struct IEEEdp {
    unsigned long manl:32;
    unsigned long manh:20;
    unsigned long exp:11;
    unsigned long sign:1;
  } *ip = (void *) &d;

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

typedef union 
{
  double value;
  struct 
  {
    uint32_t msw;
    uint32_t lsw;
  } parts;
} ieee_double_shape_type;

double nan(const char *unused)
{
  ieee_double_shape_type x;
  
  x.parts.msw = 0x7ff80000;
  x.parts.lsw = 0x0;

  return x.value;
}

typedef union
{
  float value;
  uint32_t word;
} ieee_float_shape_type;

float nanf(const char *unused)
{
  ieee_float_shape_type x;

  x.word = 0x7fc00000;

  return x.value;
}

void winbase_init(void)
{
  message("Initing WINB module ...\n");
  //message("Current time: %ld\n", time(NULL));
  install_kernel32();
  install_user32();
  install_advapi32();
  install_oleaut32();
  install_msvcrt();
}
