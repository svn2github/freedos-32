/*********************************************************************
 * FreeDOS 32 Event Management Code for DJGPP                        *
 * by Salvo Isaja                                                    *
 *                                                                   *
 * Copyright (C) 2003, Salvatore Isaja                               *
 *                                                                   *
 * This program is free software; you can redistribute it and/or     *
 * modify it under the terms of the GNU General Public License       *
 * as published by the Free Software Foundation; either version 2    *
 * of the License, or (at your option) any later version.            *
 *                                                                   *
 * This program is distributed in the hope that it will be useful,   *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of    *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the     *
 * GNU General Public License for more details.                      *
 *                                                                   *
 * You should have received a copy of the GNU General Public License *
 * along with this program; see the file COPYING.txt;                *
 * if not, write to the Free Software Foundation, Inc.,              *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA           *
 *********************************************************************/
#include <stdio.h>
#include <string.h>
#include <dpmi.h>
#include <go32.h>
#include "events.h"


/* Timer-driven events.                                                 */
/* An event includes a counter decreased at every timer tick.           */
/* When the counter reaches zero, a callback function is called passing */
/* the specified opaque parameter.                                      */
#define MAXEVENTS 30
static _go32_dpmi_seginfo old_int_1c, new_int_1c;
static struct
{
    Fd32EventCallback *handler;
    void              *params;
    unsigned           counter;
}
events[MAXEVENTS];


/* This is the timer (int 1Ch) handler.                           */
/* When an event timeout expires, it calls its callback function. */
static void int1c(void)
{
    unsigned k;
    for (k = 0; k < MAXEVENTS; k++)
        if (events[k].handler)
        {
            if (events[k].counter) events[k].counter--;
            else
            {
                events[k].handler(events[k].params);
                events[k].handler = 0; /* Invalidate event */
            }
        }
}


/* Schedule the specified event. The handler function will be called after */
/* the specified number of milliseconds.                                   */
Fd32Event fd32_event_post(unsigned milliseconds, Fd32EventCallback *handler, void *params)
{
    unsigned k;
    for (k = 0; k < MAXEVENTS; k++)
        if (events[k].handler == 0)
        {
            events[k].params  = params;
            events[k].counter = milliseconds * 182 / 10000; /* Assuming 18.2 Hz */
            events[k].handler = handler;
            return (Fd32Event) k;
        }
    return FD32_EVENT_NULL; /* Too many events */
}


/* Cancels a scheduled event */
void fd32_event_delete(Fd32Event event)
{
    events[(unsigned) event].handler = 0;
}


/* Initializes the event system */
void fd32_event_init(void)
{
    memset(events, 0, sizeof(events));
    int irq_state = __dpmi_get_and_disable_virtual_interrupt_state();
    _go32_dpmi_get_protected_mode_interrupt_vector(0x1C, &old_int_1c);
    new_int_1c.pm_selector = _go32_my_cs();
    new_int_1c.pm_offset   = (unsigned long) int1c;
    _go32_dpmi_allocate_iret_wrapper(&new_int_1c);
    _go32_dpmi_set_protected_mode_interrupt_vector(0x1C, &new_int_1c);
    __dpmi_get_and_set_virtual_interrupt_state(irq_state);
    printf("Timer handler installed\n");
}


/* Releases the event system */
void fd32_event_done(void)
{
    int irq_state = __dpmi_get_and_disable_virtual_interrupt_state();
    _go32_dpmi_set_protected_mode_interrupt_vector(0x1C, &old_int_1c);
    _go32_dpmi_free_iret_wrapper(&new_int_1c);
    __dpmi_get_and_set_virtual_interrupt_state(irq_state);
    printf("Timer handler uninstalled\n");
}
