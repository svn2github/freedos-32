#ifndef __PIT_H
#define __PIT_H

#include <timer.h>
/* Argument to pit_set_mode, mainly applies to the PIT. */
#define PIT_COMPATIBLE_MODE	0
#define PIT_NATIVE_MODE		1


#define REQ_PIT_SET_MODE			0
#define REQ_PIT_EXTERNAL_PROCESS	1


typedef struct pit_set_mode
{
	size_t Size;     /* Size in bytes of this structure */
	int mode;
	uint16_t maxcount;
} pit_set_mode_t;


#endif
