/* Project:     OSLib
 * Description: The OS Construction Kit
 * Date:                1.6.2000
 * Idea by:             Luca Abeni & Gerardo Lamastra
 *
 * OSLib is an SO project aimed at developing a common, easy-to-use
 * low-level infrastructure for developing OS kernels and Embedded
 * Applications; it partially derives from the HARTIK project but it
 * currently is independently developed.
 *
 * OSLib is distributed under GPL License, and some of its code has
 * been derived from the Linux kernel source; also some important
 * ideas come from studying the DJGPP go32 extender.
 *
 * We acknowledge the Linux Community, Free Software Foundation,
 * D.J. Delorie and all the other developers who believe in the
 * freedom of software and ideas.
 *
 * For legalese, check out the included GPL license.
 */

/*	Safe abort routine & timer asm handler	*/

.title	"Abort.S"

#include <ll/i386/sel.h>
#include <ll/i386/linkage.h>
#include <ll/i386/defs.h>

#include <ll/sys/ll/exc.h>

.data          

ASMFILE(Abort)

#define SAFESTACKSIZE 4096

.bss
		/* Safe stack area for aborts	*/		
.space		4096,0
SafeStack:

.extern  SYMBOL_NAME(act_int)        
.extern  SYMBOL_NAME(abort_tail)

.text

.globl SYMBOL_NAME(ll_abort)

SYMBOL_NAME_LABEL(ll_abort)
			/* As we are terminating we cannnot handle */
			/* any other interrupt!			   */
			cli
			/* Get argument */
			movl    4(%esp),%eax
			/* Switch to safe stack */
			movl    $(SafeStack),%esp
			/* Push argument	*/
			pushl	%eax
			/* Call sys_abort(code) */
			call    SYMBOL_NAME(abort_tail)
