/* FD32 run & return facilities
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */
#include <ll/i386/linkage.h>

.bss
#if 0
retval:
	.long
param:
	.long
#else
.lcomm retval, 4
.lcomm param, 4
#endif

.data
.globl SYMBOL_NAME(current_SP)
SYMBOL_NAME_LABEL(current_SP)
	.long 0
stubinfo_sel:
	.word 0
call_entry:
call_offset:	
.word	0
.word	0
call_selector:	
.word	0
old_ss:
.word   0
tmp:
.word   0
stack_backup:
.word	0
.word	0
newstack:
.word	0
.word	0

.text	
.globl SYMBOL_NAME(wrap_run)
.globl SYMBOL_NAME(wrap_restore_sp)

SYMBOL_NAME_LABEL(wrap_run)
	movl 24(%esp), %eax
	movl %eax, newstack
	movl 20(%esp), %eax
	movw %ax, tmp
	movl 16(%esp), %eax	/* CS */
	movw %ax, call_selector
	movl 12(%esp), %eax
	movl %eax, param
	movl 8(%esp), %eax
	movw %ax, stubinfo_sel
	movl 4(%esp), %eax
	movl %eax, call_offset
	
/*	
	movl $0, %eax
	movw stubinfo_sel, %ax
	ret
*/
	pusha
        pushl %fs
	pushl $0x666
	movl param, %ebx
	pushl %ebx
        
	movw stubinfo_sel, %fs

	movl %esp, stack_backup
	/* Change stack... DS */
	movw %ss, %bx
	movw %bx, old_ss
	movw %bx, %es
	movl newstack, %eax
	movl %eax, %esp
	movw tmp, %ax
	movw %ax, %ds
	movw %ax, %ss


	/* CHECKME: I believe this must be done after the task switch... */
	movl %esp, SYMBOL_NAME(current_SP)

	lcall %es:call_entry
/* We did "lcall", but the module perhaps returned with a simple "ret"...
 * Hence, the additional "popl"
 */
	movw old_ss, %ds
 	movl stack_backup, %esp
        movw old_ss, %ss

	popl %ebx
	popl %ebx
	cmpl $0x666, %ebx
	je goon
	popl %ebx
goon:
        popl %fs
	popa
	movl retval, %eax
	ret
	
SYMBOL_NAME_LABEL(wrap_restore_sp)
	movl 4(%esp), %eax
	movl %eax, retval
	movl SYMBOL_NAME(current_SP), %esp
	subl $8,%esp
	lret
