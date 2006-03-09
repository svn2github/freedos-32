/* Project:     OSLib
 * Description: The OS Construction Kit
 * Date:                1.6.2000
 * Idea by:             Luca Abeni & Gerardo Lamastra
 *
 * OSLib is an SO project aimed at developing a common, easy-to-use
 * low-level infrastructure for developing OS kernels and Embedded
 * Applications; it partially derives from the HARTIK project but it
 * currently is independently developed.
 *
 * OSLib is distributed under GPL License, and some of its code has
 * been derived from the Linux kernel source; also some important
 * ideas come from studying the DJGPP go32 extender.
 *
 * We acknowledge the Linux Community, Free Software Foundation,
 * D.J. Delorie and all the other developers who believe in the
 * freedom of software and ideas.
 *
 * For legalese, check out the included GPL license.
 */

/*	As the name says... All the hardware-dependent data structures... */

#ifndef __LL_I386_HW_DATA_H__
#define __LL_I386_HW_DATA_H__

#include <ll/i386/defs.h>

/* DO WE NEED A SEPARATE INCL FILE FOR THIS? */
#if defined(__GNU__)
#define __LL_ARCH__ "32/DJGPP C/COFF"
#elif defined(__LINUX__)
#define __LL_ARCH__ "32/LINUX CrossCompiled/ELF"
#elif defined(__WIN32__)
#define __LL_ARCH__ "32/Win32"
#else
#error Architecture Undefined!
#endif

#include <ll/i386/sel.h>

BEGIN_DEF

/* Useful basic types */

#ifndef __BASIC_DATA__
    #define __BASIC_DATA__
    typedef void *LIN_ADDR;
    typedef unsigned long long  QWORD;
    typedef unsigned long  DWORD;
    typedef unsigned short WORD;
    typedef unsigned char  BYTE;
    typedef unsigned long  TIME;
    typedef unsigned long  SYS_FLAGS;
    #define TRUE	1
    #define FALSE	0
    #define MAX_DWORD	0xFFFFFFFF
    #define MAX_WORD	0xFFFF
    #define MAX_BYTE	0xFF
    #define MAX_TIME	MAX_DWORD
#endif

#define CONTEXT unsigned short int

/* Hardware based types (Self explanatory) */

struct __attribute__ ((packed)) gate {
    WORD offset_lo;
    WORD sel;
    BYTE dword_cnt;
    BYTE access;
    WORD offset_hi;
};

struct __attribute__ ((packed)) descriptor {
    WORD lim_lo;
    WORD base_lo;
    BYTE base_med;
    BYTE access;
    BYTE gran;
    BYTE base_hi;
};

/* A LDT/GDT entry could be a gate or a selector */
/* An IDT entry could be a gate only	     	 */

union __attribute__ ((packed)) gdt_entry {
    struct descriptor d;
    struct gate g;
};

struct registers {
    DWORD dummy1;
    DWORD dummy2;
    DWORD egs;
    DWORD efs;
    DWORD ees;
    DWORD ess;
    DWORD eds;
    DWORD edi;
    DWORD esi;
    DWORD ebp;
    DWORD esp;
    DWORD ebx;
    DWORD edx;
    DWORD ecx;
    DWORD eax;
    DWORD eip;
    DWORD ecs;
    DWORD flags;
    DWORD vm86_esp;
    DWORD vm86_ss;
    DWORD vm86_es;
    DWORD vm86_ds;
    DWORD vm86_fs;
    DWORD vm86_gs;
};

#define STACK_ACCESS	0x92	/* Basic Access bytes */
#define DATA_ACCESS	0x92
#define CODE_ACCESS	0x9A

/* At this level we just need to set up 2 gates to enter/exit PM */
/* The entry gate is a 386 32 bit call gate			 */
/* The exit (Back To Real Mode) gate is a 286 16 bit gate	 */

#define CALL_GATE286	0x84	/* Call & Int Gate Access bytes */
#define CALL_GATE386	0x8C	
#define TASK_GATE	0x85
#define INT_GATE286	0x86
#define INT_GATE386	0x8E
#define TRAP_GATE286	0x87
#define TRAP_GATE386	0x8F

/* TSS selectors */

#define FREE_TSS386	0x89
#define BUSY_TSS386	0x8B
#define FREE_TSS286	0x81
#define BUSY_TSS286	0x83

#define GRAN_32B	0xC0	/* Granularity settings */
#define GRAN_32		0x40
#define GRAN_16		0x00

/* This is the TSS image for a 386 hardware task 			*/
/* I added two other fields to the basic structure:			*/
/* 1) The CONTROL field which is used by system software to detect	*/
/*    particular conditions; in this the first phase it is mainly used  */
/*    to mark the unused TSS & TSS which use math, altough thanks to 	*/
/*    the automatic FPU preemption supported in 386 this would be not   */
/*    necessary.							*/
/* 2) The ctx_fpu field used to store the FPU context if necessaary	*/

#define TSS_USED		0x8000
#define FPU_USED                0x4000

#define FPU_CONTEXT_SIZE	108

/* CPU flags definitions */
#define CPU_FLAG_TF	0x00000100
#define CPU_FLAG_IF	0x00000200
#define CPU_FLAG_IOPL	0x00003000
#define CPU_FLAG_NT	0x00004000
#define CPU_FLAG_VM	0x00020000
#define CPU_FLAG_AC	0x00040000
#define CPU_FLAG_VIF	0x00080000
#define CPU_FLAG_VIP	0x00100000
#define CPU_FLAG_ID	0x00200000


struct __attribute__ ((packed)) tss {
    WORD back_link;
    WORD _fill0;
    DWORD esp0;
    WORD ss0;
    WORD _fill1;
    DWORD esp1;
    WORD ss1;
    WORD _fill2;
    DWORD esp2;
    WORD ss2;
    WORD _fill3;
    DWORD cr3;
    DWORD eip;
    DWORD eflags;
    DWORD eax;
    DWORD ecx;
    DWORD edx;
    DWORD ebx;
    DWORD esp;
    DWORD ebp;
    DWORD esi;
    DWORD edi;
    WORD es;
    WORD _fill5;
    WORD cs;
    WORD _fill6;
    WORD ss;
    WORD _fill7;
    WORD ds;
    WORD _fill8;
    WORD fs;
    WORD _fill9;
    WORD gs;
    WORD _fill10;
    WORD ldt;
    WORD _fill11;
    WORD trap;
    WORD io_base;
    DWORD control;
    BYTE ctx_fpu[FPU_CONTEXT_SIZE];
};

/* Irq services specifications */

#define TIMER_IRQ	0
#define	KEYB_IRQ	1
#define COM2_IRQ	3
#define COM1_IRQ	4
#define COM4_IRQ	3
#define COM3_IRQ	4
#define SB_IRQ		5
#define FDC_IRQ		6
#define SB2_IRQ		7
#define RTC_IRQ		8
#define PS2MOUSE_IRQ	12
#define COPROC_IRQ	13
#define IDE0_IRQ	14
#define IDE1_IRQ	15

END_DEF

#endif
