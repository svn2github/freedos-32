/* FD/32 Memory Initialization
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

/* In-kernel static memory management... */
#ifdef __MEM_DEBUG__
#include <ll/i386/error.h>
#endif
#include <ll/i386/mb-info.h>
#include <ll/i386/hw-data.h>
#include <ll/i386/error.h>

#include "pmm.h"
#include "mmap.h"
#include "kmem.h"
#include "mods.h"
#include "logger.h"

struct mempool high_mem_pool;
struct mempool dos_mem_pool;

static void mem_init2(DWORD ks, DWORD ke, DWORD mstart, int mnum)
{
  int i;
  DWORD ms, me;
  DWORD start, end;

#ifdef __MEM_DEBUG__
  message("Mem Init_2:\n");
#endif

/* Here, we have to use some extra care...
 * If the kernel finishes immediately before the first module,
 * removing the kernel region will corrupt the module!!!
 * Trivial solution: remove a big block containing kernel & modules
 */
  start = ks; end = ke;
  for (i = 0; i < mnum; i++) {

    module_address(mstart, i, &ms, &me);
    if (me > end) {
	end = me;
    }
    if (ms < start) {
	start = ms;
    }
  }
#ifdef __MEM_DEBUG__
  message("Removing %lx - %lx:\n", start, end);
#endif
  pmm_remove_region(&high_mem_pool, start, end - start + 1);
}

static void mem_init_from_mmap(DWORD mmap_addr, WORD mmap_size)
{
  DWORD p;
  DWORD bah, bal, lh, ll, type;
  
#ifdef __MEM_DEBUG__
  message("Mem Init from mmap:\n");
#endif

  pmm_init(&high_mem_pool);
  p = mmap_addr;
  while (p < mmap_addr + mmap_size) {
#ifdef __MEM_DEBUG__
  message("Map Entry @%lx:\n", p);
#endif
    p = mmap_entry(p, &bah, &bal, &lh, &ll, &type);
    if ((bah != 0) || (lh != 0)) {
      error("Mem init panic:\n");
      message("base: high=%lx   low=%lx\n", bah, bal);
      message("len: high=%lx    low=%lx\n", lh, ll);
      return;
    }
#ifdef __MEM_DEBUG__
  message("Memory region: %lx - %lx Type %ld\n", bal, bal + ll, type);
#endif
    if (type == 1) {
      if (bal >= 0x100000) {
#ifdef __MEM_DEBUG__
	message("Adding region %lx - %lx (Type = %ld)\n", bal, bal + ll, type);
#endif
	pmm_add_region(&high_mem_pool, bal, ll);
      } else {
	/* DOS mem is not currently managed :( */
	/* FIXME: Implement something here... */
      }
    }
  }
}

static void mem_init_from_size(DWORD start, DWORD size)
{
#ifdef __MEM_DEBUG__
  message("Mem Init from size: %lx - %lx\n", start, start + size);
#endif

  pmm_init(&high_mem_pool);
  pmm_add_region(&high_mem_pool, start, size);
}

void mem_init(void *p)
{
  struct multiboot_info *mbp;
  extern DWORD _start;
  extern DWORD end;
  DWORD lsize, hsize, hbase;
  int mnum;
  DWORD mstart;

  mbp = p;
  if (mbp->flags & MB_INFO_MEM_MAP) {
#ifdef __MEM_DEBUG__
    message("\tMemory map provided\n");
#endif
    mem_init_from_mmap(mbp->mmap_addr, mbp->mmap_length);
  } else {
    if (mbp->flags & MB_INFO_MEMORY) {
#ifdef __MEM_DEBUG__
      message("\tMemory information OK\n");
#endif
      lsize = mbp->mem_lower * 1024;
      hsize = mbp->mem_upper * 1024;
#ifdef __MEM_DEBUG__
      message("Mem Lower: %lx %lu\n", lsize, lsize);
      message("Mem Upper: %lx %lu\n", hsize, hsize);
#endif
      hbase = 0x100000;
      if (mbp->flags & MB_INFO_BOOT_LOADER_NAME) {
#ifdef __MEM_DEBUG__
        message("Loader Name provided: %s\n", (char *)mbp->boot_loader_name);
#endif
        if (*((char *)(mbp->boot_loader_name)) == 'X') {
#if 0
          hbase = mbp->mem_upbase; /* Weirdness with X? */
#else
          hbase = 0x100000;
#endif
	}
      }

#ifdef __MEM_DEBUG__
      message("\t\tHigh Memory: %ld - %ld (%lx - %lx)\n", 
	      hbase, hbase + hsize,
	      hbase, hbase + hsize);
#endif
      mem_init_from_size(hbase, hsize);
    }
  }
  
  if (mbp->flags & MB_INFO_MODS) {
    mnum = mbp->mods_count;
    mstart = mbp->mods_addr;
  } else {
    mnum = 0;
    mstart = 0;
  }

  mem_init2((DWORD)(&_start), (DWORD)(&end), mstart, mnum);
}
    
void mem_release_module(DWORD mstart, int m, int mnum)
{
  DWORD ms, nms, me, ms1, dummy;
  int i;

  module_address(mstart, m, &ms, &me);
  nms = 0;
  for (i = 0; i < mnum; i++) {
    module_address(mstart, i, &ms1, &dummy);
    if (nms == 0) {
      if (ms1 > me) {
        nms = ms1;
      }
    } else {
      if ((ms1 > me) && (ms1 < nms)) {
        nms = ms1;
      }
    }
  }
  if (nms == 0) {
    nms = me + 1;
  }
#ifdef __MEM_DEBUG__
  fd32_log_printf("________\n");
  pmm_dump(&high_mem_pool);
  fd32_log_printf("\n");
  fd32_log_printf("releasing %lx - %lx\n", ms, nms);
#endif
  pmm_add_region(&high_mem_pool, ms, nms - ms);
#ifdef __MEM_DEBUG__
  pmm_dump(&high_mem_pool);
  fd32_log_printf("________\n");
#endif
}

void dosmem_init(DWORD base, DWORD size)
{
#ifdef __MEM_DEBUG__
  message("Initing dos mem: 0x%lx 0x%lx\n", base, size);
#endif
  pmm_init(&dos_mem_pool);
  pmm_add_region(&dos_mem_pool, base, size);
}

int dosmem_get_region(DWORD base, DWORD size)
{
  return pmm_alloc_address(&dos_mem_pool, base, size);
}

int dosmem_free(DWORD base, DWORD size)
{
  return pmm_free(&dos_mem_pool, base, size);
}

DWORD dosmem_get(DWORD amount)
{
  DWORD m;
  m = pmm_alloc(&dos_mem_pool, amount);
  return m;
}

int mem_get_region(DWORD base, DWORD size)
{
  return pmm_alloc_address(&high_mem_pool, base, size);
}

int mem_free(DWORD base, DWORD size)
{
  return pmm_free(&high_mem_pool, base, size);
}

DWORD mem_get(DWORD amount)
{
  return pmm_alloc(&high_mem_pool, amount);
}

void mem_dump(void)
{
  pmm_dump(&high_mem_pool);
  pmm_dump(&dos_mem_pool);
}
