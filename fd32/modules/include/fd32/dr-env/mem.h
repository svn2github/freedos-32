#ifndef __FD32_DRENV_MEM_H
#define __FD32_DRENV_MEM_H

#include<ll/i386/hw-data.h>
#include<ll/i386/x-dosmem.h>
#include<ll/i386/mem.h>

#define LOWMEM_ADDR DWORD
#define DOS_SELECTOR 0

extern inline LOWMEM_ADDR fd32_dosmem_get(int size, WORD *rm_segment, WORD *rm_offset)
{
  LOWMEM_ADDR buf_selector;

  buf_selector = (DWORD)DOS_alloc(size);
  *rm_segment  = buf_selector >> 4;
  *rm_offset   = buf_selector & 0x0F;

  return buf_selector;
}


extern inline void fd32_dosmem_free(LOWMEM_ADDR buf_selector, int size)
{
  DOS_free((void *)buf_selector, size); 
}


extern inline void fd32_memcpy_from_lowmem(void *destination, LOWMEM_ADDR source, DWORD source_offset, int size)
{
  memcpy(destination, ((BYTE *)source) + source_offset, size);
}


extern inline void fd32_memcpy_to_lowmem(LOWMEM_ADDR destination, DWORD destination_offset, void *source, int size)
{
  memcpy(((BYTE *)destination) + destination_offset, source, size);
}


#endif /* #ifndef __FD32_DRENV_MEM_H */

