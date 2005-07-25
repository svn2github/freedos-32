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

/*	Code & data fot CPU identification	*/

#ifndef __LL_I386_HW_ARCH_H__
#define __LL_I386_HW_ARCH_H__

#include <ll/i386/defs.h>

#include <ll/i386/hw-data.h>
#include <ll/i386/hw-instr.h>
#include <ll/i386/hw-func.h>

BEGIN_DEF

/* The following structure is filled up by the ll_init() and can be 	*/
/* read by the kernel modules to know about the architecture		*/
/* Each time a new hardware CPU is added to the ll layer, you have to	*/
/* define a new XXX_ARCH structure, whose first field is a char *arch	*/
/* This is used to identify the architecture, then subsequent field	*/
/* can be decoded!!							*/

/* WARNING: I tried to use bitfields, but this caused am INTO error?!?  */
/* only when using GNU-C!! So i preferred to use standard DWORD flags	*/

/* Capabilities */
#define LL_X86_HAS_INVLPG 	0x01
#define LL_X86_HAS_CPUID	0x02
#define LL_X86_HAS_FPU		0x04
#define LL_X86_INTERNAL_FPU	0x08
#define LL_X86_HAS_TSTAMP	0x10

/* Bugs */
#define LL_X86_FDIV_BUG		0x01
#define LL_X86_F00F_BUG		0x02

struct x86_arch {
    char *arch;
    int cpu;	/* 0,1,2,3,4,5,6 ->
    			8086/8,80186,80286,80386,80486,P5,PII o Overdrive */
    int fpu;	/* 0,1,2,3 -> None,8087,80287,80387 */
    char *model; /* Dx, Dx2, ... */
    char vendor[12]; /* Intel, Cyrix, AMD or unknown */
    DWORD capabilities;
    DWORD bugs;
    /* BUGs!! Warning: Currently, no workaround is available!
    */
    int f00f_bug;
    int fdiv_bug;
};

struct ll_cpu_info {
	DWORD x86_cpu;
	DWORD x86_cpu_id_flag;
	DWORD x86_vendor_1;
	DWORD x86_vendor_2;
	DWORD x86_vendor_3;
	DWORD x86_signature;
	DWORD x86_intel_feature_1;
	DWORD x86_intel_feature_2;
	DWORD x86_standard_feature;
};

struct ll_arch_struct {
    char *arch;
    struct x86_arch x86;
};

void x86_get_cpu(struct ll_cpu_info *p);
void x86_get_fpu(void);
int x86_is_386(void);
int x86_is_cyrix(void);
int x86_has_cpuid(void);

extern struct ll_arch_struct ll_arch;

END_DEF
#endif /* __LL_I386_HW_ARCH_H__ */
