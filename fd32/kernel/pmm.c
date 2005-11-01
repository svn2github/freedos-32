/* Luca's Simple & Buggy Physical Memory Manager (PMM)
 * It is currently used in FD/32 only to compile the kernel,
 * while waiting for a better solution...
 *
 * Copyright (C) 2002,2003,2005 by Luca Abeni
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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
  struct memheader *p, *pnew, *pprev;
  DWORD size2, b, b_end, new_start;

#ifdef __DEBUG__
  fd32_log_printf("[PMM] Alloc Address  0x%lx 0x%lx...\n", base, size);
#endif
  for (pprev = 0, p = mp->first; p!= 0; pprev = p, p = p->next) {
    b = (DWORD)p;
#ifdef __DEBUG__
    fd32_log_printf("        Block 0x%lx-0x%lx\n", b, b + p->size);
#endif

    b_end = b+p->size;
    new_start = b_end-size;
    if (new_start >= base && base >= b) {
      /* OK, we have to remove from this chunk... */
      size2 = (b_end - (base + size));
      if (size2 < sizeof(struct memheader)) {
        if (size2 != 0)
          message("[PMM] alloc address: WARNING - loosing %ld bytes\n", size2);
        if (b >= base - sizeof(struct memheader)) {
          if (b != base)
            message("[PMM] alloc address: WARNING - loosing %ld bytes\n", base - b);
          if (pprev == 0) {
            mp->first = p->next;
          } else {
            pprev->next = p->next;
          }
        } else {
          p->size -= size;
        }
      } else {
        pnew = (struct memheader *)(base + size);
        pnew->size = size2;
        pnew->next = p->next;
        if (b >= base - sizeof(struct memheader)) {
          if (b != base)
            message("[PMM] alloc address: WARNING - loosing %ld bytes\n", base - b);
          if (pprev == 0) {
            mp->first = pnew;
          } else {
            pprev->next = pnew;
          }
        } else {
          p->size = (base - b);
          p->next = pnew;
        }
      }
      return 1;
    }
    
    /* Old check for MEM_AT: if ((b <= base) && ((b + p->size) >= (base + size))) */
  }
  error("[PMM] alloc address: memory not available...\n");
  return -1;
}

/* Allocate memory from a certain address (by Hanzac Chen) */
DWORD pmm_alloc_from(struct mempool *mp, DWORD base, DWORD size)
{
  struct memheader *p, *pnew, *pprev;
  DWORD size2, b, b_end, new_start;

  for (pprev = 0, p = mp->first; p!= 0; pprev = p, p = p->next) {
    b = (DWORD)p;
    b_end = b+p->size;
    new_start = b_end-size;
    if (new_start >= b && new_start >= base) {
        if (b >= base) {
          new_start = b;
          pnew = (struct memheader *)(new_start + size);
          size2 = b_end-(DWORD)pnew;
        } else {
          pnew = (struct memheader *)b;
          size2 = new_start-b;
        }
        if (size2 < sizeof(struct memheader)) {
          message("[PMM] alloc address: WARNING - loosing %ld bytes\n", size2);
          if (pprev == 0) {
            mp->first = p->next;
          } else {
            pprev->next = p->next;
          }
        } else {
          pnew->size = size2;
          pnew->next = p->next;
          if (pprev == 0) {
            mp->first = pnew;
          } else {
            pprev->next = pnew;
          }
        }
        return new_start;
    }
  }
  
  return 0;
}

DWORD pmm_alloc(struct mempool *mp, DWORD size)
{
  struct memheader *p, *p1, *p2;
  DWORD size2, b;

#ifdef __DEBUG__
  fd32_log_printf("[PMM] Alloc 0x%lx...\n", size);
#endif
  for (p1 = 0, p = mp->first; p!= 0; p1 = p, p = p->next) {
    b = (DWORD)p;
#ifdef __DEBUG__
    fd32_log_printf("        Block 0x%lx-0x%lx\n", b, b + p->size);
#endif
    if (p->size >= size) {
#ifdef __DEBUG__
      fd32_log_printf("        Found: 0x%lx\n", b + p->size - size);
#endif
      /* OK, we found enough memory */
      size2 = (p->size - size);
      if (size2 < sizeof(struct memheader)) {
        if (size2 != 0) {
          message("[PMM] alloc: WARNING - loosing %ld bytes\n", size2);
        }
        if (p1 == 0) {
          mp->first = p->next;
        } else {
            p1->next = p->next;
        }
        return b;
      } else {
#ifdef __ALLOC_FROM_END__
        p->size -= size;
        return (b + p->size);
#else
        struct memheader tmp;
        p2 = (struct memheader *)(b + size);
        tmp.next = p->next;
        tmp.size = p->size - size;
        p2->next = tmp.next;
        p2->size = tmp.size;
        if (p1 == 0) {
          mp->first = p2;
        } else {
          p1->next = p2;
        }
        return b;
#endif
      }
    }
#ifdef __DEBUG__
    fd32_log_printf("        No: 0x%lx < 0x%lx...", p->size, size);
#endif
  }
  error("[PMM] alloc: not enough memory...\n");
  pmm_dump(mp);
  message("(Want %lu 0x%lx)\n", size, size);
  return 0;
}

int pmm_free(struct mempool *mp, DWORD base, DWORD size)
{
  struct memheader *h, *p, *p1, tmp;
  DWORD b;
  
#ifdef __DEBUG__
  fd32_log_printf("[PMM] Free 0x%lx 0x%lx...\n", base, size);
#endif
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
          if ((base + size > (DWORD)p->next) && ((DWORD)p->next != 0)) {
            /* Error: there is an overlap... */
            message("Free Chunk: %lx ---\n", (DWORD)p->next);
            message("Region to Free: %lx - %lx\n", base, base + size);
            message("Overlapping region: %lx - %lx\n", b, b + p->size);
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
       * Sanity check: The region to free must not overlap with
       * any free chunk...
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
        tmp.size = size + p->size;
        tmp.next = p->next;
        mp->free += size;
        if (p1 == 0) {
          mp->first = h;
        } else {
          p1->next = h;
        }
        h->next = tmp.next;
        h->size = tmp.size;
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

void pmm_dump(struct mempool *mp)
{
  struct memheader *p;

  for (p = mp->first; p!= 0; p = p->next) {
#ifdef __DEBUG__
    fd32_log_printf("Memory Chunk: %lx ---> %lx (%ld ---> %ld)\n",
		(DWORD)p, (DWORD)p + p->size,
		(DWORD)p, (DWORD)p + p->size);
#endif
    message("Memory Chunk: %lx ---> %lx (%ld ---> %ld)\n",
		(DWORD)p, (DWORD)p + p->size,
		(DWORD)p, (DWORD)p + p->size);
  }
}
