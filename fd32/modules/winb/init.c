#include "winb.h"

long sysconf (int parameter)
{
  long res;
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] SYSCONF parameter: %x\n", parameter);
#endif
  switch (parameter)
  {
    case 0:
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
