/* FD32 C handlers for SysCalls & similia
 * by Luca Abeni & Salvo Isaja & Hanzac Chen
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/hw-data.h>
#include <ll/i386/hw-func.h>
#include <ll/i386/x-bios.h>
#include <ll/i386/error.h>
#include <ll/stdlib.h>

#include <logger.h>
#include <exec.h>
#include <kmem.h>
#include <kernel.h>

#include "dpmi.h"
#include "dpmimem.h"
#include "rmint.h"
#include "int31_00.h"
#include "int31_01.h"
#include "int31_02.h"
#include "int31_03.h"
#include "int31_04.h"
#include "int31_05.h"
#include "int31_06.h"
#include "int31_09.h"
#include "int31_0e.h"

#define LETSSTOP 10

/* #define __DPMI_DEBUG__ */


int stop = -1;
#ifdef ENABLE_BIOSVGA
char use_biosvga;
#endif
char use_biosmouse;

static void chandler1(DWORD eax, DWORD ebx, DWORD ecx, DWORD edx, DWORD intnum)
{
  message("[DPMI] INT 0x%lx called\n", intnum);
  message("eax = 0x%lx\t", eax);
  message("ebx = 0x%lx\t", ebx);
  message("ecx = 0x%lx\t", ecx);
  message("edx = 0x%lx\n", edx);
}

static void return_to_dos(union regs *r, int resident)
{
  struct tss * p_vm86_tss = vm86_get_tss(X_VM86_TSS);
  WORD bl = p_vm86_tss->back_link;

#ifdef __DPMI_DEBUG__
  fd32_log_printf("[DPMI] Return to DOS: return code 0x%x\n", (BYTE)(r->d.eax & 0xFF));
  fd32_log_printf("Current stack: 0x%lx\n", get_sp());
#endif

  if (resident)
    fd32_get_current_pi()->type |= RESIDENT;

  if (bl != NULL) {
    /* Free all the memory of the program (e.g. dos/32a) */
    dpmi_clear_mem(fd32_get_current_pi()->mem_info);
    /* No need to free the static memory system-stack
     * and Task switch back to the kernel task
     */
    p_vm86_tss->back_link = 0;
    ll_context_load(bl);
  } else {
    fd32_get_current_pi()->_exit(r->d.eax & 0xFF);
  }
}

void int_redirect_to_rmint(DWORD intnum, volatile union regs *r)
{
  union rmregs r1;

  r1.d.edi = r->d.edi;
  r1.d.esi = r->d.esi;
  r1.d.ebp = r->d.ebp;
  r1.d.Res = 0;
  r1.d.ebx = r->d.ebx;
  r1.d.edx = r->d.edx;
  r1.d.ecx = r->d.ecx;
  r1.d.eax = r->d.eax;
  r1.x.flags = r->x.flags;
  if (r->d.flags&CPU_FLAG_VM) {
    r1.x.es = r->d.vm86_es;
    r1.x.ds = r->d.vm86_ds;
  }
  fd32_real_mode_int(intnum, (DWORD)&r1);
  r->d.edi = r1.d.edi;
  r->d.esi = r1.d.esi;
  r->d.ebp = r1.d.ebp;
  r->d.ebx = r1.d.ebx;
  r->d.edx = r1.d.edx;
  r->d.ecx = r1.d.ecx;
  r->d.eax = r1.d.eax;
  r->x.flags = r1.x.flags;
  if (r->d.flags&CPU_FLAG_VM) {
    r->d.vm86_es = r1.x.es;
    r->d.vm86_ds = r1.x.ds;
  }
}

void int_redirect_to_bios(DWORD intnum, volatile union regs *r)
{
  DWORD f;
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
  if (r->d.flags&CPU_FLAG_VM) {
    selectors_r.es = r->d.vm86_es;
    selectors_r.ds = r->d.vm86_ds;
  }

  vm86_callBIOS(intnum, &regs_r, &regs_r, &selectors_r);

  r->x.ax = regs_r.x.ax;
  r->x.bx = regs_r.x.bx;
  r->x.cx = regs_r.x.cx;
  r->x.dx = regs_r.x.dx;
  r->x.si = regs_r.x.si;
  r->x.di = regs_r.x.di;
  r->x.flags = regs_r.x.cflag;
  if (r->d.flags&CPU_FLAG_VM) {
    r->d.vm86_es = selectors_r.es;
    r->d.vm86_ds = selectors_r.ds;
  }
  ll_frestore(f);
}

void int21_handler(union regs *r)
{
  if ((r->x.ax & 0xFF00) == 0x4C00) {
  	/* Return to DOS */
    return_to_dos(r, 0);
  } else if ((r->x.ax & 0xFF00) == 0x3100) {
  	/* Terminate and stay resident */
    return_to_dos(r, 1);
  } else if (r->x.ax == 0x4B82) {
    fd32_log_printf("INT21 0x4B82 %lx\n", r->d.edx);
  } else {
    /* Redirect to call RM interrupts' handler */
    int_redirect_to_rmint(0x21, r);
  }
}

/* Int 31h handler may better be put somewhere else */
void int31_handler(union regs *r)
{
  switch (r->x.ax) {
    /* DPMI 0.9 service 0000h - Allocate LDT Descriptors */
    case 0x0000 :
      int31_0000(r);
      break;
  
    /* DPMI 0.9 service 0001h - Free LDT Descriptor */
    case 0x0001:
      int31_0001(r);
      break;
  
    /* DPMI 0.9 service 0002h - Segment to Descriptor */
    case 0x0002:
      int31_0002(r);
      break;
  
    /* DPMI 0.9 service 0003h - Get Selector Increment Value */
    case 0x0003:
      int31_0003(r);
      break;
  
    /* DPMI 0.9 services 0004h and 0005h are reserved for historical
     * reasons and should not be called
     */
  
    /* DPMI 0.9 service 0006h - Get Segment Base Address */
    case 0x0006:
      int31_0006(r);
      break;
  
    /* DPMI 0.9 service 0007h - Set Segment Base Address */
    case 0x0007:
      int31_0007(r);
      break;
  
    /* DPMI 0.9 service 0008h - Set Segment Limit */
    case 0x0008:
      int31_0008(r);
      break;
  
    /* DPMI 0.9 service 0009h - Set Descriptor Access Rights */
    case 0x0009:
      int31_0009(r);
      break;
  
    /* DPMI 0.9 service 000Ah - Create Alias Descriptor */
    case 0x000A:
      int31_000A(r);
      break;
  
    /* DPMI 0.9 service 000Bh - Get Descriptor */
    case 0x000B:
      int31_000B(r);
      break;
  
    /* DPMI 0.9 service 000Ch - Set Descriptor */
    case 0x000C:
      int31_000C(r);
      break;
  
    case 0x0100:
      int31_0100(r);
      break;
  
    case 0x0101:
      int31_0101(r);
      break;
  
    case 0x0200:
      int31_0200(r);
      break;
  
    case 0x0201:
      int31_0201(r);
      break;
  
    case 0x0202:
      int31_0202(r);
      break;
  
    case 0x0203:
      int31_0203(r);
      break;
  
    case 0x0204:
      int31_0204(r);
      break;
  
    case 0x0205:
      int31_0205(r);
      break;
  
    case 0x0300:
      int31_0300(r);
      break;
  
    case 0x0303:
      int31_0303(r);
      break;
  
    case 0x0304:
      int31_0304(r);
      break;
  
    case 0x0400:
      int31_0400(r);
      break;
  
    case 0x0401:
      int31_0401(r);
      break;
  
    case 0x0501:
      int31_0501(r);
      break;
  
    case 0x0502:
      int31_0502(r);
      break;
  
    case 0x0503:
      int31_0503(r);
      break;
  
    case 0x0507:
      int31_0507(r);
      break;
  
    case 0x0600:
      int31_0600(r);
      break;

    /* DPMI 0.9 service 0800h - Physical Address Mapping */
    case 0x0800:
      /* Do nothing ... */
      break;

    /* DPMI 1.0 service 0801h - Free Physical Address Mapping */
    case 0x0801:
      /* Do nothing ... */
      break;

    case 0x0900:
      int31_0900(r);
      break;
  
    case 0x0901:
      int31_0901(r);
      break;
  
    case 0x0E01:
      int31_0E01(r);
      break;
  
    default: 
      r->d.flags |= 0x01;
      error("INT 31 Service not implemented\n");
      message("Service number: 0x%x\n", r->x.ax);
      fd32_log_printf("INT 31, Service 0x%x not implemented\n", r->x.ax);
      fd32_log_printf("eax = 0x%lx\t", r->d.eax);
      fd32_log_printf("ebx = 0x%lx\t", r->d.ebx);
      fd32_log_printf("ecx = 0x%lx\t", r->d.ecx);
      fd32_log_printf("edx = 0x%lx\n", r->d.edx);
      fd32_log_printf("esi = 0x%lx\t", r->d.esi);
      fd32_log_printf("edi = 0x%lx\t", r->d.edi);
      fd32_log_printf("ees = 0x%lx\n", r->d.ees);
      fd32_abort();
      break;
  }
}

void dpmi_chandler(DWORD intnum, union regs r)
{
  /* DWORD eax, ebx, ecx, edx, esi, edi, ees; */
#ifdef __DPMI_DEBUG__
  fd32_log_printf("DPMI chandler, INT: %x EAX: %x EBX: %x eflags: %x\n", intnum, r.d.eax, r.d.ebx, r.d.flags);
#endif

  if (intnum != 0x2f)
    sti();

  if (stop != -1) {
    stop++;
  }
  if (stop == LETSSTOP) {
    fd32_log_printf("[DPMI Debug]: stop @ INT 0x%lx Service 0x%lx\n",
                    intnum, r.d.eax & 0xFFFF);
    fd32_log_printf("eax = 0x%lx\t", r.d.eax);
    fd32_log_printf("ebx = 0x%lx\t", r.d.ebx);
    fd32_log_printf("ecx = 0x%lx\t", r.d.ecx);
    fd32_log_printf("edx = 0x%lx\n", r.d.edx);
    fd32_log_printf("esi = 0x%lx\t", r.d.esi);
    fd32_log_printf("edi = 0x%lx\t", r.d.edi);
    fd32_log_printf("ees = 0x%lx\n", r.d.ees);

    fd32_abort();
  }

  /* Clear carry flag first */
  r.d.flags &= 0xFFFFFFFE;

  switch (intnum) {
    case 0x10:
#ifdef ENABLE_BIOSVGA
      if (use_biosvga)
        int_redirect_to_bios(0x10, &r);
      else
#endif
        int_redirect_to_rmint(0x10, &r);
      break;
    /* Get BIOS equipment list */
    case 0x11:
      int_redirect_to_bios(0x11, &r);
      break;
    /* PS BIOS interface */
    case 0x15:
      int_redirect_to_bios(0x15, &r);
      break;
    case 0x16:
      int_redirect_to_rmint(0x16, &r);
      break;
    /* PCI BIOS services */
    case 0x1A:
      int_redirect_to_bios(0x1A, &r);
      break;
    case 0x21:
      int21_handler(&r);
      break;
    case 0x25:
      int_redirect_to_rmint(0x25, &r);
      break;
    /* Network services */
    case 0x2A:
      if (r.x.ax == 0x8400)
        int_redirect_to_rmint(0x28, &r); /* DOS idle */
      else
        fd32_log_printf("INT 2A, Network service 0x%x not implemented\n", r.x.ax);
      break;
    /* Multiplex for PM execution */
    case 0x2F:
      int2f_handler(&r);
      break;
    /* NOTE: The actual DOS Protected Mode Interface, based on interrupts,
     *       is performed by this handler, that simply translates the
     *       DPMI calls into FD32 function calls.
     */
    case 0x31 : 
      int31_handler(&r);
      break;
    case 0x33:
      int_redirect_to_rmint(0x33, &r);
      break;
    default:
      chandler1(r.d.eax, r.d.ebx, r.d.ecx, r.d.edx, intnum);
      r.d.flags |= 0x01;
      message("esi = 0x%lx\t", r.d.esi);
      message("edi = 0x%lx\t", r.d.edi);
      message("ees = 0x%lx\n", r.d.ees);
      fd32_abort();
  }
}

