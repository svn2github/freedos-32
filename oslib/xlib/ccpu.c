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

/*	CPU detection code	*/

#include <ll/i386/hw-data.h>
#include <ll/i386/hw-arch.h>
#include <ll/i386/mem.h>

FILE(Cpu-C);

INLINE_OP void cpuid(DWORD a, DWORD *outa, DWORD *outb, DWORD *outc, DWORD *outd)
{
#ifdef __OLD_GNU__
    __asm__ __volatile__ (".byte 0x0F,0xA2"
#else
    __asm__ __volatile__ ("cpuid"
#endif
    : "=a" (*outa),
      "=b" (*outb),
      "=c" (*outc),
      "=d" (*outd)
    : "a"  (a));
}

void x86_get_cpu(struct ll_cpu_info *p)
{
	DWORD tmp;

	memset(p, 0, sizeof(struct ll_cpu_info));
	if (x86_is_386()) {
		p->x86_cpu = 3;
		
		return;
	}
	if (x86_has_cpuid()) {
		p->x86_cpu_id_flag = 1;
		p->x86_cpu = 5;
		cpuid(0, &tmp, &(p->x86_vendor_1), 
				&(p->x86_vendor_3),
				&(p->x86_vendor_2));
		if (tmp >= 1) {
			cpuid(1, &(p->x86_signature),
				&(p->x86_intel_feature_1),
				&(p->x86_intel_feature_2),
				&(p->x86_standard_feature));
		}
	} else {
		p->x86_cpu = 4;
		if (x86_is_cyrix()) {
			/* Err... Adjust IT!!!! */
			p->x86_cpu = 11;
		}
		/* Need tests for AMD and others... */
	}
}
