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

.title	"CPU2.S"

#include <ll/i386/linkage.h>
#include <ll/i386/defs.h>

/*
 * The following code has been extracted by Intel AP-485
 * Application Note: Intel Processor Identification with CPUID instruction
 */

.data

ASMFILE(CPU2)

.globl	SYMBOL_NAME(x86_fpu)

SYMBOL_NAME_LABEL(x86_fpu)		.byte	0
SYMBOL_NAME_LABEL(x86_cpu)		.byte	0
fpu_status:	.word	0

/*
	struct CPU {
		DWORD X86_cpu;			0
		DWORD X86_cpuIdFlag;		4
		DWORD X86_vendor_1;		8
		DWORD X86_vendor_2;		12
		DWORD X86_vendor_3;		16
		DWORD X86_signature;		20
		DWORD X86_IntelFeature_1;	24
		DWORD X86_IntelFeature_2;	28
		DWORD X86_StandardFeature;	32
	}
*/

.text

.globl SYMBOL_NAME(x86_get_cpu)
.globl SYMBOL_NAME(x86_get_fpu)
.globl SYMBOL_NAME(x86_is_386)
.globl SYMBOL_NAME(x86_is_cyrix)
.globl SYMBOL_NAME(x86_has_cpuid)

SYMBOL_NAME_LABEL(x86_is_386)
		pushfl
		popl	%eax
		movl	%eax, %ecx
		xorl	$0x40000, %eax
		pushl	%eax
		popfl
		pushfl
		popl	%eax
		cmp	%ecx, %eax
		jz	is386
		
		pushl	%ecx
		popfl	
		movl	$0, %eax
		ret
is386:
		movl	$1, %eax
		ret

SYMBOL_NAME_LABEL(x86_has_cpuid)
		pushfl
		popl	%eax
		movl	%eax, %ecx
		xorl	$0x200000, %eax
		pushl	%eax
		popfl
		pushfl
		popl	%eax
		xorl	%ecx, %eax
		je	noCPUID

		pushl	%ecx	/* Restore the old EFLAG value... */
		popfl
		movl	$1, %eax
		ret
noCPUID:
		movl	$0, %eax
		ret

SYMBOL_NAME_LABEL(x86_is_cyrix)
		xorw	%ax, %ax
		sahf
		mov	$5, %ax
		mov	$2, %bx
		divb	%bl
		lahf
		cmpb	$2, %ah
		jne	noCyrix
		movl	$1, %eax
		ret
noCyrix:
		movl	$0, %eax
		ret

SYMBOL_NAME_LABEL(x86_get_fpu)
	/* First of all, set the FPU type to 0... */
		movb	$0, SYMBOL_NAME(x86_fpu)
		fninit
	/* Let's give some time to the FPU...
	 * We cannot use WAIT 'cause we don't know if an FPU is present...
	 */
		movl	$2, %ecx
here:
		loop	here
		movw	$0x5a5a, fpu_status
		fnstsw	fpu_status
	/* Guys, I really don't know when to wai and when not...
	 */
		movl	$2, %ecx
here1:
		loop	here1
		movw	fpu_status, %ax
		
		cmpb	$0, %al
		jne	endFPUProc

chkCW:
	/* OK, if we are here, we have some kind of FPU... */
		fnstcw 	fpu_status

	/* Guys, I really don't know when to wai and when not...
	 */
		movl	$2, %ecx
here2:
		loop	here2
		
		movw	fpu_status, %ax
		andw	$0x103f, %ax
		cmpw	$0x03F, %ax
		jne	endFPUProc
	/* ... Err... I was wrong :(. Here we are sure to have an FPU */
		movb	$1, SYMBOL_NAME(x86_fpu)
		
chkInf:
/* Well... I assume that if we arrive to x86_get_fpu, we are running on a
 * 386+ processor... Hence, the following is a complete nonsense!!!
 * I'm commenting it out, we will see...
 */
#if 0
	/* Um... If we have a -386, end of the story! */
		cmpb	$3, SYMBOL_NAME(x86_cpu)
		jle	endFPUProc
		movb	$2, SYMBOL_NAME(x86_fpu)
		fld1
		fldz
		fdivp
		fld	%st
		fchs
		fcompp
		fstsw	fpu_status
		wait
		sahf
		jz	endFPUProc
		movb	$3, SYMBOL_NAME(x86_fpu)
#else
		movb	$3, SYMBOL_NAME(x86_fpu)
#endif
endFPUProc:
		ret
