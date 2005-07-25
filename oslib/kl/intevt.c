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

/*	Interrupt Events */

#include <ll/i386/stdlib.h>
#include <ll/i386/mem.h>
#include <ll/i386/error.h>
#include <ll/i386/hw-arch.h>
#include <ll/i386/pit.h>
#include <ll/i386/pic.h>
#include <ll/sys/ll/ll-data.h>
#include <ll/sys/ll/time.h>
#include <ll/sys/ll/event.h>

FILE(IntEvent);

extern int irq_active;
void (*evt_prol) (void) = NULL;
void (*evt_epil) (void) = NULL;

struct intentry irqs[16];

void irq_init(void)
{
    int i;

    /* Initialize the interrupt handlers list!!! */
    for (i = 0; i < 16; i++) {
	irqs[i].status = INTSTAT_FREE;
	irqs[i].index = i;
	irqs[i].handler = NULL;	/* Paranoia */
	irqs[i].par = NULL;	/* Paranoia */
    }
    irq_active = 0;
}

int ll_irq_active(void)
{
    return irq_active;
}

int irq_bind(int irq, void (*handler) (void *p), DWORD flags)
{

    if ((irqs[irq].status != INTSTAT_FREE) &&
	((flags & INT_FORCE) != INT_FORCE)) {
	return -1;
    }

    irqs[irq].status = INTSTAT_ASSIGNED;

    if (handler != NULL) {
	irqs[irq].handler = handler;
	irqs[irq].par = &(irqs[irq].index);
	irqs[irq].flags = flags;
    } else {
	irqs[irq].status = INTSTAT_FREE;
    }

    return 1;
}

void act_int(BYTE n)
{
    static int ai_called = 0;

    if ((n >= PIC1_BASE) && (n < PIC1_BASE + 8)) {
      n = n - PIC1_BASE;
    } else if ((n >= PIC2_BASE) && (n < PIC2_BASE + 8)) {
      n = n - PIC2_BASE + 8;
    } else {
      /* Wow... Here, we are in error... Return? */
      return;
    }

    irq_active++;
    if (irq_active == 1 && evt_prol != NULL) {
	evt_prol();
    }
    if (irqs[n].status == INTSTAT_ASSIGNED) {
	irqs[n].status = INTSTAT_BUSY;
	if (irqs[n].flags & INT_PREEMPTABLE) {
	    sti();
	}
	irqs[n].handler(irqs[n].par);
	if (irqs[n].flags & INT_PREEMPTABLE) {
	    cli();
	}
	irqs[n].status = INTSTAT_ASSIGNED;
    }
    ai_called++;
    if (irq_active == 1 && evt_epil != NULL) {
	evt_epil();
    }
    irq_active--;
}


