/* FD32 Far Call: used to invoke exception handlers from a segment
 * different from the kernel one.
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */
#include <ll/i386/linkage.h>

.globl SYMBOL_NAME(farcall)
	
CallZone:
CallOffset:	
.word	0
.word	0
CallSel:	
.word	0

SYMBOL_NAME_LABEL(farcall)
	/* Standard C prologue */
	pushl %ebp
	movl %esp,%ebp
	
	movl 8(%ebp),%eax
	movw %ax, CallSel
	movl 12(%ebp),%eax
	movl %eax, CallOffset
	/* Uhmmm... On some compilers,
	 * lcall CallZone generates a warning...
	 * on other versions, lcall *CallZone generates an error!
         * A warning is always better than an error :)
         */
	lcall CallZone

	/* Standard C epilogue */
	movl %ebp, %esp
	popl %ebp
	ret
