#include <unistd.h>

#include "sys/syscalls.h"

void _exit(int res)
{
  struct process_info *ppi = fd32_get_current_pi();
#ifdef __NATIVE_LIBC_DEBUG__
  fd32_log_printf("[DPMI] Return to DOS: return code 0x%x\n", res);
  fd32_log_printf("Current stack: 0x%lx\n", get_sp());
#endif

  /* Free sbrk mem track */
  extern void sbrk_mem_clear_up();
  sbrk_mem_clear_up();
  /* Free the JFT */
  fd32_free_jft(ppi->jft, ppi->jft_size);

  restore_sp(res); /* Back to the kernel */
}
