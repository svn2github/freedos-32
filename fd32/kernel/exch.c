/* FD/32 Exception & RM Interrupt Handlers
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/hw-func.h>
#include <ll/i386/pic.h>
#include <ll/i386/error.h>
#include <ll/i386/mem.h>
#include <ll/i386/x-bios.h>

#include "handler.h"
#include "kernel.h"

struct handler exc_table[32];
DWORD rm_irq_table[256];

void (* cpu_release)(void);

static void halt_it(void)
{
  /* Uhmmm.... Here, the idea is to block.
     We need to define a good kernel semantic for this; for the moment,
     I simply try halting the system...
  */
  __asm__ __volatile__ ("hlt");
}

void fd32_cpu_idle(void)
{
  cpu_release();
}

void exception_handler(int n, DWORD code, DWORD eip, DWORD cs, DWORD ef)
{
  WORD ctx;

  ctx = context_save();
  if (ctx == X_VM86_TSS) {
    /* HACK!!! Makes bochs bios happier... */
    return;
  }

#ifdef __EXC_DEBUG__
  fd32_log_printf("Exception %d occurred\n", n);
  fd32_log_printf("    Error Code: %ld\n", code);
  fd32_log_printf("    EIP: %ld (0x%lx)\n", eip, eip);
  fd32_log_printf("    CS: %ld (0x%lx)\n", cs, cs);
  fd32_log_printf("    EFlags: %ld (0x%lx)\n", ef, ef);
#endif

  if (exc_table[n].cs == 0) {
    message("Unhandled exception... %d (%x)\n", n, n);
  } else {
#ifdef __EXC_DEBUG
    fd32_log_printf("I have to call 0x%x:0x%lx\n", exc_table[n].cs, exc_table[n].eip);
#endif
    farcall(exc_table[n].cs, exc_table[n].eip);
  }
  /* For the moment, let's abort anyway... */
  fd32_abort();
}

void kernel_init(void)
{
  int i;

  memcpy(rm_irq_table, 0, 1024);

  for (i = 0; i < 32; i++) {
    exc_table[i].cs = 0;
    exc_table[i].eip = 0;
    l1_exc_bind(i, (void *)exception_handler);  
  }

  cpu_release = halt_it;
}
