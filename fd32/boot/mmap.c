/* FD/32 Memory Map parsing code
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/hw-data.h>
#include <ll/i386/stdlib.h>
#include <ll/i386/cons.h>

#include "mmap.h"
#include "logger.h"

struct mmap {
  DWORD size;
  DWORD base_addr_low;
  DWORD base_addr_high;
  DWORD lenght_low;
  DWORD lenght_high;
  DWORD type;
};

void mmap_process(DWORD addr, DWORD len, DWORD *lb, DWORD *ls, DWORD *hb, DWORD *hs)
{
  struct mmap *p;
  int i;
  
  p = (struct mmap *) addr;
  i = 0;

  while (p < (struct mmap *)(addr + len)) {
#ifdef __MMAP_DEBUG__
    fd32_log_printf("[Memory Map] region %d: %lx:%lx - %lx:%lx ===> %lu\n", i,
	    p->base_addr_high, p->base_addr_low,
	    p->base_addr_high + p->lenght_high,
	    p->base_addr_low + p->lenght_low,
	    p->type);
#endif
    p = p + p->size + 4;
    i++;
  }
}

DWORD mmap_entry(DWORD addr, DWORD *bah, DWORD *bal, DWORD *lh, DWORD *ll, DWORD *type)
{
  struct mmap *p;
  int i;
  
  p = (struct mmap *) addr;
  i = 0;
  
  *bah = p->base_addr_high;
  *bal = p->base_addr_low;
  *lh = p->lenght_high;
  *ll = p->lenght_low;
  *type = p->type;

  /*
  p = p + p->size + 4;
  return (DWORD p);
  */
  return addr + p->size + 4;
}

