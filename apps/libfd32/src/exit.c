#include <unistd.h>

void _exit(int res)
{
  void restore_sp(int res);

#ifdef __NATIVE_LIBC_DEBUG__
  fd32_log_printf("[DPMI] Return to DOS: return code 0x%x\n", res);
  fd32_log_printf("Current stack: 0x%lx\n", get_sp());
#endif
  restore_sp(res);
}
