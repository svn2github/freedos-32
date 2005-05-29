#include <unistd.h>

#include "sys/syscalls.h"

extern struct psp *current_psp;

void __exit(int res)
{
//  void restore_sp(int res);

#ifdef __NATIVE_LIBC_DEBUG__
  fd32_log_printf("[DPMI] Return to DOS: return code 0x%x\n", res);
  fd32_log_printf("Current stack: 0x%lx\n", get_sp());
#endif
  /* Restore the PSP */
  fd32_free_jft(current_psp->jft, current_psp->jft_size);
  current_psp = current_psp->link;

  restore_sp(res);
  /* We do not arrive here... This is just for avoiding warnings */
  while(1);
}
