#ifndef __PIT_H
#define __PIT_H

void *pit_event_register(unsigned usec, void (*callback)(void *p), void *param);
int   pit_event_cancel  (void *event_handle);
void  pit_delay         (unsigned usec);
void  nano_delay        (unsigned nsec);

#endif
