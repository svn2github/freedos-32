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

    case 0x16:
      res = keybbios_int(r1);
      return res;

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
      res = 0;
      switch(r1->x.ax)
      {
        /* MS Windows - WINDOWS ENHANCED MODE INSTALLATION CHECK */
        case 0x1600:
          r1->h.al = 0;
          break;
        /* MS Windows, DPMI, various - RELEASE CURRENT VIRTUAL MACHINE TIME-SLICE */
        case 0x1680:
          res = dosidle_int(r1); /* TODO: support multitasking here? */
          break;
        /* Windows95 - TITLE - SET / GET APPLICATION / VIRTUAL MACHINE TITLE */
        case 0x168E:
          message("Unsupported INT 0x%x EAX: 0x%lx EDX: 0x%lx\n", intnum, r1->d.eax, r1->d.edx);
          r1->x.ax = 0;
          break;
        /* OS/2 v2.0+ - INSTALLATION CHECK / GET VERSION */
        case 0x4010:
          message("Unsupported INT 0x%x EAX: 0x%lx\n", intnum, r1->d.eax);
          /* TODO: should res = -1; */
          break;
        default:
          /* WARNING: PRINT/SHARE/DOS internal to be implemented... */
          message("Unsupported INT 0x%x EAX: 0x%lx\n", intnum, r1->d.eax);
          res = -1;
      }
      return res;

    case 0x33:
      return mousebios_int(r1);

    case 0x4B:
      /* WARNING: Virtual DMA Specification not supported */
      message("Unsupported INT 0x%x EAX: 0x%lx (VDM)\n", intnum, r1->d.eax);
      if (r1->x.ax == 0x8102)
        res = 0x0F; /* Function not supported */
      else
        res = 0x0F;
      return res;
  }

  message("Unsupported INT 0x%x\n\n", intnum);
  /* Warning!!! Perform a check on ES!!! */
  message("EDI: 0x%lx\tESI: 0x%lx\tEBP: 0x%lx\n",
    r1->d.edi, r1->d.esi, r1->d.ebp);
  message("EBX: 0x%lx\tEDX: 0x%lx\tECX: 0x%lx\tEAX: 0x%lx\n",
    r1->d.ebx, r1->d.edx, r1->d.ecx, r1->d.eax);
  message("ES: 0x%x\tDS: 0x%x\tFS: 0x%x\tGS: 0x%x\n",
    r1->x.es, r1->x.ds, r1->x.fs, r1->x.gs);
  message("IP: 0x%x  CS: 0x%x  SP: 0x%x  SS: 0x%x\n",
    r1->x.ip, r1->x.cs, r1->x.sp, r1->x.ss);

  fd32_abort();

  return -1; /* Only to avoid warnings... */
}
