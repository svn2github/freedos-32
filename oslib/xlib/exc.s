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

/*	Exc/IRQ handlers (asm part)	*/
/* TODO: Unify the Exc/Int Mechanism... */

#include <ll/i386/sel.h>
#include <ll/i386/linkage.h>
#include <ll/i386/int.h>
#include <ll/i386/defs.h>

.extern SYMBOL_NAME(GDT_base)
.data          

ASMFILE(Exc)

.globl SYMBOL_NAME(ll_irq_table)
		
SYMBOL_NAME_LABEL(ll_irq_table)	.space  1024, 0

.text

.extern  SYMBOL_NAME(ll_exc_hook)
.extern  SYMBOL_NAME(ll_fpu_hook)

.globl SYMBOL_NAME(h13_bis)
.globl SYMBOL_NAME(exc7)

/* These are the hardware handlers; they all jump to ll_handler setting */
/* the interrupt number into EAX & save the registers on stack		*/
INT(0)
INT(1)
INT(2)
INT(3)
INT(4)
INT(5)
INT(6)
INT(8)
INT(9)
INT(10)
INT(11)
INT(12)
INT(13)
INT(14)
INT(15)
INT(16)
INT(17)
INT(18)
INT(19)
INT(20)
INT(21)
INT(22)
INT(23)
INT(24)
INT(25)
INT(26)
INT(27)
INT(28)
INT(29)
INT(30)
INT(31)

INT(32)
INT(33)
INT(34)
INT(35)
INT(36)
INT(37)
INT(38)
INT(39)
INT(40)
INT(41)
INT(42)
INT(43)
INT(44)
INT(45)
INT(46)
INT(47)
INT(48)
INT(49)
INT(50)
INT(51)
INT(52)
INT(53)
INT(54)
INT(55)
INT(56)
INT(57)
INT(58)
INT(59)
INT(60)
INT(61)
INT(62)
INT(63)

#if 0
/* Master PIC... (int 0x40, see ll/i386/pic.h)*/
INT_1(64)
INT_1(65)
INT_1(66)
INT_1(67)
INT_1(68)
INT_1(69)
INT_1(70)
INT_1(71)
#else
INT(64)
INT(65)
INT(66)
INT(67)
INT(68)
INT(69)
INT(70)
INT(71)
#endif

INT(72)
INT(73)
INT(74)
INT(75)
INT(76)
INT(77)
INT(78)
INT(79)
INT(80)
INT(81)
INT(82)
INT(83)
INT(84)
INT(85)
INT(86)
INT(87)
INT(88)
INT(89)
INT(90)
INT(91)
INT(92)
INT(93)
INT(94)
INT(95)

#if 0
INT(96)
INT(97)
INT(98)
INT(99)
INT(100)
INT(101)
INT(102)
INT(103)
#else
INT_1(96)
INT_1(97)
INT_1(98)
INT_1(99)
INT_1(100)
INT_1(101)
INT_1(102)
INT_1(103)
#endif
INT(104)
INT(105)
INT(106)
INT(107)
INT(108)
INT(109)
INT(110)
INT(111)
/* Slave PIC... (int 0x70, see ll/i386/pic.h)*/
INT_2(112)
INT_2(113)
INT_2(114)
INT_2(115)
INT_2(116)
INT_2(117)
INT_2(118)
INT_2(119)

INT(120)
INT(121)
INT(122)
INT(123)
INT(124)
INT(125)
INT(126)
INT(127)
INT(128)
INT(129)
INT(130)
INT(131)
INT(132)
INT(133)
INT(134)
INT(135)
INT(136)
INT(137)
INT(138)
INT(139)
INT(140)
INT(141)
INT(142)
INT(143)
INT(144)
INT(145)
INT(146)
INT(147)
INT(148)
INT(149)
INT(150)
INT(151)
INT(152)
INT(153)
INT(154)
INT(155)
INT(156)
INT(157)
INT(158)
INT(159)
INT(160)
INT(161)
INT(162)
INT(163)
INT(164)
INT(165)
INT(166)
INT(167)
INT(168)
INT(169)
INT(170)
INT(171)
INT(172)
INT(173)
INT(174)
INT(175)
INT(176)
INT(177)
INT(178)
INT(179)
INT(180)
INT(181)
INT(182)
INT(183)
INT(184)
INT(185)
INT(186)
INT(187)
INT(188)
INT(189)
INT(190)
INT(191)
INT(192)
INT(193)
INT(194)
INT(195)
INT(196)
INT(197)
INT(198)
INT(199)
INT(200)
INT(201)
INT(202)
INT(203)
INT(204)
INT(205)
INT(206)
INT(207)
INT(208)
INT(209)
INT(210)
INT(211)
INT(212)
INT(213)
INT(214)
INT(215)
INT(216)
INT(217)
INT(218)
INT(219)
INT(220)
INT(221)
INT(222)
INT(223)
INT(224)
INT(225)
INT(226)
INT(227)
INT(228)
INT(229)
INT(230)
INT(231)
INT(232)
INT(233)
INT(234)
INT(235)
INT(236)
INT(237)
INT(238)
INT(239)
INT(240)
INT(241)
INT(242)
INT(243)
INT(244)
INT(245)
INT(246)
INT(247)
INT(248)
INT(249)
INT(250)
INT(251)
INT(252)
INT(253)
INT(254)
INT(255)

/* The ll_handler process the request using the kernel function act_int() */
/* Then sends EOI & schedules any eventual new task!			  */

ll_handler:             
			/* We do not know what is the DS value	*/
			/* Then we save it & set it correctly  	*/
			
			pushl	%ds
			pushl	%ss
			pushl	%es
			pushl	%fs
			pushl	%gs
			/* But we first transfer to the _act_int  */
			/* the interrupt number which is required */
			/* as second argument			  */
			pushl   %eax
			movw    $(X_FLATDATA_SEL),%ax
			movw    %ax,%es
			mov     %ax,%ds
        movw    %ax, %fs
        movw    %ax, %gs
			
			/* Now save the actual context on stack		*/
			/* to pass it to _act_int (C caling convention)	*/
			/* CLD is necessary when calling a C function	*/
		
			cld
			
			/* The following could be optimized a little... */
			popl %eax
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
			movw %ss, %dx
			movw %ds, %cx
			movw %cx, %ss
			pushl %ebx
			pushl %edx
			pushl %eax

			movl	SYMBOL_NAME(ll_irq_table)(, %eax, 4), %ebx
			call    *%ebx
		
			popl	%ebx    /* Store in EBX the Int number	*/
			popl	%eax
			popl	%ecx	/* We must subtract it from ESP...*/
			subl	%ecx, %esp
			movw	%ax, %ss
			
			/* Resume the return value of _act_int 		*/
			/* & do the context switch if necessary!	*/
#ifdef __VIRCSW__
			movw SYMBOL_NAME(currCtx), %ax
			cmpw    JmpSel,%ax
			je      NoPreempt3
			movw    %ax,JmpSel
			ljmp    JmpZone 
#endif			
NoPreempt3:             popl	%gs
			popl	%fs
			popl	%es
			popl	%ss
			popl	%ds
			popal
			iret

ll_handler_master_pic:             
			/* We do not know what is the DS value	*/
			/* Then we save it & set it correctly  	*/
			
			pushl	%ds
			pushl	%ss
			pushl	%es
			pushl	%fs
			pushl	%gs
			/* But we first transfer to the _act_int  */
			/* the interrupt number which is required */
			/* as second argument			  */
			pushl   %eax
			movw    $(X_FLATDATA_SEL),%ax
			movw    %ax,%es
			mov     %ax,%ds
        movw    %ax, %fs
        movw    %ax, %gs
			
			/* Now save the actual context on stack		*/
			/* to pass it to _act_int (C caling convention)	*/
			/* CLD is necessary when calling a C function	*/
		
			cld

#ifndef __LATE_EOI__
		/* Send EOI to master PIC */
			movb	$0x20,%al
			movl	$0x20,%edx
			outb	%al,%dx
#endif	
			
			/* The following could be optimized a little... */
			popl %eax
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
			movw %ss, %dx
			movw %ds, %cx
			movw %cx, %ss
			pushl %ebx
			pushl %edx
			pushl %eax

			movl	SYMBOL_NAME(ll_irq_table)(, %eax, 4), %ebx
			call    *%ebx
		
			popl	%ebx    /* Store in EBX the Int number	*/
			popl	%eax
			popl	%ecx	/* We must subtract it from ESP...*/
			subl	%ecx, %esp
			movw	%ax, %ss

#ifdef __LATE_EOI__
			/* Send EOI to master PIC */
			movb	$0x20,%al
			movl	$0x20,%edx
			outb	%al,%dx
#endif

			/* Resume the return value of _act_int 		*/
			/* & do the context switch if necessary!	*/
#ifdef __VIRCSW__
			movw SYMBOL_NAME(currCtx), %ax
			cmpw    JmpSel,%ax
			je      NoPreempt4
			movw    %ax,JmpSel
			ljmp    JmpZone 
#endif			
NoPreempt4:             popl	%gs
			popl	%fs
			popl	%es
			popl	%ss
			popl	%ds
			popal
			iret

ll_handler_slave_pic:             
			/* We do not know what is the DS value	*/
			/* Then we save it & set it correctly  	*/
			
			pushl	%ds
			pushl	%ss
			pushl	%es
			pushl	%fs
			pushl	%gs
			/* But we first transfer to the _act_int  */
			/* the interrupt number which is required */
			/* as second argument			  */
			pushl   %eax
			movw    $(X_FLATDATA_SEL),%ax
			movw    %ax,%es
			mov     %ax,%ds
        movw    %ax, %fs
        movw    %ax, %gs
			
			/* Now save the actual context on stack		*/
			/* to pass it to _act_int (C caling convention)	*/
			/* CLD is necessary when calling a C function	*/
		
			cld
#ifndef __LATE_EOI__
			/* Send EOI to master & slave PIC */
			movb	$0x20,%al
			movl	$0xA0,%edx
			outb	%al,%dx
			movl	$0x20,%edx
			outb	%al,%dx
#endif

			/* The following could be optimized a little... */
			popl %eax
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
			movw %ss, %dx
			movw %ds, %cx
			movw %cx, %ss
			pushl %ebx
			pushl %edx
			pushl %eax

			movl	SYMBOL_NAME(ll_irq_table)(, %eax, 4), %ebx
			call    *%ebx
		
			popl	%ebx    /* Store in EBX the Int number	*/
			popl	%eax
			popl	%ecx	/* We must subtract it from ESP...*/
			subl	%ecx, %esp
			movw	%ax, %ss
			
#ifdef __LATE_EOI__
			/* Send EOI to master & slave PIC */
			movb	$0x20,%al
			movl	$0xA0,%edx
			outb	%al,%dx
			movl	$0x20,%edx
			outb	%al,%dx
#endif

			/* Resume the return value of _act_int 		*/
			/* & do the context switch if necessary!	*/
#ifdef __VIRCSW__
			movw SYMBOL_NAME(currCtx), %ax
			cmpw    JmpSel,%ax
			je      NoPreempt5
			movw    %ax,JmpSel
			ljmp    JmpZone 
#endif			
NoPreempt5:             popl	%gs
			popl	%fs
			popl	%es
			popl	%ss
			popl	%ds
			popal
			iret




/* OK, this is Exception 7, and it is generated when an ESC or WAIT
 * intruction is reached, and the MP and TS bits are set... Basically,
 * it means that the FPU context must be switched
 */
SYMBOL_NAME_LABEL(exc7)	pushal
            		pushl	%ds
			pushl	%ss
			pushl	%es
			pushl	%fs
			pushl	%gs
			movw	$(X_FLATDATA_SEL),%ax
			movw	%ax,%es
			movw	%ax,%ds
			cld
			call	SYMBOL_NAME(ll_fpu_hook)
			popl	%gs
			popl	%fs
			popl	%es
			popl	%ss
			popl	%ds
			popal
			iret
