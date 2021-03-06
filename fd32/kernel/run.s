/* FD32 run & return facilities
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */
#include <ll/i386/linkage.h>

#define DLL_PROCESS_DETACH 0
#define DLL_PROCESS_ATTACH 1

.bss
.lcomm retval, 4
.lcomm param, 4

.data
.globl SYMBOL_NAME(current_SP)
SYMBOL_NAME_LABEL(current_SP)
	.long 0
stubinfo_sel:
	.word 0

.text	
.globl SYMBOL_NAME(run)
.globl SYMBOL_NAME(dll_run)
.globl SYMBOL_NAME(restore_sp)

SYMBOL_NAME_LABEL(dll_run)
	movl 4(%esp), %eax
	pusha
	pushl $0
	pushl $DLL_PROCESS_ATTACH
	pushl $0
	movl %esp, SYMBOL_NAME(current_SP)
	call *%eax
/* The DLL init function follows the pascal parameters passing
   convenction: no need to pop the parameters from the stack!!! */
	popa
	movl $0, %eax
	ret

SYMBOL_NAME_LABEL(run)
	movl 12(%esp), %eax
	movl %eax, param
	movl 8(%esp), %eax
	movw %ax, stubinfo_sel
	movl 4(%esp), %eax
	pusha
		pushl %fs
	movl param, %ebx
	pushl %ebx
	movl %esp, SYMBOL_NAME(current_SP)
	movw stubinfo_sel, %fs
	call *%eax
	popl %ebx
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
