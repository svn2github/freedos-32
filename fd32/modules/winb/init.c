#include <ll/i386/error.h>
#include "winbase.h"


void winbase_init(void)
{
  message("Initing winbase dynalink lib ...\n");
  add_kernel32();
  add_msvcrt();
}
