#ifndef __FD32_DRENV_MEM_H
#define __FD32_DRENV_MEM_H

#include<sys/movedata.h>
#include<sys/segments.h>
#include<go32.h> /* for _dos_ds */
#include<dpmi.h>

#define LOWMEM_ADDR int
#define DOS_SELECTOR _dos_ds


extern inline LOWMEM_ADDR fd32_dosmem_get(int size, WORD *rm_segment, WORD *rm_offset)
{
  LOWMEM_ADDR buf_selector;

  *rm_segment =__dpmi_allocate_dos_memory((size + 15) >> 4, &buf_selector);
  *rm_offset = 0;

  return buf_selector;
}


extern inline void fd32_dosmem_free(LOWMEM_ADDR buf_selector, int size)
{
  __dpmi_free_dos_memory(buf_selector); 
}


extern inline void fd32_memcpy_from_lowmem(void *destination, LOWMEM_ADDR source, DWORD source_offset, int size)
{
  movedata(source, source_offset, _my_ds(), (unsigned)destination, size);
}


extern inline void fd32_memcpy_to_lowmem(LOWMEM_ADDR destination, DWORD destination_offset, void *source, int size)
{
  movedata(_my_ds(), (DWORD)source, (unsigned) destination, destination_offset, size);
}


#endif /* #ifndef __FD32_DRENV_MEM_H */

