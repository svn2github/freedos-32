/* FD32 Interrupt Handlers & similar stuff
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/linkage.h>
#include <ll/i386/sel.h>

#define HANDLER(n) \
.globl SYMBOL_NAME(int##n)		; \
SYMBOL_NAME_LABEL(int##n)		; \
	pushl $##n			; \
	jmp handler
.text
.extern SYMBOL_NAME(fd32_int_handler)
	
HANDLER(0x15)
HANDLER(0x56)

HANDLER(0x76)
HANDLER(0x77)

handler:
	pushal
	pushl   %ds
        pushl   %es
        pushl   %fs
        pushl   %gs
	pushfl
	movw	$(X_FLATDATA_SEL),%ax
	movw	%ax,%es
	movw	%ax,%ds
	cld
	call	SYMBOL_NAME(fd32_int_handler)
	popl	%eax
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popal
        addl $4, %esp
	iret
