#ifndef __DOSMEM_H__
#define __DOSMEM_H__

#include <ll/i386/hw-data.h>

/* Allocate dos memory in paragraphs */
WORD dos_alloc(WORD size);
/* Free the dos memory */
int dos_free(WORD seg);
/* Resize the dos memory */
int dos_resize(WORD seg, WORD newsize);

#endif
