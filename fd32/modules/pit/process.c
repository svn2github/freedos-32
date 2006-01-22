/* FreeDOS-32 PIT-driven Event Management version 0.1
 * Copyright (C) 2006  Salvatore ISAJA
 *
 * This file "process.c" is part of the
 * FreeDOS-32 PIT-driven Event Management (the Program).
 *
 * The Program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Program; see the file GPL.txt; if not, write to
 * the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <dr-env.h>
#include <list.h>
#include <kernel.h>
#include <ll/i386/error.h>
#include "pit.h"
extern void pit_isr(void);

#define PIT_DEBUG 0
#if PIT_DEBUG
 #define LOG_PRINTF(x) fd32_log_printf x
#else
 #define LOG_PRINTF(x)
#endif

/**
 * \defgroup pit PIT-driven event management
 *
 * These facilities add timer support to FreeDOS-32, using the Programmable Interval Timer.
 * The PIT is programmed at the default frequency of about 18.2 Hz, and is used to
 * schedule events at specified times. When an event is triggered a custom callback
 * function is called. All callback functions are called from inside the interrupt service
 * routine of the PIT.
 * \par Copyright:
 *      FreeDOS-32 PIT-driver event management version 0.1 \n
 *      Copyright &copy; 2006 Salvatore Isaja \n
 *      GNU General Public License version 2 or later
 * \todo The list of events could be ordered to optimize how the timer ISR loops across
 *       events to check which one to trigger.
 * @{
 */

typedef struct Event
{
	struct Event *prev;
	struct Event *next;
	uint64_t when;
	void (*callback)(void *p);
	void *param;
}
Event;

#define NUM_EVENTS 64 /* Max number of events allowed to be registered at the same time */
#define PIT_CLOCK 1193181 /* Hz, according to Mark Feldman's PC-GPE article on PIT */
static Event events_table[NUM_EVENTS];
static List events_used;
static List events_free;
static volatile uint64_t ticks;
static unsigned usec_per_tick;


void pit_process(void)
{
	//LOG_PRINTF(("[PIT] start pit_process\n"));
	Event *e, *enext;
	ticks++;
	for (e = (Event *) events_used.begin; e; e = enext)
	{
		enext = e->next;
		if (ticks >= e->when)
		{
			LOG_PRINTF(("[PIT] @%u event %08xh (scheduled @%u)\n", (unsigned) ticks, (unsigned) e, (unsigned) e->when));
			list_erase(&events_used, (ListItem *) e);
			list_push_front(&events_free, (ListItem *) e);
			e->callback(e->param);
		}
	}
	//LOG_PRINTF(("[PIT] end pit_process\n"));
}


/**
 * \brief Registers a timer-driven event.
 *
 * The callback function specified by \a callback will be called by the timer handler
 * after the amount of microseconds specified by \a usec has elapsed. The parameter
 * specified by \a param will be passed to the callback function.
 * \return A handle to identify the registered event, or NULL on failure.
 * \remarks The actual time elapsed may be longer than \a usec according to the rounding
 *          due to the frequency the PIT is programmed.
 */
void *pit_event_register(unsigned usec, void (*callback)(void *p), void *param)
{
	LOG_PRINTF(("[PIT] start pit_event_register (%u us)\n", usec));
	uint32_t f = ll_fsave();
	Event *e = (Event *) events_free.begin;
	if (e)
	{
		list_erase(&events_free, (ListItem *) e);
		list_push_back(&events_used, (ListItem *) e);
		e->when = ticks + usec / usec_per_tick;
		e->callback = callback;
		e->param = param;
	}
	ll_frestore(f);
	LOG_PRINTF(("[PIT] end pit_event_register %08xh (ticks=%u,when=%u,diff=%u)\n",
		(unsigned) e, (unsigned) ticks, (unsigned) e->when, (unsigned) (e->when - ticks)));
	return e;
}


/**
 * \brief Cancels a registered timer-driven event.
 * \param event_handle handle of the event to cancel as returned by pit_event_register.
 * \return 0 on success, or a negative value on failure (event not found).
 * \remarks It is not necessary to cancel an event that already occurred, although it
 *          does not harm to (and a negative value will be returned).
 */
int pit_event_cancel(void *event_handle)
{
	Event *e;
	int res = -1;
	uint32_t f = ll_fsave();
	LOG_PRINTF(("[PIT] @%u pit_event_cancel %08xh\n", (unsigned) ticks, (unsigned) event_handle));
	for (e = (Event *) events_used.begin; e; e = e->next)
		if (e == event_handle)
		{
			list_erase(&events_used, (ListItem *) e);
			list_push_front(&events_free, (ListItem *) e);
			res = 0;
			break;
		}
	ll_frestore(f);
	return res;
}


/**
 * \brief Blocks the execution for the specified amount of time.
 * \param usec number of microseconds to block.
 * \remarks The function actively loops until the specified amount of time has passed.
 * \remarks The actual time elapsed may be longer than \a usec according to the rounding
 *          due to the frequency the PIT is programmed.
 */
void pit_delay(unsigned usec)
{
	uint64_t when = ticks + usec / usec_per_tick;
	while (ticks < when) ;
}


static struct { char *name; uint32_t address; } symbols[] =
{
	{ "pit_event_register", (uint32_t) pit_event_register },
	{ "pit_event_cancel",   (uint32_t) pit_event_cancel   },
	{ "pit_delay",          (uint32_t) pit_delay          },
	{ 0, 0 }
};


void pit_init()
{
	unsigned k;
	uint32_t f;
	message("Going to install PIT-driven event management... ");
	for (k = 0; symbols[k].name; k++)
		if (add_call(symbols[k].name, symbols[k].address, ADD) == -1)
		{
			message("\n[PIT] Cannot add \"%s\" to the symbol table. Aborted.\n", symbols[k].name);
			return;
		}
	list_init(&events_used);
	list_init(&events_free);
	for (k = 0; k < NUM_EVENTS; k++) list_push_back(&events_free, (ListItem *) &events_table[k]);
	ticks = 0;
	//k = PIT_CLOCK / frequency;
	k = 0x10000; /* Run only at the lowest frequency for now */
	usec_per_tick = (1000000u >> 4) * k / (PIT_CLOCK >> 4); /* a bit less precise (in theory), but avoids 64-bit division */
	f = ll_fsave();
	fd32_outb(0x43, 0x34); /* Counter 0, load LSB then MSB, mode 2 (rate generator), binary counter */
	fd32_outb(0x40, k & 0x00FF);
	fd32_outb(0x40, k >> 8);
	idt_place(PIC1_BASE + 0, pit_isr);
	irq_unmask(0);
	ll_frestore(f);
	message("done\n");
}

/* @} */
