/* DPMI Driver for FD32: handler for INT 0x31, Service 0x02
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include<ll/i386/hw-data.h>
#include<ll/i386/hw-instr.h>
#include<ll/i386/hw-func.h>
#include<ll/i386/error.h>
#include<ll/i386/cons.h>

#include "dpmi.h"
#include "handler.h"
#include "int31_02.h"
#include <logger.h>

#define __DEBUG__

extern DWORD ll_exc_table[16];
extern BYTE exc_table[1];

extern GATE IDT[256];

extern DWORD rm_irq_table[256];

void int31_0200(union regs *r)
{
  BYTE intnum;

  intnum = (BYTE)r->d.ebx;
#ifdef __DEBUG__
  fd32_log_printf("[FD32] Get Real Mode Interrupt Vector: interrupt number %d\n",
                  intnum);
#endif
  CLEAR_CARRY;

  /* This should be OK... Check it! */
  r->d.ecx = ((rm_irq_table[intnum]) & 0xFFFF0000) >> 16;
  r->d.edx = ((rm_irq_table[intnum]) & 0x0000FFFF);
  return;
}

void int31_0201(union regs *r)
{
  BYTE intnum;

  intnum = (BYTE)r->d.ebx;
#ifdef __DEBUG__
  fd32_log_printf("[FD32] Set Real Mode Interrupt Vector: interrupt number %d\n",
                  intnum);
#endif
  CLEAR_CARRY;

  /* Check this... */
  rm_irq_table[intnum] = (r->d.ecx << 16) + (r->d.edx & 0x0000FFFF);
  
  return;
}


void int31_0202(union regs *r)
{
  BYTE excnum;
  struct handler *entry;

  excnum = (BYTE)r->d.ebx;
  entry = exc_table + excnum * sizeof(struct handler);

#ifdef __DEBUG__
  fd32_log_printf("[FD32] Get Exception Handler: exception number %d\n", excnum);
#endif
  if (excnum > 0x1F) {
    /* Error!!! */
    r->d.eax &= 0xFFFF0000;
    r->d.eax |= 0x8021;
    SET_CARRY;

    return;
  }
  
  /* CX:EDX = <selector>:<offset> of the exception handler */
  r->d.ecx = entry->cs;
  r->d.edx = entry->eip;
  CLEAR_CARRY;
}

void int31_0203(union regs *r)
{
  BYTE excnum;
  struct handler *entry;

  excnum = (BYTE)r->d.ebx;
  entry = exc_table + excnum * sizeof(struct handler);

#ifdef __DEBUG__
  fd32_log_printf("[FD32] Set Exception Handler: exception number %d\n", excnum);
  fd32_log_printf("            Handler: 0x%lx:0x%lx\n", r->d.ecx, r->d.edx);
#endif

  if (excnum > 0x1F) {
    /* Error!!! */
    r->d.eax &= 0xFFFF0000;
    r->d.eax |= 0x8021;
    SET_CARRY;

    return;
  }
  /* CX:EDX = <selector>:<offset> of the exception handler */
  /* Set it */
  /* Warn: we have to add a check on the selector value (CX) */
  entry->cs = r->d.ecx;
  entry->eip = r->d.edx;

  CLEAR_CARRY;
}

void int31_0204(union regs *r)
{
  BYTE intnum;

  intnum = (BYTE)r->d.ebx;

#ifdef __DEBUG__
  fd32_log_printf("[FD32] Get Protect Mode Interrupt Vector: interrupt number %d\n",
                  intnum);
#endif
  if (intnum > 0xFF) {
    /* Error!!! */
    r->d.eax &= 0xFFFF0000;
    r->d.eax |= 0x8021;
    SET_CARRY;

    return;
  }
  /* CX:EDX = <selector>:<offset> of the interrupt handler */
  /* Set it */
  /* Warn: we have to add a check on the selector value (CX) */
  r->d.ecx = IDT[intnum].sel;
  r->d.edx = IDT[intnum].offset_hi << 16 | IDT[intnum].offset_lo;
  
  CLEAR_CARRY;
}

void int31_0205(union regs *r)
{
  BYTE intnum;

  intnum = (BYTE)r->d.ebx;

#ifdef __DEBUG__
  fd32_log_printf("[FD32] Set Protect Mode Interrupt Vector: interrupt number %d\n",
                  intnum);
  fd32_log_printf("            Handler: 0x%lx:0x%lx\n", r->d.ecx, r->d.edx);
#endif

  if (intnum > 0xFF) {
    /* Error!!! */
    r->d.eax &= 0xFFFF0000;
    r->d.eax |= 0x8021;
    SET_CARRY;

    return;
  }
  /* CX:EDX = <selector>:<offset> of the interrupt handler */
  /* Set it */
  /* Warn: we have to add a check on the selector value (CX) */
  IDT[intnum].sel = (WORD)(r->d.ecx);
  IDT[intnum].offset_hi = (WORD)((r->d.edx) >> 16);
  IDT[intnum].offset_lo = (WORD)(r->d.edx);
  
  CLEAR_CARRY;
}
