/* FD32 Interrupt Handlers & similar stuff
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/linkage.h>
#include <ll/i386/sel.h>
	
.globl SYMBOL_NAME(khand)
SYMBOL_NAME_LABEL(khand)
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
	call	SYMBOL_NAME(keyb_handler)
	popl	%eax
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popal
	iret
