#ifndef __PIT_H
#define __PIT_H

/* Argument to pit_gettime. */
#define PIT_TICKS_FAST 		1
#define PIT_TICKS_EXACT 	2
#define PIT_USECS_FAST 		3
#define PIT_USECS_EXACT 	4
#define PIT_TICK_FREQUENCY 	5
#define PIT_TSC_FREQUENCY	6
#define PIT_TSC 			7
#define PIT_GET_MODE		8

/* Argument to pit_set_mode. */
#define PIT_COMPATIBLE_MODE	0
#define PIT_NATIVE_MODE		1

void *pit_event_register(unsigned usec, void (*callback)(void *p), void *param);
int   pit_event_cancel  (void *event_handle);
void  pit_delay         (unsigned usec);
void  nano_delay        (unsigned nsec);
void  pit_external_process(void);
uint64_t pit_gettime( int mode );
int pit_set_mode( int mode, uint16_t maxcount );


#endif
