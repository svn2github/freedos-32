/* Luca's Simple & Buggy Physical Memory Manager (PMM)
 * It is currently used in FD/32 only to compile the kernel,
 * while waiting for a better solution...
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

/*
#define __DEBUG__
*/

#include <ll/i386/error.h>
#include <ll/i386/hw-data.h>

#include "pmm.h"
#include "kernel.h"
#include "logger.h"

void pmm_init(struct mempool *mp)
{
  mp->free = 0;
  mp->first = 0;
}

int pmm_alloc_address(struct mempool *mp, DWORD base, DWORD size)
{
  struct memheader *p, *h1, *p1;
  DWORD size2, b;

#ifdef __DEBUG__
    fd32_log_printf("PMM A_addr  0x%lx 0x%lx...", base, size);
#endif
  for (p1 = 0, p = mp->first; p!= 0; p1 = p, p = p->next) {
    b = (DWORD)p;
#ifdef __DEBUG__
    fd32_log_printf("Block 0x%lx-0x%lx  ", b, b + p->size);
#endif
    if ((b <= base) && ((b + p->size) >= (base + size))) {
      /* OK, we have to remove from this chunk... */
      size2 = (b + p->size - (base + size));
      if (size2 < sizeof(struct memheader)) {
	if (size2 != 0) {
          fd32_log_printf("PMM alloc address: WARNING - loosing %ld bytes\n", size2);
	}
	if (b >= base - sizeof(struct memheader)) {
	  if (b != base) {
            fd32_log_printf("PMM alloc address: WARNING - loosing %ld bytes\n",
		    base - b);
	  }
	  if (p1 == 0) {
	    mp->first = p->next;
	  } else {
	    p1->next = p->next;
	  }
	} else {
	  p->size -= size;
	}
      } else {
	h1 = (struct memheader *)(base + size);
	h1->size = size2;
	h1->next = p->next;
	if (b >= base - sizeof(struct memheader)) {
	  if (b != base) {
            fd32_log_printf("PMM alloc address: WARNING - loosing %ld bytes\n",
		    base - b);
	  }
	  if (p1 == 0) {
	    mp->first = h1;
	  } else {
	    p1->next = h1;
	  }
	} else {
	  p->size = (base - b);
	  p->next = h1;
	}
      }
      return 1;
    }
  }
  error("PMM alloc address: memory not available...\n");
  return -1;
}

DWORD pmm_alloc(struct mempool *mp, DWORD size)
{
  struct memheader *p, *p1;
  DWORD size2, b;

#ifdef __DEBUG__
    fd32_log_printf("PMM Alloc 0x%lx...", size);
#endif
  for (p1 = 0, p = mp->first; p!= 0; p1 = p, p = p->next) {
    b = (DWORD)p;
#ifdef __DEBUG__
    fd32_log_printf("Block 0x%lx-0x%lx  ", b, b + p->size);
#endif
    if (p->size >= size) {
#ifdef __DEBUG__
    fd32_log_printf("OK\n");
#endif
      /* OK, we found enough memory */
      size2 = (p->size - size);
      if (size2 < sizeof(struct memheader)) {
	if (size2 != 0) {
          fd32_log_printf("PMM alloc: WARNING - loosing %ld bytes\n", size2);
	}
	if (p1 == 0) {
	  mp->first = p->next;
	} else {
	  p1->next = p->next;
	}
	return b;
      } else {
	p->size -= size;
	return (b + p->size);
      }
    }
#ifdef __DEBUG__
    fd32_log_printf("No: 0x%lx < 0x%lx...", p->size, size);
#endif

  }
  error("PMM alloc: not enough memory\n");
  return 0;
}

int pmm_free(struct mempool *mp, DWORD base, DWORD size)
{
  struct memheader *h, *p, *p1;
  DWORD b;

  if (mp->first == 0) {
    h = (struct memheader *)base;
    h->size = size;
    h->next = 0;
  
    mp->free += size;
    mp->first = h;

    return 1;
  }

  for (p1 = 0, p = mp->first; p!= 0; p1 = p, p = p->next) {
    b = (DWORD)p;

    if (base <= b + p->size) {
      if (base == b + p->size) {
        if (base + size == (DWORD)(p->next)) {
          p->size += (size + p->next->size);
	  p->next = p->next->next;
	  mp->free += size;
	  return 1;
	} else {
	  if (base + size > (DWORD)p->next) {
            /* Error: there is an overlap... */
            message("Free Chunk: %lx ---\n", (DWORD)p->next);
            message("Region to Free: %lx - %lx\n", base, base + size);
            error("PMM Free: strange overlap...\n");
            fd32_abort();
	  }
	  p->size +=size;
	  mp->free += size;
	  return 1;
	}
      }
      /* Here, base < b + p->size... */
      
      /*
        Sanity check: The region to free must not overlap with any free chunk...
       */
      if (base + size > b) {
        /* Error: there is an overlap... */
        message("Free Chunk: %lx - %lx\n", b, b + p->size);
        message("Region to Free: %lx - %lx\n", base, base + size);
        error("PMM Free: strange overlap...\n");
        fd32_abort();
      }
      if (base + size == b) {
	h = (struct memheader *)base;
	h->size = size + p-> size;
	mp->free += size;
	if (p1 == 0) {
	  mp->first = h;
	} else {
	  p1->next = h;
	}
	h->next = p->next;

	return 1;
      }

      h = (struct memheader *)base;
      h->size = size;
      h->next = p;
      if (p1 == 0) {
	mp->first = h;
      } else {
	p1->next = h;
      }
      mp->free += size;
      
      return 1;
    }
  }

  h = (struct memheader *)base;
  h->size = size;
  h->next = 0;
  
  mp->free += size;
  p1->next = h;
  return 1;
}
