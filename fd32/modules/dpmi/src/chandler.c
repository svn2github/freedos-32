/* FD32 C handlers for SysCalls & similia
 * by Luca Abeni & Salvo Isaja
 * 
 * This is free software; see GPL.txt
 */

#include<ll/i386/hw-data.h>
#include<ll/i386/hw-instr.h>
#include<ll/i386/error.h>
#include<ll/i386/cons.h>

#include "logger.h"
#include "kernel.h"

#include "dpmi.h"
#include "rmint.h"
#include "int31_00.h"
#include "int31_02.h"
#include "int31_03.h"
#include "int31_04.h"
#include "int31_05.h"
#include "int31_06.h"
#include "int31_09.h"
#include "int31_0e.h"

#define LETSSTOP 10

/*
#define __DPMI_DEBUG__
*/

int stop = -1;

void chandler1(DWORD eax, DWORD ebx, DWORD ecx, DWORD edx, DWORD intnum)
{
  message("[DPMI] INT 0x%lx called\n", intnum);
  message("eax = 0x%lx\t", eax);
  message("ebx = 0x%lx\t", ebx);
  message("ecx = 0x%lx\t", ecx);
  message("edx = 0x%lx\n", edx);
}

void return_to_dos(union regs *r)
{
  void restore_sp(int res);

#ifdef __DPMI_DEBUG__
  fd32_log_printf("[DPMI] Return to DOS: return code 0x%x\n", (BYTE)(r->d.eax & 0xFF));
  fd32_log_printf("Current stack: 0x%lx\n", get_sp());
#endif
  restore_sp(r->d.eax & 0xFF);
}


void chandler(DWORD intnum, union regs r)
{
  /* DWORD eax, ebx, ecx, edx, esi, edi, ees; */

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

  switch (intnum) {
    /* FIX ME: Int 31h handler may be better to stay outside this routine
     *
     * NOTE: The actual DOS Protected Mode Interface, based on interrupts,
     *       is performed by this handler, that simply translates the
     *       DPMI calls into FD32 function calls.
     */
    case 0x31 : switch (r.d.eax & 0xFFFF) {
      /* DPMI 0.9 service 0000h - Allocate LDT Descriptors */
      case 0x0000 :
        int31_0000(&r);
        return;

      /* DPMI 0.9 service 0001h - Free LDT Descriptor */
      case 0x0001:
        int31_0001(&r);
	return;

      /* DPMI 0.9 service 0002h - Segment to Descriptor */
      case 0x0002:
        int31_0002(&r);
	return;

      /* DPMI 0.9 service 0003h - Get Selector Increment Value */
      case 0x0003:
        int31_0003(&r);
	return;

      /* DPMI 0.9 services 0004h and 0005h are reserved for historical
       * reasons and should not be called
       */

      /* DPMI 0.9 service 0006h - Get Segment Base Address */
      case 0x0006:
        int31_0006(&r);
        return;

      /* DPMI 0.9 service 0007h - Set Segment Base Address */
      case 0x0007:
        int31_0007(&r);
        return;

      /* DPMI 0.9 service 0008h - Set Segment Limit */
      case 0x0008:
        int31_0008(&r);
        return;

      /* DPMI 0.9 service 0009h - Set Descriptor Access Rights */
      case 0x0009:
        int31_0009(&r);
        return;
	
      /* DPMI 0.9 service 000Ah - Create Alias Descriptor */
      case 0x000A:
        int31_000A(&r);
	return;

      /* DPMI 0.9 service 000Bh - Get Descriptor */
      case 0x000B:
        int31_000B(&r);
	return;

      /* DPMI 0.9 service 000Ch - Set Descriptor */
      case 0x000C:
        int31_000C(&r);
	return;

      case 0x0507:
	int31_0507(&r);
	return;

      case 0x0200:
        int31_0200(&r);
	return;

      case 0x0201:
        int31_0201(&r);
	return;

      case 0x0202:
        int31_0202(&r);
	return;

      case 0x0203:
        int31_0203(&r);
	return;

      case 0x0204:
        int31_0204(&r);
	return;

      case 0x0205:
        int31_0205(&r);
	return;
	
      case 0x0300:
        int31_0300(&r);
	return;

      case 0x0303:
        int31_0303(&r);
	return;
      case 0x0304:
        int31_0303(&r);
	return;

      case 0x0400:
	int31_0400(&r);
	return;
	
      case 0x0401:
	int31_0401(&r);
	return;
	
      case 0x0501:
	int31_0501(&r);
	return;
      case 0x0502:
	int31_0502(&r);
	return;

      case 0x0600:
	int31_0600(&r);
	return;

      case 0x0900:
        int31_0900(&r);
        return;

      case 0x0901:
        int31_0901(&r);
        return;

      case 0x0E01:
	int31_0E01(&r);
	return;

      default: 
	r.d.flags |= 0x01;
	error("INT 31 Service not implemented\n");
	message("Service number: 0x%lx\n", r.d.eax & 0xFFFF);
	fd32_log_printf("INT 31, Service 0x%lx not implemented\n",
			r.d.eax & 0xFFFF);
	fd32_log_printf("eax = 0x%lx\t", r.d.eax);
	fd32_log_printf("ebx = 0x%lx\t", r.d.ebx);
	fd32_log_printf("ecx = 0x%lx\t", r.d.ecx);
	fd32_log_printf("edx = 0x%lx\n", r.d.edx);
	fd32_log_printf("esi = 0x%lx\t", r.d.esi);
	fd32_log_printf("edi = 0x%lx\t", r.d.edi);
	fd32_log_printf("ees = 0x%lx\n", r.d.ees);
      if ((r.d.eax & 0xFFFF) != 0x507) {
        fd32_abort();
      }
      return;
    }
    case 0x21:
      if ((r.d.eax & 0xFF00) == 0x4C00) {
        return_to_dos(&r);
        /* Should not arrive here... */
      } else {
        /* TODO: Check if correct to call RM interrupts' handler? */
        extern void int21_handler(union rmregs *r);
        union rmregs r1 = { {r.d.edi, r.d.esi, r.d.ebp, 0, r.d.ebx, r.d.edx, r.d.ecx, r.d.eax} };
        int21_handler(&r1);
        return;
      }
  }
  chandler1(r.d.eax, r.d.ebx, r.d.ecx, r.d.edx, intnum);
  r.d.flags |= 0x01;
  message("esi = 0x%lx\t", r.d.esi);
  message("edi = 0x%lx\t", r.d.edi);
  message("ees = 0x%lx\n", r.d.ees);
  fd32_abort();
}

