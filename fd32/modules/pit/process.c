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
/* Use the programmed PIT tick and convert when using, avoid losing precision */
static unsigned pit_ticks;

static unsigned convert_to_ticks(unsigned usec)
{
	uint64_t tmp_pit_ticks = pit_ticks;
	uint64_t tmp_usec = usec;
	return (tmp_usec * PIT_CLOCK) / (tmp_pit_ticks * 1000000);
}


void pit_process(void)
{
	//LOG_PRINTF(("[PIT] start pit_process\n"));
	Event *e, *enext;
	for (e = (Event *) events_used.begin; e; e = enext)
	{
		enext = e->next;
		fd32_cli();
		if (ticks >= e->when)
		{
			LOG_PRINTF(("[PIT] @%u event %08xh (scheduled @%u)\n", (unsigned) ticks, (unsigned) e, (unsigned) e->when));
			fd32_sti();
			list_erase(&events_used, (ListItem *) e);
			list_push_front(&events_free, (ListItem *) e);
			e->callback(e->param);
		}
		fd32_sti();
	}
	//LOG_PRINTF(("[PIT] end pit_process\n"));
}


/**
 * \brief Replacement for interrupt service routine.
 *
 * Another module or native application taking control of the PIT may call this
 * function every 55ms to enable time events registered with this module to be
 * dispatched. INT 1Ch and other actions only required for DOS compatibility 
 * will not be performed by this function.
 */
void pit_external_process(void)
{
	fd32_cli();
	ticks++;
	fd32_sti();
	pit_process();
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
		e->when = ticks + convert_to_ticks(usec);
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
	uint64_t when = ticks + convert_to_ticks(usec);
	while (1)
	{
		fd32_cli();
		if(ticks >= when)
		{
			fd32_sti();
			break;
		}
		fd32_sti();
		fd32_cpu_idle();
	}
}

/**
 * \brief Blocks the execution for the specified amount of more precise time.
 * \param nsec number of nanoseconds to block.
 * \remarks The function actively loops until the specified amount of time has passed.
 */
static void nano_delay_l(unsigned nsec)
{
	unsigned usec = nsec/1000 + 1;
	pit_delay(usec);
}

INLINE_OP uint64_t rdtsc(void)
{
	uint64_t r;
	__asm__ __volatile__ ("rdtsc" : "=A" (r));
	return r;
}

static int use_rdtsc;
static uint64_t tick_per_sec_p;

/* A implementation of nano_delay Pentium+ processor (by Hanzac Chen) */
static void nano_delay_p(unsigned nsec)
{
	uint64_t cpu_tick = rdtsc();
	/* NOTE:
	 *	Use tick_per_usec instead of tick_per_nsec is for increasing integer
	 *	calculation precision
	 */
	uint64_t when_tick = cpu_tick + nsec * tick_per_sec_p / 1000000000; /* using libgcc to allow 64-bit division */

	while (cpu_tick < when_tick) {
		cpu_tick = rdtsc();
	}
}

static void nano_delay_init(void)
{
	if (use_rdtsc) {
		uint64_t start, end;

		start = rdtsc();
		pit_delay(1000000); /* delay one second */
		end = rdtsc();

		tick_per_sec_p = end - start;

		LOG_PRINTF(("[PIT] PIT Tick per second is %u\n", PIT_CLOCK / pit_ticks));
		LOG_PRINTF(("[PIT] CPU Tick per second is %llu\n", tick_per_sec_p));

		if (add_call("nano_delay", (uint32_t) nano_delay_p, ADD) == -1)
			message("[PIT] Cannot add \"nano_delay\" to the symbol table. Aborted.\n");
	} else {
		if (add_call("nano_delay", (uint32_t) nano_delay_l, ADD) == -1)
			message("[PIT] Cannot add \"nano_delay\" to the symbol table. Aborted.\n");
	}
}

static struct { char *name; uint32_t address; } symbols[] =
{
	{ "pit_event_register", 	(uint32_t) pit_event_register 	},
	{ "pit_event_cancel",   	(uint32_t) pit_event_cancel   	},
	{ "pit_delay",          	(uint32_t) pit_delay          	},
	{ "pit_external_process", 	(uint32_t) pit_external_process	},
	{ 0, 0 }
};


static struct option pit_options[] =
{
  /* These options set a flag. */
  {"pentium-tsc", no_argument, &use_rdtsc, 0},
  /* These options don't set a flag.
     We distinguish them by their indices. */
  {0, 0, 0, 0}
};


void pit_init(process_info_t *pi)
{
	unsigned k;
	uint32_t f;

	/* Parsing options ... */
	char **argv;
	int argc = fd32_get_argv(pi->filename, pi->args, &argv);

	use_rdtsc = 0; /* Disable rdtsc by default for i386 compatible */
	if (argc > 1) {
		int c, option_index = 0;
		/* Parse the command line */
		for ( ; (c = getopt_long (argc, argv, "", pit_options, &option_index)) != -1; ) {
			switch (c) {
				case 0:
					use_rdtsc = 1;
					message("Pentium+ time-stamp enabled\n");
					break;
				default:
					break;
			}
		}
	}
	fd32_unget_argv(argc, argv);

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
	pit_ticks = 0x10000; /* Run only at the lowest frequency for now */
	f = ll_fsave();
	fd32_outb(0x43, 0x34); /* Counter 0, load LSB then MSB, mode 2 (rate generator), binary counter */
	fd32_outb(0x40, pit_ticks & 0x00FF);
	fd32_outb(0x40, pit_ticks >> 8);
	idt_place(PIC1_BASE + 0, pit_isr);
	irq_unmask(0);
	ll_frestore(f);

	/* Initialize the nano_delay */
	nano_delay_init();
	message("done\n");
}

/* @} */
