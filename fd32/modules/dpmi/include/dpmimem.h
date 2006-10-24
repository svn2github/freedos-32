#ifndef __DPMIMEM_H__
#define __DPMIMEM_H__

#include <ll/i386/hw-data.h>

#define MEMSIG 0xFD32

struct dpmimem_info {
  DWORD signature;
  DWORD size;

  struct dpmimem_info *next;
};

void dpmi_set_limit(DWORD l);
DWORD dpmi_get_mem(DWORD addr, DWORD size);
DWORD dpmi_alloc(DWORD size, DWORD *base);
int dpmi_free(DWORD handle);

void dpmi_clear_mem(struct dpmimem_info *top);

#endif    /* DPMIMEM_H */
