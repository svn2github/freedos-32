/* FD/32 Initialization Code
 * by Luca Abeni
 *
 * This is free software; see GPL.txt
 */
#include <ll/i386/error.h>
#include <ll/i386/mem.h>

#include "init.h"
#include "kernel.h"
#include "kmem.h"
#include "pmm.h"

struct mempool dos_mempool;

void system_init(void *mbp)
{
  /* The interrupt handlers are in the drivers, now!!! */
  /* (and exception handlers are set by kernel_init() */

  mem_init(mbp);
  kernel_init();
}

void dos_init(DWORD membase, DWORD memsize)
{
  if (membase == 0) {
	  /* Randomly chosen safe value :) */
    membase = 64 * 1024;
  }
  pmm_init(&dos_mempool);
  pmm_add_region(&dos_mempool, membase, memsize);
}
