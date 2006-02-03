/* DPMI Driver for FD32: real mode interrupt simulator
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/hw-data.h>
#include <ll/i386/x-bios.h>
#include <ll/i386/error.h>
#include <logger.h>
#include <kernel.h>
#include <kmem.h>
#include "rmint.h"
#include "dpmi.h"

extern int dosidle_int(union rmregs *r);
extern int keybbios_int(union rmregs *r);
extern int videobios_int(union rmregs *r);
extern int mousebios_int(union rmregs *r);
extern int fastconsole_int(union rmregs *r);
extern void dos21_int(union rmregs *r);

extern char use_biosvga;
extern char use_biosmouse;

static int dpmi_rmint_to_bios(DWORD intnum, volatile union rmregs *r)
{
  DWORD f;
  DWORD stack_mem, pre_stack_mem;
  X_REGS16 regs_r;
  X_SREGS16 selectors_r;

  f = ll_fsave();
  regs_r.x.ax = r->x.ax;
  regs_r.x.bx = r->x.bx;
  regs_r.x.cx = r->x.cx;
  regs_r.x.dx = r->x.dx;
  regs_r.x.si = r->x.si;
  regs_r.x.di = r->x.di;
  regs_r.x.cflag = r->x.flags;
  selectors_r.es = r->x.es;
  selectors_r.ds = r->x.ds;

  /* Use the new dpmi stack */
  pre_stack_mem = dpmi_stack;
  stack_mem = dpmi_stack = mem_get(DPMI_STACK_SIZE);
  dpmi_stack_top = dpmi_stack+DPMI_STACK_SIZE;
  vm86_callBIOS(intnum, &regs_r, &regs_r, &selectors_r);
  /* Restore the previous stack */
  dpmi_stack = pre_stack_mem;
  dpmi_stack_top = dpmi_stack+DPMI_STACK_SIZE;
  mem_free(stack_mem, DPMI_STACK_SIZE);

  r->x.ax = regs_r.x.ax;
  r->x.bx = regs_r.x.bx;
  r->x.cx = regs_r.x.cx;
  r->x.dx = regs_r.x.dx;
  r->x.si = regs_r.x.si;
  r->x.di = regs_r.x.di;
  r->x.flags = regs_r.x.cflag;
  r->x.es = selectors_r.es;
  r->x.ds = selectors_r.ds;
  ll_frestore(f);

  return 0;
}

//#define __RM_INT_DEBUG__

int fd32_real_mode_int(int intnum, DWORD rmcs_address)
{
  union rmregs *r1;
  int res;

  r1 = (union rmregs *)rmcs_address;
  switch (intnum) {
    case 0x10:
      if (use_biosvga)
        res = dpmi_rmint_to_bios(0x10, r1);
      else
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
      dos21_int(r1);
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
        /* CD-ROM v2.00+ - DRIVE CHECK */
        case 0x150B:
          message("Unsupported INT 0x%x EAX: 0x%lx\n", intnum, r1->d.eax);
          break;
        /* MS Windows - WINDOWS ENHANCED MODE INSTALLATION CHECK */
        case 0x1600:
          /* NOTE: Neither Windows 3.x enhanced mode nor Windows/386 2.x running */
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
          fd32_log_printf("[DPMI] OS/2 v2.0+ - Installation Check / Get Version (INT 0x%x EAX: 0x%lx)\n", intnum, r1->d.eax);
          /* NOTE: Do nothing ... */
          break;
        default:
          /* WARNING: PRINT/SHARE/DOS internal to be implemented... */
          message("Unsupported INT 0x%x EAX: 0x%lx\n", intnum, r1->d.eax);
          res = -1;
      }
      return res;

    case 0x33:
      if (use_biosmouse)
        return dpmi_rmint_to_bios(0x33, r1);
      else
        return mousebios_int(r1);

    case 0x4B:
      fd32_log_printf("[DPMI] Virtual DMA Specification (INT 0x%x EAX: 0x%lx)\n", intnum, r1->d.eax);
      /* NOTE: Virtual DMA Specification not supported */
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
