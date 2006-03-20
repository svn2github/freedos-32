#ifndef __TIMER_H
#define __TIMER_H

#include <dr-env/stdint.h>
/* 'mode' argument to gettime. */
#define TIMER_GET_EXACT 		1
#define TIMER_GET_FAST			2
#define TIMER_GET_FREQUENCY		3

/* Argument to timer_set_mode, mainly applies to the PIT. */
#define TIMER_COMPATIBLE_MODE	0
#define TIMER_NATIVE_MODE		1

/* Identificatin of timer. 
	bit 8-31: types of timer supported by module. */
/* Platform: generic */
#define REALTIME_TIMER_TYPE		(1<<8)
#define USEC_TIMER_TYPE 		(1<<9)
#define NSEC_TIMER_TYPE		 	(1<<10)

/* Platform: PC (x86) */
#define PIT_TIMER_TYPE 		(1<<13)
#define APIC_TIMER_TYPE 	(1<<14)
#define HPET_TIMER_TYPE		(1<<15)
#define TSC_TIMER_TYPE		(1<<16)

/* Timer specific constants <0x1000 */
#define REQ_TIMER_EVENT_REGISTER	(0 + 0x1000)
#define REQ_TIMER_EVENT_CANCEL		(1 + 0x1000)
#define REQ_TIMER_DELAY				(2 + 0x1000)

typedef struct timer_register_event
{
	size_t Size;     /* Size in bytes of this structure */
	unsigned usec;
	void (*callback)(void *p);
	void *param;
	void *event_handle; /* This will be set by the request function. */
} timer_register_event_t;

typedef struct timer_delay
{
	size_t Size;     /* Size in bytes of this structure */
	unsigned usec;
} timer_delay_t;


void *timer_event_register(unsigned usec, void (*callback)(void *p), void *param);
int   timer_event_cancel  (void *event_handle);
void  timer_delay         (unsigned usec);
void  nano_delay        (unsigned nsec);
//void  pit_external_process(void);
uint64_t timer_gettime( int mode, int timer );
//int pit_set_mode( int mode, uint16_t maxcount );


#endif
