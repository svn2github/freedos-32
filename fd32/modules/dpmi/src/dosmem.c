/* DOS16 memory allocation
 * by Hanzac Chen
 *
 * This is free software; see GPL.txt
 */

#include <ll/i386/hw-data.h>
#include <ll/i386/error.h>
#include <ll/stdlib.h>

#include "errno.h"
#include "kmem.h"
#include "dosmem.h"

/* The dos memory tracker is for save of the dos memory usage */
typedef struct dos_mem_track
{
	struct dos_mem_track *next;
	DWORD base;
	DWORD size;
	WORD dos_seg;
} dos_mem_track_t;

static dos_mem_track_t dmtrack_top = {0, 0, NULL};

/* Allocate dos memory in paragraphs */
WORD dos_alloc(WORD size)
{
	dos_mem_track_t *p = (dos_mem_track_t *)mem_get(sizeof(dos_mem_track_t));

	p->size = size<<4;
	p->base = dosmem_get(p->size);
	if (p->base > 0) {
		p->dos_seg = p->base>>4;
		if ((p->base&0x0F) != 0) {
			message("[DPMI] Allocate DOS memory block not at 0x10 boundary!\n");
		}
		/* Keep the track of ... */
		p->next = dmtrack_top.next;
		dmtrack_top.next = p;
		return p->dos_seg;
	} else {
		mem_free((DWORD)p, sizeof(dos_mem_track_t));
		return 0;
	}
}

/* Free the dos memory */
int dos_free(WORD seg)
{
	int res;
	dos_mem_track_t *p, *q;

	for (q = &dmtrack_top, p = dmtrack_top.next; p != NULL; q = p, p = p->next)
		if (seg == p->dos_seg) {
			res = dosmem_free(p->base, p->size);
			if (res == 0) {
				q->next = p->next;
				return mem_free((DWORD)p, sizeof(dos_mem_track_t));
			} else {
				return res;
			}
		}

	/* Bad segment or segment not allocated */
	return -EFAULT;
}

/* Resize the dos memory */
int dos_resize(WORD seg, WORD newsize)
{
	dos_mem_track_t *p;

	for (p = dmtrack_top.next; p != NULL; p = p->next)
		if (seg == p->dos_seg) {
			newsize <<= 4;
			if (dosmem_resize(p->base, p->size, newsize) == newsize) {
				p->size = newsize;
				return 0;
			} else {
				return -ENOMEM;
			}
		}

	/* Bad segment or segment not allocated */
	return -EFAULT;
}
