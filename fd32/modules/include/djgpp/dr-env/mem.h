#ifndef __FD32_DRENV_MEM_H
#define __FD32_DRENV_MEM_H

#include<sys/movedata.h>
#include<sys/segments.h>
#include<go32.h> /* for _dos_ds */
#include<dpmi.h>

#define LOWMEM_ADDR  int /* A protected mode selector */
#define DOS_SELECTOR _dos_ds


extern inline LOWMEM_ADDR fd32_dosmem_get(unsigned size, WORD *rm_segment, WORD *rm_offset)
{
  int         seg;
  LOWMEM_ADDR buf_selector;

  seg = __dpmi_allocate_dos_memory((size + 15) >> 4, &buf_selector);
  if (seg < 0) return 0; /* Failed to allocate DOS memory */
  *rm_segment = (WORD) seg;
  *rm_offset  = 0;
  return buf_selector;
}


extern inline void fd32_dosmem_free(LOWMEM_ADDR buf_selector, unsigned size)
{
  __dpmi_free_dos_memory(buf_selector); 
}


extern inline LOWMEM_ADDR fd32_dmamem_get(unsigned size, WORD *rm_segment, WORD *rm_offset)
{
  DWORD       begin, end;
  int         seg;
  LOWMEM_ADDR buf_selector;

  /* Allocate size*2 bytes, so that if we cross a 64 KiB boundary,  */
  /* we add size to the base in order to get a non-crossing buffer. */
  seg = __dpmi_allocate_dos_memory((size * 2 + 15) >> 4, &buf_selector);
  if (seg < 0) return 0; /* Failed to allocate DOS memory */
  begin = seg << 4;
  end   = begin + size;
  if ((end & 0xFFFF) < (begin & 0xFFFF)) begin += size;
  *rm_segment = (WORD) (begin >> 4);
  *rm_offset  = (WORD) (begin & 0x000F);
  return buf_selector;
}


#define fd32_dmamem_free fd32_dosmem_free


extern inline void fd32_memcpy_from_lowmem(void *destination, LOWMEM_ADDR source, DWORD source_offset, unsigned size)
{
  movedata(source, source_offset, _my_ds(), (unsigned) destination, size);
}


extern inline void fd32_memcpy_to_lowmem(LOWMEM_ADDR destination, DWORD destination_offset, const void *source, unsigned size)
{
  movedata(_my_ds(), (DWORD) source, (unsigned) destination, destination_offset, size);
}


#endif /* #ifndef __FD32_DRENV_MEM_H */

