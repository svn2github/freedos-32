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

/*	The Programmable Interrupt Controller management code	*/

#ifndef __PIC_H__
#define __PIC_H__

#include <ll/i386/defs.h>

#include <ll/i386/hw-data.h>
#include <ll/i386/hw-instr.h>
#include <ll/i386/hw-func.h>

BEGIN_DEF

/* WARNING!!! When these values are changed,
 * xlib/exc.s should be changed too
 */
#define PIC1_BASE 0x060 /* Interrupt base for each PIC */
#define PIC2_BASE 0x070 

void pic_init(void);
void pic_end(void);
void irq_mask(WORD irqno);
void irq_unmask(WORD irqno);

INLINE_OP void l1_exc_bind(int i, void (*f)(int n)) 
{
  l1_int_bind(i, f);
}

INLINE_OP int l1_irq_bind(int irq, void *f)
{
  int i;
  
  if (irq < 8) {
    i = irq + PIC1_BASE;
  } else if (irq < 16) {
    i = irq + PIC2_BASE - 8;
  } else {
    return -1;
  }
  
  l1_int_bind(i, f);
  return 1;
}

END_DEF
#endif        /* __PIC_H__ */
