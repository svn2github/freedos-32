#include<ll/i386/hw-data.h>
#include<ll/i386/hw-instr.h>
#include<ll/i386/hw-func.h>

#include "handler.h"

#include "dpmiexc.h"

extern DWORD ll_exc_table[16];
extern BYTE exc_table[1];

extern GATE IDT[256];

extern DWORD rm_irq_table[256];



int fd32_get_real_mode_int(int intnum, WORD *segment, WORD *offset)
{
  if ((intnum < 0) || (intnum > 255)) {
    return -1;
  }
/* This should be OK... Check it! */
  *segment = ((rm_irq_table[intnum]) & 0x000FFFF0) >> 4;
  *offset = ((rm_irq_table[intnum]) & 0x000F);

  return 0;
}

int fd32_set_real_mode_int(int intnum, WORD segment, WORD offset)
{
  if ((intnum < 0) || (intnum > 255)) {
    return -1;
  }
  /* Check this... */
  rm_irq_table[intnum] = (segment << 4) + offset;

  return 0;
}

int fd32_get_exception_handler(int excnum, WORD *selector, DWORD *offset)
{
  struct handler *entry;

  if (excnum > 0x1F) {
    return ERROR_INVALID_VALUE;
#if 0
    /* Error!!! */
    r->d.eax &= 0xFFFF0000;
    r->d.eax |= 0x8021;
    SET_CARRY;

    return;
#endif
  }
  
  entry = (struct handler *)(exc_table + excnum * sizeof(struct handler));

  /* CX:EDX = <selector>:<offset> of the exception handler */
  *selector = entry->cs;
  *offset = entry->eip;

  return 0;
}

int fd32_set_exception_handler(int excnum, WORD selector, DWORD offset)
{
  struct handler *entry;

  if (excnum > 0x1F) {
    return ERROR_INVALID_VALUE;
  }
  
  entry = (struct handler *)(exc_table + excnum * sizeof(struct handler));

  /* CX:EDX = <selector>:<offset> of the exception handler */
  /* Set it */
  /* Warn: we have to add a check on the selector value (CX) */
  entry->cs = selector;
  entry->eip = offset;

  return 0;
}

int fd32_get_protect_mode_int(int intnum, WORD *selector, DWORD *offset)
{
  if (intnum > 0xFF) {
    return ERROR_INVALID_VALUE;
  }
  
  /* CX:EDX = <selector>:<offset> of the interrupt handler */
  /* Set it */
  /* Warn: we have to add a check on the selector value (CX) */
  *selector = IDT[intnum].sel;
  *offset = IDT[intnum].offset_hi << 16 | IDT[intnum].offset_lo;
  
  return 0;
}

int fd32_set_protect_mode_int(int intnum, WORD selector, DWORD offset)
{
  if (intnum > 0xFF) {
    return ERROR_INVALID_VALUE;
  }
  
  /* CX:EDX = <selector>:<offset> of the interrupt handler */
  /* Set it */
  /* Warn: we have to add a check on the selector value (CX) */
  IDT[intnum].sel = selector;
  IDT[intnum].offset_hi = (WORD)(offset >> 16);
  IDT[intnum].offset_lo = (WORD)(offset);

  return 0;
}
