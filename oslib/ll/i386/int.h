#ifndef __LL_I386_INT__
#define __LL_I386_INT__

#include <ll/i386/linkage.h>

#define INT(n) \
.globl SYMBOL_NAME(h##n) ; \
SYMBOL_NAME_LABEL(h##n)  ; \
        pushal           ; \
	movl $##n, %eax	 ; \
	jmp ll_handler

#define INT_1(n) \
.globl SYMBOL_NAME(h##n) ; \
SYMBOL_NAME_LABEL(h##n)  ; \
        pushal           ; \
	movl $##n, %eax	 ; \
	jmp ll_handler_master_pic

#define INT_2(n) \
.globl SYMBOL_NAME(h##n) ; \
SYMBOL_NAME_LABEL(h##n)  ; \
        pushal           ; \
	movl $##n, %eax	 ; \
	jmp ll_handler_slave_pic

#define EXC(n) \
.globl SYMBOL_NAME(exc##n) ; \
SYMBOL_NAME_LABEL(exc##n)  ; \
	movl	$##n, %eax ; \
	jmp	ll_handler2
#endif
