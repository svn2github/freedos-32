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

/*	Time Event routines	*/

#include <ll/i386/stdlib.h>
#include <ll/i386/mem.h>
#include <ll/i386/error.h>
#include <ll/i386/hw-arch.h>
#include <ll/i386/pic.h>
#include <ll/i386/pit.h>
#include <ll/sys/ll/ll-data.h>
#include <ll/sys/ll/time.h>
#include <ll/sys/ll/event.h>

FILE(Event);

int (*event_post)(struct timespec time, void (*handler)(void *p), void *par);
int (*event_delete)(int index);
BYTE frc;

/* Timer 0 usec base tick */
DWORD ticksize;

/* Timer 0 loaded time constant (= ticksize * 1.193) */
WORD pit_time_const;
DWORD timermode;

static DWORD nts;		/* System tick in nanoSeconds... */
struct timespec actTime;	/* Time (in nanosecs)... */
extern int irq_active;

WORD lastTime;
struct pitspec globalCounter;

struct event eventlist[MAX_EVENT];

struct event *freeevents;
struct event *firstevent;

extern void *last_handler;
extern void (*evt_prol) (void);
extern void (*evt_epil) (void);

void event_setlasthandler(void *p)
{
    last_handler = p;
}                                                                               

void event_setprologue(void *p)
{
    evt_prol = p;
}

void event_setepilogue(void *p)
{
    evt_epil = p;
}

/* Switched to timespec */
int periodic_event_post(struct timespec time, void (*handler) (void *p),
			void *par)
{
    struct event *p;
    struct event *p1, *t;

    if (!freeevents) {
	return -1;
    }

    /* Extract from the ``free events'' queue */
    p = freeevents;
    freeevents = p->next;

    /* Fill the event fields */
    p->handler = handler;
    TIMESPEC_ASSIGN(&(p->time), &time);
    p->par = par;

    /* ...And insert it in the event queue!!! */

    t = NULL;
    /* walk through list, finding spot, adjusting ticks parameter */
    for (p1 = firstevent; p1; p1 = t->next) {
/*
		SUBTIMESPEC(&time, &(p1->time), &tmp);
		if ((tmp.tv_sec > 0) && (tmp.tv_nsec > 0)) {
*/
	if (TIMESPEC_A_GT_B(&time, &p1->time))
	    t = p1;
	else
	    break;
    }

    /* adjust next entry */
    if (t) {
	t->next = p;
    } else {
	firstevent = p;
    }
    p->next = p1;

    return p->index;
}

int periodic_event_delete(int index)
{
    struct event *p1, *t;

    t = NULL;
    /* walk through list, finding spot, adjusting ticks parameter */
    for (p1 = firstevent; (p1) && (index != p1->index); p1 = t->next) {
	t = p1;
    }

    if (p1 == NULL) {
	return -1;
    }

    if (t == NULL) {
	firstevent = p1->next;
    } else {
	t->next = p1->next;
    }
    p1->next = freeevents;
    freeevents = p1;

    return 1;
}

void periodic_wake_up(void)
{				/* CHANGE the NAME, please... */
    struct event *p, *pp;
    WORD tmp;

    tmp = pit_read(frc);
    ADDPITSPEC((WORD) (lastTime - tmp), &globalCounter);
    lastTime = tmp;

    irq_active++;
    if (irq_active == 1 && evt_prol != NULL) {
	evt_prol();
    }

    ADDNANO2TIMESPEC(nts, &actTime);

    for (p = firstevent; p != NULL; p = pp) {
/*
		SUBTIMESPEC(&(p->time), &actTime, &tmp);
		if ((tmp.tv_sec > 0) && (tmp.tv_nsec > 0)) {
			break;
		} */
	if ((p->time.tv_sec > actTime.tv_sec) ||
	    ((p->time.tv_sec == actTime.tv_sec)
	     && (p->time.tv_nsec > actTime.tv_nsec))) {
	    break;
	}
	pp = p->next;
	p->next = freeevents;
	freeevents = p;
	firstevent = pp;
	p->handler(p->par);
    }

    if (irq_active == 1 && evt_epil != NULL) {
	evt_epil();
    }
    irq_active--;
}

void event_init(struct ll_initparms *l)
{
    extern void ll_timer(void);
    int i;
    BYTE mask;
    TIME t;

    idt_place(PIC1_BASE, ll_timer);

    if ((l->mode & LL_TIMER_MODE_MASK) != LL_PERIODIC) {
	error("Trying one-shot!!!");
	t = 0;
	/* Mode: Binary/Mode 4/16 bit Time_const/Counter 0 */
	pit_init(0, TMR_MD4, 0xFFFF);	/* Timer 0, Mode 4, constant 0xFFFF */
    } else {
	t = l->tick;
	/* Translate the tick value in usec into a suitable time constant   */
	/* for 8254 timer chip; the chip is driven with a 1.19318 MHz       */
	/* (= 14.31818 / 12)frequency; then the effective frequency is      */
	/* given by the base frequency divided for the time constant;       */
	/* the tick is the inverse of this effective frequency (in usec!)   */
	/* Time-Constant = f_base (MHz) * tick (usec)                       */
	/* If T-C == 0 -> T-C = 65536 (Max available)                       */
	ticksize = t;
	t = t * 1193;
	t = t / 1000;
	/* Only for security! This should cause timer overrun */
	/* While 0 would set maximum period on timer          */
	if (t == 0)
	    t = 1;
	pit_time_const = (WORD) (t & 0xFFFF);
	/* Mode: Binary/Mode 2/16 bit Time_const/Counter 0 */
	pit_init(0, TMR_MD2, t);	/* Timer 0, Mode 2, Time constant t */


    }
    timermode = l->mode & LL_TIMER_MODE_MASK;
    if ((ll_arch.x86.cpu > 4) && ((l->mode & LL_FORCE_TIMER2) == 0)) { 
	/* Timer1: mode 0, time const 0... */
	pit_init(1, TMR_MD0, 0);
	frc = 1;
    } else {
	frc = 2;
	pit_init(2, TMR_MD0, 0);
	outp(0x61, 3);
    }

    mask = inp(0x21);
    mask &= 0xFE;		/* 0xFE = ~0x01 */
    outp(0x21, mask);


    /* Init the event list... */
    for (i = 0; i < MAX_EVENT; i++) {
	if (i < MAX_EVENT - 1) {
	    eventlist[i].next = &(eventlist[i + 1]);
	}
	eventlist[i].index = i;
    }
    eventlist[MAX_EVENT - 1].next = NULL;
    freeevents = &(eventlist[0]);

    evt_prol = NULL;
    evt_epil = NULL;

    /* Initialization of the time variables for periodic mode */
#if 0
#else
    ticksize = pit_time_const * 1000 / 1193;
#endif
    nts = ticksize * 1000;
    NULL_TIMESPEC(&actTime);

    /* Initialization of the general time variables */
    NULLPITSPEC(&globalCounter);
    lastTime = 0;

    if (timermode == LL_PERIODIC) {
	event_post = periodic_event_post;
	event_delete = periodic_event_delete;
    } else {
	event_post = oneshot_event_post;
	event_delete = oneshot_event_delete;
    }

    /* Last but not least... */
    irq_unmask(0);
}
