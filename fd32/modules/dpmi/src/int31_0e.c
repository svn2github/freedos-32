/* DPMI Driver for FD32: handler for INT 0x31, Service 0x0E
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include<ll/i386/hw-data.h>
#ifdef __DEBUG__
#include<logger.h>
#endif

#include "dpmi.h"
#include "int31_0e.h"

void int31_0E01(union regs *r)
{
#ifdef __DEBUG__
  fd32_log_printf("[FD32] Set FPU Emulation: mode 0x%lx\n", *ebx);
#endif

  /* This is DPMI 1.0: we don't support it... */
  SET_CARRY;
}
