/* Luca's Simple & Buggy Physical Memory Manager (PMM)
 * It is currently used in FD/32 only to compile the kernel,
 * while waiting for a better solution...
 *
 * Copyright (C) 2002,2003,2005 by Luca Abeni
 * Copyright (C) 2005 by Hanzac Chen
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

/* #define __DEBUG__ */

#include "pmm.h"
#include "kernel.h"
#include "logger.h"

void pmm_init(struct mempool *mp)
{
  mp->top.size = 0; /* Free memory size */
  mp->top.next = 0;
}

int pmm_alloc_address(struct mempool *mp, DWORD base, DWORD size)
{
  struct memheader *p, *pnew, *pprev;
  DWORD lsize, start, end;

#ifdef __DEBUG__
  fd32_log_printf("[PMM] Alloc Address  0x%lx 0x%lx...\n", base, size);
#endif
  for (pprev = &mp->top, p = pprev->next; p!= 0; pprev = p, p = pprev->next) {
    start = (DWORD)p;
#ifdef __DEBUG__
    fd32_log_printf("        Block 0x%lx-0x%lx\n", start, start + p->size);
#endif
    end = start+p->size;
    if ( end-size >= base && base >= start) {
      lsize = base-start;
      if (lsize < sizeof(struct memheader)) {
        mp->top.size -= lsize;
        if (lsize != 0)
          message("[PMM] alloc address: WARNING - losing %ld bytes\n", lsize);
        pprev->next = p->next;
        p = pprev;
      } else {
        pnew = (struct memheader *)start;
        pnew->size = lsize;
        pnew->next = p->next;
        pprev->next = pnew;
        p = pnew;
      }

      lsize = end-(base+size);
      if (lsize < sizeof(struct memheader)) {
        mp->top.size -= lsize;
        if (lsize != 0)
          message("[PMM] alloc address: WARNING - loosing %ld bytes\n", lsize);
      } else {
        pnew = (struct memheader *)(base+size);
        pnew->size = lsize;
        pnew->next = p->next;
        p->next = pnew;
      }

      mp->top.size -= size;
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
  DWORD lsize, start, end, new_start;

  for (pprev = &mp->top, p = pprev->next; p!= 0; pprev = p, p = pprev->next) {
    start = (DWORD)p;
    end = start+p->size;
    new_start = end-size;
    if (new_start >= start && new_start >= base) {
      if (start >= base) {
        new_start = start;
        pnew = (struct memheader *)(start + size);
        lsize = end-(DWORD)pnew;
      } else {
        pnew = (struct memheader *)start;
        lsize = new_start-start;
      }
      if (lsize < sizeof(struct memheader)) {
        message("[PMM] alloc address: WARNING - loosing %ld bytes\n", lsize);
        pprev->next = p->next;
      } else {
        pnew->size = lsize;
        pnew->next = p->next;
        pprev->next = pnew;
      }
      return new_start;
    }
  }

  return 0;
}

DWORD pmm_alloc(struct mempool *mp, DWORD size)
{
  struct memheader *p, *pnew, *pprev;
  DWORD lsize, start;

#ifdef __DEBUG__
  fd32_log_printf("[PMM] Alloc 0x%lx...\n", size);
#endif
  for (pprev = &mp->top, p = pprev->next; p!= 0; pprev = p, p = pprev->next) {
    start = (DWORD)p;
#ifdef __DEBUG__
    fd32_log_printf("        Block 0x%lx-0x%lx", start, start + p->size);
#endif
    if (p->size >= size) { /* OK, we found enough memory */
#ifdef __DEBUG__
      fd32_log_printf("        Found: 0x%lx\n", start);
#endif
      lsize = (p->size - size);
      if (lsize < sizeof(struct memheader)) {
        mp->top.size -= lsize;
        if (lsize != 0)
          message("[PMM] alloc: WARNING - loosing %ld bytes\n", lsize);
        pprev->next = p->next;
      } else {
#ifdef __ALLOC_FROM_END__
        p->size -= size;
        return (start + p->size);
#else
        pnew = (struct memheader *)(start + size);
        pnew->next = p->next; /* NOTE: if the size < 8 bytes, will cause memory corruption, so use lsize (actually it's faster too) */
        pnew->size = lsize;
        pprev->next = pnew;
#endif
      }
      mp->top.size -= size;
      return start;
    }
#ifdef __DEBUG__
    fd32_log_printf("        No: 0x%lx < 0x%lx...\n", p->size, size);
#endif
  }
  error("[PMM] alloc: not enough memory... "); message("need %lx bytes\n", size);
  pmm_dump(mp);
  return 0;
}

static void local_pmm_error_overlap(DWORD base, DWORD size, DWORD o_base, DWORD o_size, DWORD f_base, DWORD f_size)
{
  message("Free Chunk: %lx - %lx\n", f_base, f_base + f_size);
  message("Region to Free: %lx - %lx\n", base, base + size);
  message("Overlapping region: %lx - %lx\n", o_base, o_base + o_size);
  error("[PMM] Free: strange overlap...\n");
  fd32_abort();
}

int pmm_free(struct mempool *mp, DWORD base, DWORD size)
{
  struct memheader *p, *pnew, *pprev;
  DWORD start, end;

#ifdef __DEBUG__
  fd32_log_printf("[PMM] Free 0x%lx 0x%lx...\n", base, size);
#endif

  for (pprev = &mp->top, p = pprev->next; p!= 0; pprev = p, p = pprev->next) {
    start = (DWORD)p;
    end = start + p->size;
    if (base == end) {
      pnew = p->next;
      if (pnew == 0) {
        p->size += size;
      } else if (base + size == (DWORD)pnew) {
        p->size += size + pnew->size;
        p->next = pnew->next;
      } else if (base + size < (DWORD)pnew) {
        p->size += size;
      } else {  /* Error: there is an overlap... */
        local_pmm_error_overlap(base, size, (DWORD)p, p->size, (DWORD)pnew, pnew->size);
      }
      break;
    } else if (base < end) {
      /*
       * Sanity check: The region to free must not overlap with
       * any free chunk...
       */
      if (base + size <= start) {
        pnew = (struct memheader *)base;
        if (base + size == start) {
          /* Merge the next entry if adjacent */
          size += p->size;
          p = p->next;
        }
        pnew->next = p;
        pnew->size = size;
        pprev->next = pnew;
      } else {  /* Error: there is an overlap... */
        local_pmm_error_overlap(base, size, (DWORD)p, p->size, (DWORD)p, p->size);
      }
      break;
    }
  }
  if (p == 0) {
    pnew = (struct memheader *)base;
    pnew->next = 0;
    pnew->size = size;
    pprev->next = pnew;
  }

  mp->top.size += size;
  return 1;
}

void pmm_dump(struct mempool *mp)
{
  struct memheader *p;

  for (p = mp->top.next; p!= 0; p = p->next) {
#ifdef __DEBUG__
    fd32_log_printf("Memory Chunk: 0x%lx ---> 0x%lx (%ld ---> %ld)\n",
		(DWORD)p, (DWORD)p + p->size,
		(DWORD)p, (DWORD)p + p->size);
#endif
    message("Memory Chunk: 0x%lx ---> 0x%lx (%ld ---> %ld)\n",
		(DWORD)p, (DWORD)p + p->size,
		(DWORD)p, (DWORD)p + p->size);
  }
}
