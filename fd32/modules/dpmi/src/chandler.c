/* FD32 C handlers for SysCalls & similia
 * by Luca Abeni & Salvo Isaja
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/hw-data.h>
#include <ll/i386/hw-func.h>
#include <ll/i386/x-bios.h>
#include <ll/i386/error.h>
#include <ll/stdlib.h>
#include <ll/string.h>

#include <logger.h>
#include <kernel.h>
#include <kmem.h>

#include "dpmi.h"
#include "rmint.h"
#include "ldtmanag.h"
#include "dos_exec.h"
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

static void return_to_dos(union regs *r)
{
  struct tss * p_vm86_tss = vm86_get_tss();
  WORD bl = p_vm86_tss->back_link;

#ifdef __DPMI_DEBUG__
  fd32_log_printf("[DPMI] Return to DOS: return code 0x%x\n", (BYTE)(r->d.eax & 0xFF));
  fd32_log_printf("Current stack: 0x%lx\n", get_sp());
#endif

  if (bl != NULL) {
    /* Free the system stack */
    mem_free(p_vm86_tss->esp0-VM86_STACK_SIZE+1, VM86_STACK_SIZE);
    /* Task switch to the kernel task */
    p_vm86_tss->back_link = 0;
    ll_context_load(bl);
  } else {
    restore_sp(r->d.eax & 0xFF);
  }
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
        break;

      /* DPMI 0.9 service 0001h - Free LDT Descriptor */
      case 0x0001:
        int31_0001(&r);
        break;

      /* DPMI 0.9 service 0002h - Segment to Descriptor */
      case 0x0002:
        int31_0002(&r);
        break;

      /* DPMI 0.9 service 0003h - Get Selector Increment Value */
      case 0x0003:
        int31_0003(&r);
        break;

      /* DPMI 0.9 services 0004h and 0005h are reserved for historical
       * reasons and should not be called
       */

      /* DPMI 0.9 service 0006h - Get Segment Base Address */
      case 0x0006:
        int31_0006(&r);
        break;

      /* DPMI 0.9 service 0007h - Set Segment Base Address */
      case 0x0007:
        int31_0007(&r);
        break;

      /* DPMI 0.9 service 0008h - Set Segment Limit */
      case 0x0008:
        int31_0008(&r);
        break;

      /* DPMI 0.9 service 0009h - Set Descriptor Access Rights */
      case 0x0009:
        int31_0009(&r);
        break;
	
      /* DPMI 0.9 service 000Ah - Create Alias Descriptor */
      case 0x000A:
        int31_000A(&r);
        break;

      /* DPMI 0.9 service 000Bh - Get Descriptor */
      case 0x000B:
        int31_000B(&r);
        break;

      /* DPMI 0.9 service 000Ch - Set Descriptor */
      case 0x000C:
        int31_000C(&r);
        break;

      case 0x0100:
        int31_0100(&r);
        break;

      case 0x0101:
        int31_0101(&r);
        break;

      case 0x0200:
        int31_0200(&r);
        break;

      case 0x0201:
        int31_0201(&r);
        break;

      case 0x0202:
        int31_0202(&r);
        break;

      case 0x0203:
        int31_0203(&r);
        break;

      case 0x0204:
        int31_0204(&r);
        break;

      case 0x0205:
        int31_0205(&r);
        break;
	
      case 0x0300:
        int31_0300(&r);
        break;

      case 0x0303:
        int31_0303(&r);
        break;

      case 0x0304:
        int31_0303(&r);
        break;

      case 0x0400:
        int31_0400(&r);
        break;
	
      case 0x0401:
        int31_0401(&r);
        break;
	
      case 0x0501:
        int31_0501(&r);
        break;

      case 0x0502:
        int31_0502(&r);
        break;

      case 0x0507:
        int31_0507(&r);
        break;

      case 0x0600:
        int31_0600(&r);
        break;

      case 0x0900:
        int31_0900(&r);
        break;

      case 0x0901:
        int31_0901(&r);
        break;

      case 0x0E01:
        int31_0E01(&r);
        break;

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
        break;
      }
      break;
    case 0x21:
      if ((r.d.eax & 0xFF00) == 0x4C00) {
        return_to_dos(&r);
        /* Should not arrive here... */
      } else {
        /* Redirect to call RM interrupts' handler */
        extern void int21_handler(union rmregs *r);
        /* TODO: Remove the static rmregs */
        union rmregs r1;
        r1.d.edi = r.d.edi;
        r1.d.esi = r.d.esi;
        r1.d.ebp = r.d.ebp;
        r1.d.Res = 0;
        r1.d.ebx = r.d.ebx;
        r1.d.edx = r.d.edx;
        r1.d.ecx = r.d.ecx;
        r1.d.eax = r.d.eax;
        r1.x.flags = r.x.flags;
        r1.x.es = r.d.vm86_es;
        r1.x.ds = r.d.vm86_ds;
        r1.x.fs = r.d.vm86_fs;
        r1.x.gs = r.d.vm86_gs;
        r1.x.cs = r.d.ecs;
        r1.x.ss = r.d.vm86_ss;
        int21_handler(&r1);
        r.d.edi = r1.d.edi;
        r.d.esi = r1.d.esi;
        r.d.ebp = r1.d.ebp;
        r.d.ebx = r1.d.ebx;
        r.d.edx = r1.d.edx;
        r.d.ecx = r1.d.ecx;
        r.d.eax = r1.d.eax;
        r.x.flags = r1.x.flags;
      }
      break;
    /* Multiplex for PM execution, wrote by Hanzac Chen */
    case 0x2F:
      if (r.x.ax == 0x1687) {
        r.x.ax  = 0x0000; /* DPMI installed */
        r.x.dx  = 0xFD32; /* DPMI version */
        r.x.bx |= 0x01;   /* 32-bit programs supported */
        r.h.cl  = 0x03;   /* Processor type i386 */
        r.x.si  = 0x00;   /* Number of paragraphs of DOS extender private data */

        /* Mode-switch entry point */
        r.d.vm86_es = (DWORD)dos_exec_mode16>>4; /* r.d.ecs; */
        r.x.di  = (DWORD)dos_exec_mode16&0x0F; /* -(r.d.ecs<<4); */
      #ifdef __DPMI_DEBUG__
        fd32_log_printf("[DPMI] VM86 switch: %x ES: %x DI: %x eflags: %x\n", (int)dos_exec_mode16, (int)r.d.vm86_es, r.x.di, (int)r.d.flags);
      #endif
      } else if (r.x.ax == 0xFD32) {
        DWORD p_vm86_stack, p_vm86_app_stack;
        struct tss *p_vm86_tss = vm86_get_tss();
        struct psp *ppsp = (struct psp *)(p_vm86_tss->es<<4);
        WORD retf_cs;

      #ifdef __DPMI_DEBUG__
        fd32_log_printf("[DPMI] TASK Switch from VM86 (ECS:%x EIP:%x)!\n", (int)r.d.ecs, (int)r.d.eip);
      #endif
        /* Create new selectors */
        r.d.ecs = fd32_segment_to_descriptor(r.d.ecs);
        fd32_set_descriptor_access_rights(r.d.ecs, 0x009A);
        /* NOTE: The DS access rights is different with CS */
        r.d.eds = fd32_segment_to_descriptor(r.d.vm86_ds);
        /* NOTE: The CS in the VM86 stack is replaced with the new CS selector */
        p_vm86_stack = r.d.esp+sizeof(DWORD)*3;    /* use the ring0/system(vm86_tss:ss0) stack and skip eip, ecs, flags */
        p_vm86_app_stack = (r.d.vm86_ss<<4)+r.d.vm86_esp;
        retf_cs = fd32_allocate_descriptors(1);
        fd32_set_segment_base_address(retf_cs, ((WORD *)p_vm86_app_stack)[1]<<4);
        fd32_set_segment_limit(retf_cs, 0xFFFF);
        fd32_set_descriptor_access_rights(retf_cs, 0x009A);
        ((WORD *)p_vm86_stack)[0] = ((WORD *)p_vm86_app_stack)[0];
        ((WORD *)p_vm86_stack)[1] = retf_cs;
        /* NOTE: The ES = selector to program's PSP (100h byte limit) */
        r.d.ees = fd32_segment_to_descriptor(p_vm86_tss->es);
        /* NOTE: The ENV selector is also created */
        ppsp->ps_environ = fd32_segment_to_descriptor(ppsp->ps_environ);
      #ifdef __DPMI_DEBUG__
        fd32_log_printf("[DPMI] After Switch (CS:%x EIP:%x)\n", r.d.ecs, r.d.eip);
      #endif
        r.d.flags &= 0xFFFDFFFF; /* Clear the VM flag */
        /* NOTE: Technique, iret to the targeted CS:IP still in protected mode
                 but the space of vm registers saved in the ring0 stack is mostly lost
        */
      }
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

