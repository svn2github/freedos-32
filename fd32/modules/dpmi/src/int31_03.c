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

#include "dpmi.h"
#include "int31_03.h"
#include <logger.h>

/*
#define __DEBUG__
*/

void int31_0300(union regs *r)
{
  BYTE intnum;
  BYTE f;
//  struct rmcall *r1;
  union rmregs *r1;
  DWORD base, limit;
  int res;
  /*
  DWORD memaddr;
  char filename[256];
  char string[255];
  int n;
  */

  intnum = (BYTE)r->d.ebx;
  f = (BYTE)(r->d.ebx >> 8);

#ifdef __DEBUG__
  fd32_log_printf("[FD32]: Simulate RM interrupt (INT 0x%x\n)", intnum);
  fd32_log_printf("Real Mode Call Structure @ 0x%x:0x%lx\n",
                  (WORD)r->d.ees, r->d.edi);
#endif
  base = GDT_read((WORD)r->d.ees, &limit, NULL, NULL);
#ifdef __DEBUG__
  fd32_log_printf("ES: base=0x%lx, limit=0x%lx\n", base, limit);
#endif
//  r1 = (struct rmcall *)(base + r->d.edi);
  r1 = (union rmregs *)(base + r->d.edi);
  switch (intnum) {
    case 0x10:
      res = videobios_int(r1);
      if (res != 0) {
	r->x.ax = res;
	SET_CARRY;
      } else {
	CLEAR_CARRY;
      }
      return;
    case 0x21:
    /*
      res = dos_int(r1);
      if (res != 0) {
	r->x.ax = res;
	SET_CARRY;
      } else {
	CLEAR_CARRY;
      }
      return;
    */
      fd32_log_printf("INT 21h - AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x DS=%04x ES=%04x...\n",
                      r1->x.ax, r1->x.bx, r1->x.cx, r1->x.dx,
                      r1->x.si, r1->x.di, r1->x.ds, r1->x.es);
      int21_handler(r1);
      if (r1->x.flags & 0x0001)
        fd32_log_printf("Failed  - ");
       else
        fd32_log_printf("Success - ");
      fd32_log_printf("AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x DS=%04x ES=%04x\n",
                      r1->x.ax, r1->x.bx, r1->x.cx, r1->x.dx,
                      r1->x.si, r1->x.di, r1->x.ds, r1->x.es);
      CLEAR_CARRY;
      return;

    case 0x2F:
      /* Warning: Don't know what this is... */
    return;

  }

  message("Unsupported INT 0x%x\n", intnum);
  /* Warning!!! Perform a check on ES!!! */
  message("\n EDI: 0x%lx    ESI: 0x%lx    EBP: 0x%lx\n",
	  r1->d.edi, r1->d.esi, r1->d.ebp);
  message("EBX: 0x%lx    EDX: 0x%lx    ECX: 0x%lx    EAX: 0x%lx\n",
	  r1->d.ebx, r1->d.edx, r1->d.ecx, r1->d.eax);
  message("ES: 0x%x    DS: 0x%x    FS: 0x%x    GS: 0x%x\n",
	  r1->x.es, r1->x.ds, r1->x.fs, r1->x.gs);
  message("IP: 0x%x    CS: 0x%x    SP: 0x%x    SS: 0x%x\n",
	  r1->x.ip, r1->x.cs, r1->x.sp, r1->x.ss);

  fd32_abort();
}

void int31_0303(union regs *r)
{
#ifdef __DEBUG__
  fd32_log_printf("[FD32]: Allocate Real Mode CallBack Address\n");
  fd32_log_printf("Real Mode Call Structure @ 0x%x:0x%lx\n",
                  (WORD)r->d.ees, r->d.edi);
  fd32_log_printf("Procedure to Call @ 0x%x:0x%lx\n", (WORD)r->d.eds, r->d.esi);
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
  fd32_log_printf("[FD32]: Free Real Mode CallBack Address\n");
  fd32_log_printf("Real Mode CallBack Address: 0x%x:0x%lx\n",
                  r->d.ecx, r->d.edx);
#endif

  /* TODO: This must be completely implemented too*/
  r->d.eax = r->d.eax & 0xFFFF0000;
  CLEAR_CARRY;
}
