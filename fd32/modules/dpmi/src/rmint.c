/* DPMI Driver for FD32: real mode interrupt simulator
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/hw-data.h>
#include <ll/i386/hw-instr.h>
#include <ll/i386/hw-func.h>
#include <ll/i386/mem.h>
#include <ll/i386/string.h>
#include <ll/i386/error.h>
#include <ll/i386/cons.h>
#include <logger.h>
#include <kernel.h>
#include "rmint.h"

extern int dosidle_int(union rmregs *r);
extern int keybbios_int(union rmregs *r);
extern int videobios_int(union rmregs *r);
extern int mousebios_int(union rmregs *r);
extern int fastconsole_int(union rmregs *r);
extern void int21_handler(union rmregs *r);

//#define __RM_INT_DEBUG__

int fd32_real_mode_int(int intnum, DWORD rmcs_address)
{
  union rmregs *r1;
  int res;

  r1 = (union rmregs *)rmcs_address;
  switch (intnum) {
    case 0x10:
      res = videobios_int(r1);
      return res;
#if 1	/* This is not needed, for now... */
    case 0x16:
      res = keybbios_int(r1);
      return res;
#endif
    case 0x21:
#ifdef __RM_INT_DEBUG__
      fd32_log_printf("INT 21h - AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x DS=%04x ES=%04x...\n",
                      r1->x.ax, r1->x.bx, r1->x.cx, r1->x.dx,
                      r1->x.si, r1->x.di, r1->x.ds, r1->x.es);
#endif
      int21_handler(r1);
#ifdef __RM_INT_DEBUG__
      if (r1->x.flags & 0x0001) {
        fd32_log_printf("Failed  - ");
      } else {
        fd32_log_printf("Success - ");
      }
      fd32_log_printf("AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x DS=%04x ES=%04x\n",
                      r1->x.ax, r1->x.bx, r1->x.cx, r1->x.dx,
                      r1->x.si, r1->x.di, r1->x.ds, r1->x.es);
#endif
      return 0;

    case 0x28:
      res = dosidle_int(r1);
      return res;

    case 0x29:
      res = fastconsole_int(r1);
      return res;

    case 0x2F:
      /* Warning: Don't know what this is... */
      return 0;

    case 0x33:
      return mousebios_int(r1);

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

  return -1; /* Only to avoid warnings... */
}
