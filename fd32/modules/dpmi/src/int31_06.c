/* DPMI Driver for FD32: handler for INT 0x31, Service 0x06
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include<ll/i386/hw-data.h>
#ifdef __DEBUG__
#include<logger.h>
#endif

#include "dpmi.h"
#include "int31_06.h"

void int31_0600(union regs *r)
{
#ifdef __DEBUG__
  fd32_log_printf("[FD32] Lock Linear Region\n");
#endif

  /* We don't support paging now... Always succeeds */
  CLEAR_CARRY;
}
