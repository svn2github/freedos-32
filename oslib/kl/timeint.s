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

/*	Safe abort routine & timer asm handler	*/

.title	"Timer.S"

#include <ll/i386/sel.h>
#include <ll/i386/linkage.h>
#include <ll/i386/defs.h>

#include <ll/sys/ll/exc.h>

.data          

ASMFILE(TimerHandler)

.extern JmpSel
.extern JmpZone
.globl SYMBOL_NAME(ll_clock)
.globl SYMBOL_NAME(last_handler)		
		
		/* Used as JMP entry point to check if a real	*/
		/* context switch is necessary			*/
		
SYMBOL_NAME_LABEL(ll_clock)	.int	0
SYMBOL_NAME_LABEL(last_handler)	.long   0

.extern  SYMBOL_NAME(periodic_wake_up)
.extern  SYMBOL_NAME(oneshot_wake_up)

.text
		.globl SYMBOL_NAME(ll_timer)
/* This is the timer handler; it reloads for safety the DS register; this   */
/* prevents from mess when timer interrupts linear access to memory (not in */
/* ELF address space); then EOI is sent in order to detect timer overrun    */
/* The high level kernel procedure wake_up() is called to perform the 	    */
/* preeption at higher level (process descriptos); the resulting context    */
/* if different from the old one is used to trigger the task activation.    */

SYMBOL_NAME_LABEL(ll_timer)
			pushal
			pushw	%ds
			pushw	%es
			pushw	%fs
			pushw	%gs
			
			movw    $(X_FLATDATA_SEL),%ax
			movw    %ax,%ds
			movw    %ax,%es

			/* Send EOI to master PIC		*/
			/* to perform later the overrun test	*/
			movb    $0x20,%al
			movl	$0x20,%edx
			outb    %al,%dx

			/* Call wake_up(actual_context) */
			
			movl	SYMBOL_NAME(ll_clock),%eax
			incl 	%eax
			movl	%eax,SYMBOL_NAME(ll_clock)
			
			/* The following could be optimized a little... */
			xorl %ebx, %ebx
			movw %ss, %bx
			/* We must switch to a ``safe stack'' */
	/*
	 * OK, this is the idea: in %esp we have the address of the
	 * stack pointer in the APPLICATION address space...
	 * We assume that address spaces are implemented through segments...
	 * What we have to do is to add the segment base to %esp:
	 *	- Load the GDT base in a register
	 *	- Add DS * 8 to that value
	 *	- Read the corresponding GDT entry (the segment descriptor)
	 *	- Compute the base...
	 *	It is (*p & 0xFC) | (*(p +1)  & 0x0F) << 16) | *(p + 2)
	 */
			movl SYMBOL_NAME(GDT_base), %edi
			addl %ebx, %edi
			xorl %ebx, %ebx
			movb 7(%edi), %bh
			movb 4(%edi), %bl
			shl $16, %ebx
			movw 2(%edi), %bx
			/* Is it correct? I think so... Test it!!! */
			addl %ebx, %esp
			/* Save EBX for returning to our stack... */
			movw %ds, %cx
			movw %ss, %dx
			movw %cx, %ss
			pushl %ebx
			pushl %edx
			cld
			movl	SYMBOL_NAME(timermode), %eax
			cmpl	$1, %eax
			je	oneshot	
			call    SYMBOL_NAME(periodic_wake_up)
			jmp	goon
oneshot:		call    SYMBOL_NAME(oneshot_wake_up)
goon:
			/* This is the overrun test		 */
			/* Do it after sending EOI to master PIC */

			movb    $0x0A,%al
			movl	$0x020,%edx
			outb    %al,%dx
			inb     %dx,%al
			testb   $1,%al
			jz      Timer_OK
			movl    $(CLOCK_OVERRUN),%eax
			pushl	%eax
			call    SYMBOL_NAME(ll_abort)

Timer_OK:
			/* Restore ESP */
			popl	%edx
			popl	%ebx	/* We must subtract it from ESP...*/	
			subl	%ebx, %esp
			movw	%dx, %ss
			
#ifdef __VIRCSW__

			movw	SYMBOL_NAME(currCtx), %ax
			cmpw    %ax,JmpSel
			je      NoPreempt2
			movw    %ax,JmpSel
			ljmp    JmpZone  /* DISPATCH! */
#endif
NoPreempt2:

			cmpl	$0, SYMBOL_NAME(last_handler)
			je	nohandler
			movl	SYMBOL_NAME(last_handler), %ebx
			call	*%ebx
nohandler:
			popw	%gs
			popw	%fs
			popw	%es
			popw	%ds
			popal
			iret
