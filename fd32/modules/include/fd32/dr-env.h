#ifndef __FD32_DRENV_H
#define __FD32_DRENV_H

#include <ll/i386/hw-data.h>
#include <ll/i386/hw-func.h>
#include <ll/i386/hw-instr.h>
#include <ll/i386/pic.h>
#include <ll/i386/stdio.h>
#include <ll/i386/error.h>
#include <ll/i386/stdlib.h>
#include <ll/i386/string.h>
#include <ll/ctype.h>

#include <dr-env/bios.h>
#include <dr-env/mem.h>
#include <dr-env/datetime.h>

#include "kmem.h"
#include "kernel.h"
#include "logger.h"

#define fd32_outb(port, data)   outp(port, data)
#define fd32_outw(port, data)   outpw(port, data)
#define fd32_outd(port, data)   outpd(port, data)
#define fd32_inb                inp
#define fd32_inw                inpw
#define fd32_ind                inpd

#define fd32_message            message
#define fd32_log_printf         fd32_log_printf
#define fd32_error              error

#define fd32_kmem_get           (void *)mem_get
#define fd32_kmem_get_region    mem_get_region
#define fd32_kmem_free(m, size) mem_free((DWORD)m, size)

#define fd32_cli                cli
#define fd32_sti                sti

#if 1
#define itoa(value, string, radix)  dcvt(value, string, radix, 10, 0)
#else
/* Follows the itoa function of the DJGPP libc.              */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
/* FIX ME: This is here only as a temporary solution...      */
extern inline char *itoa(int value, char *string, int radix)
{
  char tmp[33];
  char *tp = tmp;
  int i;
  unsigned v;
  int sign;
  char *sp;

  if (radix > 36 || radix <= 1) return NULL;

  sign = (radix == 10 && value < 0);
  if (sign) v = -value;
       else v = (unsigned)value;

  while (v || tp == tmp)
  {
    i = v % radix;
    v = v / radix;
    if (i < 10) *tp++ = i+'0';
           else *tp++ = i + 'a' - 10;
  }

  if (string == 0) string = (char *) mem_get((tp-tmp)+sign+1);
  sp = string;

  if (sign) *sp++ = '-';
  while (tp > tmp) *sp++ = *--tp;
  *sp = 0;
  return string;
}
#endif

#if 0
extern inline int fd32_set_handler(int intnum, void *handler)
{
  extern GATE IDT[256];
  DWORD f;
  
  f = ll_fsave();
  IDT[intnum].sel = get_CS();
  IDT[intnum].offset_hi = (WORD)(((DWORD)handler) >> 16);
  IDT[intnum].offset_lo = (WORD)((DWORD)handler);
  ll_frestore(f);
  return 1;
}
#else
#define fd32_set_handler(n, h) x_irq_bind(n, h); irq_unmask(n);
#endif

#endif /* #ifndef __FD32_DRENV_H */

