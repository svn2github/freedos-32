/* DPMI Driver for FD32: memory management functions
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/hw-data.h>
#include "kmem.h"
#include "dpmimem.h"
#include "stubinfo.h"
#include "kernel.h"

#define MEMSIG 0xFD32

/*
#define __DPMIMEM_DEBUG__
*/

extern struct psp *current_psp;

struct dpmimem_info {
  DWORD signature;
  DWORD size;
};

DWORD dpmi_get_mem(DWORD addr, DWORD size)
{
  struct dpmimem_info *p;
  DWORD area;
  int res;
  
  res = mem_get_region(addr - sizeof(struct dpmimem_info),
			size + sizeof(struct dpmimem_info));
  if (res == -1) {
    return 0;
  }
  area = addr - sizeof(struct dpmimem_info);
  
  /* Fill the memory info header... */
  p = (struct dpmimem_info *)area;
  p->signature = MEMSIG;
  p->size = size;

  return area;
}

DWORD dpmi_alloc(DWORD size, DWORD *base)
{
  struct dpmimem_info *p;
  DWORD area;
  int res;

#ifdef __DPMIMEM_DEBUG__
  fd32_log_printf("DPMI Alloc 0x%lx\n", size);
#endif
  if (current_psp != 0) {
#ifdef __DPMIMEM_DEBUG__
    fd32_log_printf("Current PSP: 0x%lx --- MemLimit: 0x%lx\n",
	    current_psp, current_psp->memlimit);
#endif
    res = mem_get_region(current_psp->memlimit,
			 size + sizeof(struct dpmimem_info));
    if (res == -1) {
      return 0;
    }
    area = current_psp->memlimit;
    current_psp->memlimit += (size + sizeof(struct dpmimem_info));
  } else {
    area = mem_get(size + sizeof(struct dpmimem_info));
  }
  
  /*
  area = mem_get(size + sizeof(struct dpmimem_info));
  */
  if (area == 0) {
    return 0;
  }
  
  /* Fill the memory info header... */
  p = (struct dpmimem_info *)area;
  p->signature = MEMSIG;
  p->size = size;

  *base = area + sizeof(struct dpmimem_info);
  return area;
}

int dpmi_free(DWORD handle)
{
  struct dpmimem_info *p;

  if (handle == 0) {
    return -1;
  }
  p = (struct dpmimem_info *)handle;
  if (p->signature != MEMSIG) {
    return -1;
  }
  p->signature = 0;
  return mem_free(handle, p->size + sizeof(struct dpmimem_info));
}
