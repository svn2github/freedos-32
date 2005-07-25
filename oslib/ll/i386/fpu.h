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

/*
    FPU context switch management functions!
    FPU management exported at kernel layer to allow the use
    of floating point in kernel primitives; this turns to be
    useful for bandwidth reservation or guarantee!
*/

#ifndef __LL_I386_FPU_H__
#define __LL_I386_FPU_H__

#include <ll/i386/defs.h>

#define INLINE_OP __inline__ static

#include <ll/i386/hw-data.h>

BEGIN_DEF

/* FPU lazy state save handling.. */
INLINE_OP void save_fpu(struct tss *t)
{
    __asm__ __volatile__("fnsave %0\n\tfwait":"=m" (t->ctx_fpu));
}

INLINE_OP void restore_fpu(struct tss *t)
{
#if 1
    __asm__ __volatile__("frstor %0" :"=m" (t->ctx_fpu));
#else
    __asm__ __volatile__("frstor %0\n\tfwait": :"m" (t->ctx_fpu));
#endif
/*    __asm__ __volatile__("frstor _LL_FPU_savearea"); */
}

INLINE_OP void smartsave_fpu(struct tss *t)
{
    if (t->control & FPU_USED) save_fpu(t);
}

INLINE_OP void reset_fpu(void) { __asm__ __volatile__ ("fninit"); }

#if 0
/* OK, now everything is clear... We test the NE bit to see if the
 * CPU is using the internal mechanism for reporting FPU errors or not...
 */
INLINE_OP int check_fpu(void)
{
    int result;
    
    __asm__ __volatile__ ("movl %cr0,%eax");
    __asm__ __volatile__ ("movl %eax,%edi");
    __asm__ __volatile__ ("andl $0x0FFFFFFEF,%eax");
    __asm__ __volatile__ ("movl %eax,%cr0");
    __asm__ __volatile__ ("movl %cr0,%eax");
    __asm__ __volatile__ ("xchgl %edi,%eax");
    __asm__ __volatile__ ("movl %eax,%cr0");
#if 0
    __asm__ __volatile__ ("xorl %eax,%eax");
    __asm__ __volatile__ ("movb %bl,%al");
#else
    __asm__ __volatile__ ("movl %edi,%eax");
    __asm__ __volatile__ ("andl $0x10,%eax");
#endif
    __asm__ __volatile__ ("shrb $4,%al");
    __asm__ __volatile__ ("movl %%eax,%0"
	: "=r" (result)
	:
	: "eax" );
    return(result);
}
#endif

INLINE_OP void init_fpu(void)
{
    __asm__ __volatile__ ("movl %cr0,%eax");
    __asm__ __volatile__ ("orl  $34,%eax");
    __asm__ __volatile__ ("movl %eax,%cr0");
    __asm__ __volatile__ ("fninit");
}


extern BYTE ll_fpu_savearea[];

extern __inline__ void ll_fpu_save(void)
{
    #ifdef __LINUX__
	__asm__ __volatile__ ("fsave ll_fpu_savearea");
    #else
	__asm__ __volatile__ ("fsave _ll_fpu_savearea");
    #endif
}

extern __inline__ void ll_fpu_restore(void)
{
    #ifdef __LINUX__
	__asm__ __volatile__ ("frstor ll_fpu_savearea");
    #else
	__asm__ __volatile__ ("frstor _ll_fpu_savearea");
    #endif
}

END_DEF

#endif
