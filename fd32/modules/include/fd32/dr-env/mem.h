#ifndef __FD32_DRENV_MEM_H
#define __FD32_DRENV_MEM_H

#include<ll/i386/hw-data.h>
#include<ll/i386/x-dosmem.h>
#include<ll/i386/mem.h>

#define LOWMEM_ADDR  DWORD /* A physical buffer */
#define DOS_SELECTOR 0     /* Ignore it in flat address space */


extern inline LOWMEM_ADDR fd32_dosmem_get(unsigned size, WORD *rm_segment, WORD *rm_offset)
{
  LOWMEM_ADDR phys_addr;

  phys_addr   = (LOWMEM_ADDR) DOS_alloc(size);
  *rm_segment = phys_addr >> 4;
  *rm_offset  = phys_addr & 0x0F;
  return phys_addr;
}


extern inline void fd32_dosmem_free(LOWMEM_ADDR phys_addr, unsigned size)
{
  DOS_free((void *) phys_addr, size);
}


extern inline LOWMEM_ADDR fd32_dmamem_get(unsigned size, WORD *rm_segment, WORD *rm_offset)
{
  DWORD       begin, end;
  LOWMEM_ADDR phys_addr;

  /* Allocate size*2 bytes, so that if we cross a 64 KiB boundary,  */
  /* we add size to the base in order to get a non-crossing buffer. */
  phys_addr = (LOWMEM_ADDR) DOS_alloc(size * 2);
  if (phys_addr == 0) return 0; /* Failed to allocate DOS memory */
  begin = phys_addr;
  end   = begin + size;
  if ((end & 0xFFFF) < (begin & 0xFFFF)) begin += size;
  *rm_segment = (WORD) (begin >> 4);
  *rm_offset  = (WORD) (begin & 0x000F);
  return phys_addr;
}


extern inline void fd32_dmamem_free(LOWMEM_ADDR phys_addr, unsigned size)
{
  /* We actually allocated a size*2 buffer */
  DOS_free((void *) phys_addr, size * 2);
}


extern inline void fd32_memcpy_from_lowmem(void *destination, LOWMEM_ADDR source, DWORD source_offset, int size)
{
  memcpy(destination, ((BYTE *)source) + source_offset, size);
}


extern inline void fd32_memcpy_to_lowmem(LOWMEM_ADDR destination, DWORD destination_offset, const void *source, int size)
{
  memcpy(((BYTE *)destination) + destination_offset, source, size);
}


#endif /* #ifndef __FD32_DRENV_MEM_H */

