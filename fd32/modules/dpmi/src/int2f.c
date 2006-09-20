/* DPMI Driver for FD32: Multiplex for PM execution
 * by Hanzac Chen
 *
 * This is free software; see GPL.txt
 */

#include <ll/i386/hw-data.h>
#include <ll/i386/hw-arch.h>
#include <ll/i386/x-bios.h>
#include <ll/i386/error.h>

#include <logger.h>
#include "dpmi.h"
#include "rmint.h"
#include "dosexec.h"
#include "ldtmanag.h"

extern int dosidle_int(union rmregs *r);

/* Real-mode emulation */
void int2f_int(union rmregs *r)
{
	switch (r->x.ax) {
		/* CD-ROM - Installation check */
		case 0x1500:
			r->x.bx = 0; /* number of CD-ROM drive letters used */
			/* r->x.cx = 0; starting drive letter (0=A:) */
			break;
		/* CD-ROM v2.00+ - Drive check */
		case 0x150B:
			/* None */
			break;
		/* MS Windows - Windows enhanced mode installation check */
		case 0x1600:
			/* None */
			break;
		/* MS Windows 3.1 - Identify windows version and type */
		case 0x160A:
			/* None */
			break;
		/* MS Windows, DPMI, various - Release current virtual machine time-slice */
		case 0x1680:
			dosidle_int(r); /* TODO: support multitasking here? */
			break;
		/* Windows95 - Title - Set / Get application / Virtual machine title */
		case 0x168E:
			if (r->x.dx == 0x0000 || r->x.dx == 0x0001)
				fd32_log_printf("[DPMI] Windows set title: %s\n", (char *)(r->x.es<<4)+r->x.di);
			else
				fd32_log_printf("[DPMI] Windows get title ...\n");
			r->x.ax = 0x0000; /* failed now */
			break;
		/* MS Windows WINOLDAP - Identify WinOldAp version */
		case 0x1700:
			/* None */
			break;
		/* OS/2 v2.0+ - Installation check / Get version */
		case 0x4010:
			/* None */
			break;
		/* Extended memory specification (XMS) v2+ - Installation check */
		case 0x4300:
			/* None */
			break;
		default:
			message("[DPMI] Unsupported INT 0x2F EAX: 0x%lx\n", r->d.eax);
			break;
	}
}

extern void int_redirect_to_rmint(DWORD intnum, volatile union regs *r);

/* INT 2fh handler for Multiplex for PM execution (AX=function) */
void int2f_handler(union regs *r)
{
#ifdef __DPMI_DEBUG__
	fd32_log_printf("[DPMI] INT 2fh handler, multiplex for pm execution: %x\n", r->x.ax);
#endif
	switch (r->x.ax) {
		/* Obtain Real(vm86)-to-Protected mode switch entry point */
		case 0x1687:
			r->x.ax	 = 0x0000;		/* DPMI installed */
			r->x.dx	 = 0xFD32;		/* DPMI version */
			r->x.bx |= 0x01;		/* 32-bit programs supported */
			r->h.cl	 = cpu.x86_cpu;	/* Processor type >= i386 */
			r->x.si	 = 0x00;		/* Number of paragraphs of DOS extender private data */

			/* Mode-switch entry point */
			r->d.vm86_es = (DWORD)fd32_vm86_to_pmode>>4; /* r->d.ecs; */
			r->x.di	= (DWORD)fd32_vm86_to_pmode&0x0F; /* -(r->d.ecs<<4); */
#ifdef __DPMI_DEBUG__
			fd32_log_printf("[DPMI] VM86 switch: %x ES: %x DI: %x eflags: %x\n", (int)fd32_vm86_to_pmode, (int)r->d.vm86_es, r->x.di, (int)r->d.flags);
#endif
			break;
		/* FD32 specific vm86->pmode switch */
		case 0xFD32:
		{
			DWORD p_vm86_app_stack;
			struct tss *p_vm86_tss = vm86_get_tss(X_VM86_TSS);
			struct psp *ppsp = (struct psp *)(p_vm86_tss->es<<4);
			WORD retf_cs;

#ifdef __DPMI_DEBUG__
			fd32_log_printf("[DPMI] TASK Switch from VM86 (ECS:%x EIP:%x)!\n", (int)r->d.ecs, (int)r->d.eip);
#endif
			/* Create new selectors */
			r->d.ecs = fd32_segment_to_descriptor(r->d.ecs);
			fd32_set_descriptor_access_rights(r->d.ecs, 0x009A);
			/* NOTE: The DS access rights is different with CS */
			r->d.eds = fd32_segment_to_descriptor(r->d.vm86_ds);
			/* NOTE: The CS in the VM86 stack is replaced with the new CS selector */
			/* p_vm86_stack = r->d.esp+sizeof(DWORD)*3; */		/* use the ring0/system(vm86_tss:ss0) stack and skip eip, ecs, flags */
			p_vm86_app_stack = (r->d.vm86_ss<<4)+r->d.vm86_esp;
			retf_cs = fd32_allocate_descriptors(1);
			fd32_set_segment_base_address(retf_cs, ((WORD *)p_vm86_app_stack)[1]<<4);
			fd32_set_segment_limit(retf_cs, 0xFFFF);
			fd32_set_descriptor_access_rights(retf_cs, 0x009A);
			/* r->d.vm86_esp = (((WORD *)p_vm86_app_stack)[0]) | (retf_cs<<16); */
			((WORD *)p_vm86_app_stack)[1] = retf_cs;
			r->d.vm86_ss = fd32_segment_to_descriptor(r->d.vm86_ss);
			/* Using vm86 system stack, not the original vm86 app stack */
			/* NOTE: The ES = selector to program's PSP (100h byte limit) */
			r->d.ees = fd32_segment_to_descriptor(p_vm86_tss->es);
			/* NOTE: The ENV selector is also created */
			ppsp->ps_environ = fd32_segment_to_descriptor(ppsp->ps_environ);
#ifdef __DPMI_DEBUG__
			fd32_log_printf("[DPMI] TASK Switch to PMODE (CS:%lx EIP:%lx)\n", r->d.ecs, r->d.eip);
#endif
			/* Clear the VM flag, NOTE: clear the NT flag (for DOS/32A 9.1.2+ compatible) */
			r->d.flags &= ~(CPU_FLAG_VM|CPU_FLAG_NT);
			/* NOTE: Technique, iret to the targeted CS:IP still in protected mode
					but the space of vm registers saved in the ring0 stack is mostly lost
			*/
			break;
		}
		default:
			int_redirect_to_rmint (0x2f, r);
			break;
	}
}
