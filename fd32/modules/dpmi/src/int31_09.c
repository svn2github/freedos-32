/* DPMI Driver for FD32: handler for INT 0x31, Service 0x09
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include<ll/i386/hw-data.h>
#include<ll/i386/hw-instr.h>
#include<ll/i386/hw-func.h>
#include<ll/i386/error.h>
#include<ll/i386/cons.h>

#include "logger.h"

#include "dpmi.h"
#include "dpmiexc.h"

void int31_0900(union regs *r)
{
#ifdef __DEBUG__
  fd32_log_printf("[DPMI] Interrupts disable\n");
#endif

  /* Do something... */
  r->h.al = ((r->d.flags&CPU_FLAG_IF) != 0);
  r->d.flags &= ~CPU_FLAG_IF;
  CLEAR_CARRY;

  return;
}

void int31_0901(union regs *r)
{
#ifdef __DEBUG__
  fd32_log_printf("[DPMI] Interrupts enable\n");
#endif

  /* Do something... */
  r->h.al = ((r->d.flags&CPU_FLAG_IF) != 0);
  r->d.flags |= CPU_FLAG_IF;
  CLEAR_CARRY;

  return;
}
