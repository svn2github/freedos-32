/* DPMI Driver for FD32: handler for INT 0x31, Service 0x04
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include<ll/i386/hw-data.h>
#include<ll/i386/hw-instr.h>
#include<ll/i386/hw-arch.h>
#include<ll/i386/mem.h>
#include<ll/i386/error.h>
#include<ll/i386/cons.h>

#include "dpmi.h"
#include "int31_04.h"
#include <logger.h>

void int31_0400(union regs *r)
{
  struct ll_cpuInfo cpu;

#ifdef __DEBUG__
  fd32_log_printf("[FD32] Get DPMI Information\n");
#endif
  CLEAR_CARRY;
  /* DPMI Version: 0.9 */
  r->d.eax = ((r->d.eax & 0xFFFF0000) | 0x00 << 8 | 0x09);
  /* DPMI Flags: 1 ---> running on 0x386 */
  r->d.ebx = ((r->d.ebx & 0xFFFF0000) | 0x01);
  /* CL = Processor type */
  X86_get_CPU(&cpu);
  r->d.ecx = ((r->d.ecx & 0xFFFFFF00) | (cpu.X86_cpu & 0xFF));
  /* Master & Slave base interrupts... */
  r->d.edx = ((r->d.edx & 0xFFFF0000) | 0x70 << 8 | 0x40); /* I hope... */
}
