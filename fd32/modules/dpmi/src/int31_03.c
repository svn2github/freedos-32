/* DPMI Driver for FD32: handler for INT 0x31, Service 0x03
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include<ll/i386/hw-data.h>
#include<ll/i386/hw-instr.h>
#include<ll/i386/hw-func.h>
#include<ll/i386/mem.h>
#include<ll/i386/string.h>
#include<ll/i386/error.h>
#include<ll/i386/cons.h>

#include "logger.h"

#include "dpmi.h"
#include "rmint.h"
#include "int31_03.h"

void int31_0300(union regs *r)
{
  BYTE f;
  DWORD base, limit;
  int res;

  f = (BYTE)(r->d.ebx >> 8);

#ifdef __DEBUG__
  fd32_log_printf("[DPMI]: Simulate RM interrupt (INT 0x%x)\n",
	  r->h.bl);
  fd32_log_printf("    Real Mode Call Structure @ 0x%x:0x%lx\n",
	  r->x.es, r->d.edi);
#endif
  base = GDT_read((WORD)r->d.ees, &limit, NULL, NULL);
#ifdef __DEBUG__
  fd32_log_printf("    ES: base=0x%lx, limit=0x%lx\n", base, limit);
#endif

  res = fd32_real_mode_int(r->h.bl, base + r->d.edi);
  if (res != 0) {
      r->x.ax = res;
      SET_CARRY;
  } else {
      CLEAR_CARRY;
  }
}

void int31_0303(union regs *r)
{
#ifdef __DEBUG__
  fd32_log_printf("[DPMI]: Allocate Real Mode CallBack Address\n");
  fd32_log_printf("    Real Mode Call Structure @ 0x%x:0x%lx\n",
                  (WORD)r->d.ees, r->d.edi);
  fd32_log_printf("    Procedure to Call @ 0x%x:0x%lx\n", (WORD)r->d.eds, r->d.esi);
#endif

  /* TODO: This must be completely implemented */

  r->d.ecx = 0;
  r->d.edx = 0;
  r->d.eax = r->d.eax & 0xFFFF0000;
  CLEAR_CARRY;
}

void int31_0304(union regs *r)
{
#ifdef __DEBUG__
  fd32_log_printf("[DPMI]: Free Real Mode CallBack Address\n");
  fd32_log_printf("    Real Mode CallBack Address: 0x%x:0x%lx\n",
                  r->d.ecx, r->d.edx);
#endif

  /* TODO: This must be completely implemented too*/
  r->d.eax = r->d.eax & 0xFFFF0000;
  CLEAR_CARRY;
}
