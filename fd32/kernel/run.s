/* FD32 run & return facilities
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */
#include <ll/i386/linkage.h>

.bss
retval:
	.long
.data
.globl SYMBOL_NAME(current_SP)
SYMBOL_NAME_LABEL(current_SP)
	.long 0
stubinfo_sel:
	.word 0

.text	
.globl SYMBOL_NAME(run)
.globl SYMBOL_NAME(restore_sp)

SYMBOL_NAME_LABEL(run)
	movl 8(%esp), %eax
	movw %ax, stubinfo_sel
	movl 4(%esp), %eax
/*	
	movl $0, %eax
	movw stubinfo_sel, %ax
	ret
*/
	pusha
        pushl %fs
	movl %esp, SYMBOL_NAME(current_SP)
	movw stubinfo_sel, %fs
	call *%eax
        popl %fs
	popa
	movl retval, %eax
	ret
	
SYMBOL_NAME_LABEL(restore_sp)
	movl 4(%esp), %eax
	movl %eax, retval
	movl SYMBOL_NAME(current_SP), %esp
	subl $4,%esp
	ret
