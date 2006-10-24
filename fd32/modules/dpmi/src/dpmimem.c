/* DPMI Driver for FD32: memory management functions
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/hw-data.h>
#include <ll/string.h>

#include <kmem.h>
#include <kernel.h>
#include <exec.h>

#include "dpmimem.h"


/*
#define __DPMIMEM_DEBUG__
*/

DWORD dpmi_get_mem(DWORD addr, DWORD size)
{
  struct dpmimem_info *top = (struct dpmimem_info *)fd32_get_current_pi()->mem_info;
  struct dpmimem_info *p;
  DWORD area;
  int res;

  area = addr - sizeof(struct dpmimem_info);
  res = mem_get_region(area, size + sizeof(struct dpmimem_info));
  if (res == -1) {
    return 0;
  }
  
  /* Fill the memory info header... */
  p = (struct dpmimem_info *)area;
  p->signature = MEMSIG;
  p->size = size;

  /* Link */
  if (top != NULL) {
    p->next = top->next;
    top->next = p;
  }

  return area;
}

DWORD dpmi_alloc(DWORD size, DWORD *base)
{
  struct dpmimem_info *top = (struct dpmimem_info *)fd32_get_current_pi()->mem_info;
  struct dpmimem_info *p;
  DWORD area;

#ifdef __DPMIMEM_DEBUG__
  fd32_log_printf("DPMI Alloc 0x%lx\n", size);
#endif
  
/* area = mem_get_region_after(fd32_get_current_pi()->memlimit, size + sizeof(struct dpmimem_info));
  if (area == 0) {
    return 0;
  }
  fd32_get_current_pi()->memlimit = area + (size + sizeof(struct dpmimem_info));
*/

  /* NOTE: Use a normal mem_get 'cause DJGPP libc can support memory anywhere
   *       althought it maybe result a negative memory address in an DPMI environment ...
   */
  area = mem_get(size + sizeof(struct dpmimem_info));
  if (area == 0) {
    return 0;
  }
  
  /* Fill the memory info header... */
  p = (struct dpmimem_info *)area;
  p->signature = MEMSIG;
  p->size = size;

  /* Link */
  if (top != NULL) {
    p->next = top->next;
    top->next = p;
  }

  *base = area + sizeof(struct dpmimem_info);
  return area;
}

int dpmi_free(DWORD handle)
{
  struct dpmimem_info *top = (struct dpmimem_info *)fd32_get_current_pi()->mem_info;
  struct dpmimem_info *p, *q;

  if (handle == 0) {
    return -1;
  }
  p = (struct dpmimem_info *)handle;
  if (p->signature != MEMSIG) {
    return -1;
  }
  p->signature = 0;

  /* Remove link */
  if (top != NULL) {
    for (q = top; q != NULL; q = q->next)
      if (q->next == p)
      {
        q->next = p->next;
        break;
      }

    /* if (q == NULL) ...problem */
  }
  return mem_free(handle, p->size + sizeof(struct dpmimem_info));
}

void dpmi_clear_mem(struct dpmimem_info *top)
{
  struct dpmimem_info *p, *t;

  for (p = top->next; p != NULL; p = t)
  {
    t = p->next;
    mem_free((DWORD)p, p->size + sizeof(struct dpmimem_info));
  }
}
