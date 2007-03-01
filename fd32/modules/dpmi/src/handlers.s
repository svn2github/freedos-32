/* DPMI Driver for FD32: Interrupt Handlers & stack switch
 * by Luca Abeni & Hanzac Chen
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/sel.h>
#include <ll/i386/linkage.h>
#include "const.h"

.text
.extern SYMBOL_NAME(memcpy)
.extern SYMBOL_NAME(mem_get)
.extern SYMBOL_NAME(mem_free)
.extern SYMBOL_NAME(dpmi_chandler)

.globl SYMBOL_NAME(chandler)
SYMBOL_NAME_LABEL(chandler)
	/* Allocate new stack */
	subl   $0x04, %esp
	movl   $(DPMI_STACK_SIZE), (%esp)
	call   SYMBOL_NAME(mem_get)
	addl   $0x04, %esp
	/* Move eax to the stack top */
	addl   $(DPMI_STACK_SIZE), %eax

	/* { stack frame */
	push   %ebp
	movl   %esp, %ebp
	addl   $0x08, %ebp

	/* Switch to the new stack, SS is always X_FLATDATA_SEL */
	movl   %eax, %esp

	subl   $(INT_HANDLER_PARAMS_SIZE), %esp
	movl   %esp, %eax
	/* Copy the parameters to the new stack */
	subl   $0x0c, %esp
	movl   $(INT_HANDLER_PARAMS_SIZE), 0x8(%esp)
	movl   %ebp, 0x4(%esp)
	movl   %eax, (%esp)
	call   SYMBOL_NAME(memcpy)
	addl   $0x0c, %esp

	call   SYMBOL_NAME(dpmi_chandler)

	movl   %esp, %eax
	/* Copy back the parameters */
	subl   $0x0c, %esp
	movl   $(INT_HANDLER_PARAMS_SIZE), 0x8(%esp)
	movl   %eax, 0x4(%esp)
	movl   %ebp, (%esp)
	call   SYMBOL_NAME(memcpy)
	addl   $0x0c, %esp

	/* Get the stack in eax */
	movl   %esp, %eax
	addl   $(INT_HANDLER_PARAMS_SIZE), %eax
	subl   $(DPMI_STACK_SIZE), %eax
	
	subl   $0x08, %ebp
	leave
	/* } */

	/* Free the stack */
	subl   $0x08, %esp
	movl   $(DPMI_STACK_SIZE), 4(%esp)
	movl   %eax, (%esp)
	call   SYMBOL_NAME(mem_free)
	addl   $0x08, %esp
	ret


.globl SYMBOL_NAME(farcall)
SYMBOL_NAME_LABEL(farcall)
	push   %ebp
	movl   %esp, %ebp

	push %ds
	push %es

	movl 16(%ebp), %eax
	movw %ax, %ds
	movl 20(%ebp), %esi
	movl 24(%ebp), %eax
	movw %ax, %es
	movl 28(%ebp), %edi

	pushfl
	lcall 8(%ebp)

	pop %es
	pop %ds

	leave
	ret

/* FD32 run & return facilities for WRAPPER (by Luca Abeni) */
.bss
.lcomm retval, 4
.lcomm param, 4

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

	/* NOTE: the current_SP should be modified before DS changed */
	movl %esp, SYMBOL_NAME(current_SP)

	movw %ax, %ds
	movw %ax, %ss

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
	je gone
	popl %ebx
gone:
	popl %fs
	popa
	movl retval, %eax
	ret

SYMBOL_NAME_LABEL(wrap_restore_sp)
	movl 4(%esp), %eax
	movl %eax, retval
	movw tmp, %ax
	movw %ax, %ss
	movl SYMBOL_NAME(current_SP), %esp
	subl $8,%esp
	lret

/* FD32 specific vm86->pmode switch */
.code16
.globl SYMBOL_NAME(_fd32_vm86_to_pmode)
.globl SYMBOL_NAME(_fd32_vm86_to_pmode_end)
.globl SYMBOL_NAME(_fd32_rm_callback)
.globl SYMBOL_NAME(_fd32_rm_callback_end)

SYMBOL_NAME_LABEL(_fd32_vm86_to_pmode)
	int $0x2f
	pop %eax
	pop %ss
	mov %eax, %esp
	lret
SYMBOL_NAME_LABEL(_fd32_vm86_to_pmode_end)

SYMBOL_NAME_LABEL(_fd32_rm_callback)
	int $0x2f
	lret
SYMBOL_NAME_LABEL(_fd32_rm_callback_end)
