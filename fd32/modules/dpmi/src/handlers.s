/* DPMI Driver for FD32: Interrupt Handlers & stack switch
 * by Luca Abeni & Hanzac Chen
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/sel.h>
#include <ll/i386/linkage.h>

#define INT_HANDLER_PARAMS_SIZE 0x64

.text
.extern SYMBOL_NAME(memcpy)
.extern SYMBOL_NAME(dpmi_chandler)
.extern SYMBOL_NAME(dpmi_stack)
.extern SYMBOL_NAME(dpmi_stack_top)

.globl SYMBOL_NAME(chandler)
SYMBOL_NAME_LABEL(chandler)
    movl   %esp, %eax
    cmpl   SYMBOL_NAME(dpmi_stack), %eax
    jbe    STACK_SWITCH
    cmpl   SYMBOL_NAME(dpmi_stack_top), %eax
    ja     STACK_SWITCH
    jmp    SYMBOL_NAME(dpmi_chandler)

STACK_SWITCH:
    push   %ebp
    movl   %esp, %ebp
    addl   $0x08, %ebp

    /* Switch to the new stack */
    movl   SYMBOL_NAME(dpmi_stack_top), %eax
    subl   $(INT_HANDLER_PARAMS_SIZE), %eax
    /* SS is always X_FLATDATA_SEL */
    movl   %eax, %esp
    
    subl   $0x10, %esp
    /* Copy the parameters to the new stack */
    movl   $(INT_HANDLER_PARAMS_SIZE), 0x8(%esp)
    movl   %ebp, 0x4(%esp)
    movl   %eax, (%esp)
    call   SYMBOL_NAME(memcpy)
    addl   $0x10, %esp
    
    call   SYMBOL_NAME(dpmi_chandler)
    
    subl   $0x10, %esp
    /* Copy back the parameters */
    movl   $(INT_HANDLER_PARAMS_SIZE), 0x8(%esp)
    movl   %ebp, (%esp)
    movl   SYMBOL_NAME(dpmi_stack_top), %eax
    subl   $(INT_HANDLER_PARAMS_SIZE), %eax
    movl   %eax, 0x4(%esp)
    call   SYMBOL_NAME(memcpy)
    
    subl   $0x08, %ebp
    leave
    ret

/* FD32 run & return facilities for WRAPPER (by Luca Abeni) */
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
	je gone
	popl %ebx
gone:
	popl %fs
	popa
	movl retval, %eax
	ret
