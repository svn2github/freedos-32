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
