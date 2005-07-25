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

/*	As the name says... All the hardware-dependent instructions
	there is a 1->1 corrispondence with ASM instructions	*/

#ifndef __LL_I386_HW_INSTR_H__
#define __LL_I386_HW_INSTR_H__

#include <ll/i386/defs.h>

#define INLINE_OP __inline__ static

#include <ll/i386/hw-data.h>

/* Low Level I/O funcs are in a separate file (by Luca) */
#include <ll/i386/hw-io.h>

BEGIN_DEF

INLINE_OP WORD get_cs(void)
{WORD r; __asm__ __volatile__ ("movw %%cs,%0" : "=q" (r)); return(r);}

INLINE_OP WORD get_ds(void)
{WORD r; __asm__ __volatile__ ("movw %%ds,%0" : "=q" (r)); return(r);}
INLINE_OP WORD get_fs(void)
{WORD r; __asm__ __volatile__ ("movw %%fs,%0" : "=q" (r)); return(r);}

/*INLINE_OP DWORD get_SP(void)
{DWORD r; __asm__ __volatile__ ("movw %%esp,%0" : "=q" (r)); return(r);}*/
INLINE_OP DWORD get_sp(void)
{
    DWORD rv;
    __asm__ __volatile__ ("movl %%esp, %0"
	  : "=a" (rv));
    return(rv);
}

INLINE_OP DWORD get_bp(void)
{
    DWORD rv;
    __asm__ __volatile__ ("movl %%ebp, %0"
	  : "=a" (rv));
    return(rv);
}

INLINE_OP WORD get_tr(void)
{WORD r; __asm__ __volatile__ ("strw %0" : "=q" (r)); return(r); }

INLINE_OP void set_tr(WORD n)
{__asm__ __volatile__("ltr %%ax": /* no output */ :"a" (n)); }

INLINE_OP void set_ldtr(WORD addr)
{ __asm__ __volatile__("lldt %%ax": /* no output */ :"a" (addr)); }


/* Clear Task Switched Flag! Used for FPU preemtion */
INLINE_OP void clts(void)
{__asm__ __volatile__ ("clts"); }

/* Halt the processor! */
INLINE_OP void hlt(void)
{__asm__ __volatile__ ("hlt"); }

/* These functions are used to mask/unmask interrupts           */
INLINE_OP void sti(void) {__asm__ __volatile__ ("sti"); }
INLINE_OP void cli(void) {__asm__ __volatile__ ("cli"); }

INLINE_OP SYS_FLAGS ll_fsave(void)
{
    SYS_FLAGS result;
    
    __asm__ __volatile__ ("pushfl");
    __asm__ __volatile__ ("cli");
    __asm__ __volatile__ ("popl %eax");
    __asm__ __volatile__ ("movl %%eax,%0"
	: "=r" (result)
	:
	: "eax" );
    return(result);
}

INLINE_OP void ll_frestore(SYS_FLAGS f)
{
    __asm__ __volatile__ ("mov %0,%%eax"
	:
	: "r" (f)
	: "eax");
    __asm__ __volatile__ ("pushl %eax");
    __asm__ __volatile__ ("popfl");
}

INLINE_OP void lmempokeb(LIN_ADDR a, BYTE v)
{
	*((BYTE *)a) = v;
}
INLINE_OP void lmempokew(LIN_ADDR a, WORD v)
{
	*((WORD *)a) = v;
}
INLINE_OP void lmempoked(LIN_ADDR a, DWORD v)
{
	*((DWORD *)a) = v;
}

INLINE_OP BYTE lmempeekb(LIN_ADDR a)
{
	return *((BYTE *)a);
}
INLINE_OP WORD lmempeekw(LIN_ADDR a)
{
	return *((WORD *)a);
}
INLINE_OP DWORD lmempeekd(LIN_ADDR a)
{
	return *((DWORD *)a);
}



END_DEF

#endif
