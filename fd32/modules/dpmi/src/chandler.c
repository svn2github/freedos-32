/* FD32 C handlers for SysCalls & similia
 * by Luca Abeni & Salvo Isaja
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/hw-data.h>
#include <ll/i386/hw-func.h>
#include <ll/i386/hw-instr.h>
#include <ll/i386/x-bios.h>
#include <ll/i386/error.h>
#include <ll/i386/cons.h>
#include <ll/stdlib.h>
#include <ll/string.h>

#include <kmem.h>
#include <logger.h>
#include <kernel.h>

#include "dpmi.h"
#include "rmint.h"
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

void return_to_dos(union regs *r)
{
  struct tss * p_vm86_tss = vm86_get_tss();
  WORD bl = p_vm86_tss->back_link;
  void restore_sp(int res);

#ifdef __DPMI_DEBUG__
  fd32_log_printf("[DPMI] Return to DOS: return code 0x%x\n", (BYTE)(r->d.eax & 0xFF));
  fd32_log_printf("Current stack: 0x%lx\n", get_sp());
#endif

  if (bl != NULL) {
    p_vm86_tss->back_link = 0;
    ll_context_load(bl);
  } else {
    restore_sp(r->d.eax & 0xFF);
  }
}


static void gdt_place2(WORD sel,DWORD base,DWORD lim,BYTE acc,BYTE gran)
{
    union gdt_entry x;
    /* This is declared in [wc32/gnu]\x0.[asm/s] */
    extern LIN_ADDR GDT_base;

    x.d.base_lo = (base & 0x0000FFFF);
    x.d.base_med = ((base & 0x00FF0000) >> 16);
    x.d.base_hi = ((base & 0xFF000000) >> 24);
    x.d.access = acc;
    x.d.lim_lo = (lim & 0xFFFF);
    x.d.gran = (gran | ((lim >> 16) & 0x0F));    
    memcpy(GDT_base+(sel & ~3), &x, sizeof(union gdt_entry));
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
        static union rmregs r1;
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
    /* Multiplex for PM execution */
    case 0x2F:
      if (r.x.ax == 0x1687) {
        BYTE *p;
        r.x.ax  = 0x0000; /* DPMI installed */
        r.x.dx  = 0xFD32; /* DPMI version */
        r.x.bx |= 0x01;   /* 32-bit programs supported */
        r.h.cl  = 0x03;   /* Processor type i386 */
        r.x.si  = 0x00;   /* Number of paragraphs of DOS extender private data */
        
        p = (BYTE *)dosmem_get(0x10);

        p[0] = 0xB8, p[1] = 0x32, p[2] = 0xFD;
        p[3] = 0xCD, p[4] = 0x2F, p[5] = 0xC3; /* mov ax, 0xfd32; int 0x2f; ret; */

        /* TODO: Mode switch, move dosmem_get and  the code out, put them in the dos_exec */
        r.d.vm86_es = r.d.ecs;
        r.x.di  = (DWORD)p-(r.d.ecs<<4);
      #ifdef __DPMI_DEBUG__
        fd32_log_printf("[DPMI] VM86 switch: %x ES: %x DI: %x eflags: %x\n", (int)p, (int)r.d.vm86_es, r.x.di, (int)r.d.flags);
      #endif
      } else if (r.x.ax == 0xFD32) {
        DWORD i, done, p_vm86_stack;
        struct tss * p_vm86_tss = vm86_get_tss();
        WORD sel, bl = p_vm86_tss->back_link;
        struct tss * p_k_tss = (struct tss *)gdt_read(bl, NULL, NULL, NULL);
        BYTE acc;
      #ifdef __DPMI_DEBUG__
        fd32_log_printf("[DPMI] TASK Switch from VM86 (ECS:%x EIP:%x)!\n", (int)r.d.ecs, (int)r.d.eip);
      #endif
        /* Create new selector */
        sel = 8;
        
        for (i = 0, done = 0; i < 5; i++, done = 0) {
          while (sel < 256 && (!done)) {
            gdt_read(sel, NULL, &acc, NULL);
            if (acc == 0) {
              done = 1;
            } else {
              sel += 8;
            }
          }
          #define RUN_RING 0
          /* TODO: Choose a better segment limit */
          if (i == 0) {
            gdt_place2(sel, r.d.ecs<<4, 0xF0000, 0x9A|(RUN_RING<<5), 0x00);
            p_k_tss->cs = sel;
            p_k_tss->eip = r.d.eip;
          } else if (i == 1) {
            gdt_place2(sel, r.d.vm86_ss<<4, 0xF0000, 0x92|(RUN_RING<<5), 0x00);
            /* Set the privilege level-0 stack = the kernel stack */
            p_k_tss->ss0 = p_k_tss->ss;
            p_k_tss->esp0 = p_k_tss->esp;
            /* Switch to the APP's stack */
            p_k_tss->ss = sel;
            p_k_tss->esp = r.d.vm86_esp-2;
            /* NOTE: The CS in the VM86 stack is replaced with the new CS selector */
            p_vm86_stack = (r.d.vm86_ss<<4)+r.d.vm86_esp-2;
            ((DWORD *)p_vm86_stack)[0] = ((WORD *)p_vm86_stack)[1];
            ((WORD *)p_vm86_stack)[2] = p_k_tss->cs;
          } else if (i == 2) {
            gdt_place2(sel, r.d.vm86_ds<<4, 0xF0000, 0x92|(RUN_RING<<5), 0x00);
            p_k_tss->ds = sel;
          } else if (i == 3) {
            /* NOTE: The ES = selector to program's PSP (100h byte limit) */
            gdt_place2(sel, p_vm86_tss->es<<4, 0xF0000, 0x92|(RUN_RING<<5), 0x00);
            p_k_tss->es = sel;
          } else if (i == 4) {
            struct psp *ppsp = (struct psp *)(p_vm86_tss->es<<4);
            /* NOTE: The ENV selector is also created */
            gdt_place2(sel, ppsp->ps_environ<<4, 0xF0000, 0x92|(RUN_RING<<5), 0x00);
            ppsp->ps_environ = sel;
          }
        }
      #ifdef __DPMI_DEBUG__
        fd32_log_printf("[DPMI] After Switch (CS:%x EIP:%x)\n", p_k_tss->cs, p_k_tss->eip);
      #endif
        /* Restore the registers' status */
        p_k_tss->eflags = (p_k_tss->eflags&0xFFFF0000)|(r.d.flags&0x0000FFFF);
        p_k_tss->eax = r.d.eax;
        p_k_tss->ecx = r.d.ecx;
        p_k_tss->edx = r.d.edx;
        p_k_tss->ebx = r.d.ebx;
        p_k_tss->ebp = r.d.ebp;
        p_k_tss->esi = r.d.esi;
        p_k_tss->edi = r.d.edi;
        p_vm86_tss->back_link = 0;
        ll_context_load(bl);
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

