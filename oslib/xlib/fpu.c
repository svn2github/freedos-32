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
   FPU Context switch & management functions!
   Generic 32 bit module
*/

#include <ll/i386/hw-data.h>
#include <ll/i386/hw-instr.h>
#include <ll/i386/hw-func.h>
#include <ll/i386/fpu.h>
#include <ll/i386/mem.h>

#include <ll/i386/tss-ctx.h>

FILE(FPU);

extern struct tss main_tss;

BYTE ll_fpu_savearea[FPU_CONTEXT_SIZE]; /* Global FPU scratch SaveArea */
#ifdef __FPU_DEBUG__
static long int ndp_called = 0, ndp_switched = 0;
#endif

/* FPU context management */
static struct tss *ll_has_fpu = &main_tss;

/* As the 8086 does not have an hardware mechanism to support task      */
/* switch, also the FPU context switch is implemented via software.     */
/* When a preemption occurs, if the task is marked as a MATH task, the  */
/* preemption routine will save/restore the FPU context.		*/
/* The hook is called whenever a FPU context switch is necessarty	*/

void ll_fpu_hook(void)
{
    CONTEXT current;
    struct tss *base;

    current = get_tr();
    base = (struct tss *)gdt_read(current, NULL, NULL, NULL);

    clts();
#ifdef __FPU_DEBUG__
	ndp_called++;
#endif
    if (ll_has_fpu == base) return;
#ifdef __FPU_DEBUG__
	ndp_switched++;
#endif

#if 0
    LL_FPU_save();
    memcpy(TSS_table[LL_has_FPU].ctx_FPU,LL_FPU_savearea,FPU_CONTEXT_SIZE);
#else 
    save_fpu(ll_has_fpu);
#endif

    ll_has_fpu = base;

#if 1
    memcpy(ll_fpu_savearea, base->ctx_fpu, FPU_CONTEXT_SIZE);
    ll_fpu_restore();
#else 
    restore_fpu(&(tss_table[ll_has_fpu]));
#endif
    return;
}

struct tss *ll_fpu_get_task(void)
{
    return(ll_has_fpu);
}
