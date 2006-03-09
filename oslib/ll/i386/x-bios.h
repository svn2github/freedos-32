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

/*	For accessing BIOS calls returning in Real Mode, or using VM86	*/

#ifndef __LL_I386_X_BIOS_H__
#define __LL_I386_X_BIOS_H__

#include <ll/i386/defs.h>

#include <ll/i386/hw-data.h>
#include <ll/i386/hw-instr.h>

BEGIN_DEF

#define DOS_OFF(x)      ((DWORD)(x) & 0x000F)
#define DOS_SEG(x)      (((DWORD)(x) & 0xFFFF0) >> 4)

typedef union __attribute__ ((packed)) x_regs16 {
    struct __attribute__ ((packed)) {
	WORD ax;
	WORD bx;
	WORD cx;
	WORD dx;
	WORD si;
	WORD di;
	WORD cflag;
	WORD bp;
    } x;
    struct __attribute__ ((packed)) {
	BYTE al,ah;
	BYTE bl,bh;
	BYTE cl,ch;
	BYTE dl,dh;
    } h;
} X_REGS16;

typedef struct __attribute__ ((packed)) x_sregs16 {
	WORD es;
	WORD cs;
	WORD ss;
	WORD ds;
} X_SREGS16;

void X_meminfo(LIN_ADDR *b1,DWORD *s1,LIN_ADDR *b2,DWORD *s2);
void X_callBIOS(int service,X_REGS16 *in,X_REGS16 *out,X_SREGS16 *s);
void vm86_init(LIN_ADDR buff, DWORD size, DWORD *rm_irqtable_entry);
struct tss *vm86_get_tss(WORD tss_sel);
DWORD vm86_get_stack(void);
int vm86_call(WORD ip, WORD sp, X_REGS16 *in, X_REGS16 *out, X_SREGS16 *s, struct tss *ps_tss, void *params_handle);
int vm86_callBIOS(int service,X_REGS16 *in,X_REGS16 *out,X_SREGS16 *s);

END_DEF

#endif
