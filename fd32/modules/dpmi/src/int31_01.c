#include <ll/i386/hw-data.h>
#include <ll/i386/error.h>
#include "kmem.h"

#include "dpmi.h"
#include "ldtmanag.h"
#include "int31_01.h"

/* DPMI 0.9+ - ALLOCATE DOS MEMORY BLOCK
 */
void int31_0100(union regs *r)
{
  DWORD dosmem;
  int dossel;

  /* NOTE: Assuming the dosmem is allocated at 0x10 boundary */
  dosmem = dosmem_get(r->x.bx<<4);
  if ((dosmem&0x0F) != 0) {
    message("[DPMI] INT 31H 0100 - Allocate DOS Memory Block not at 0x10 boundary!\n");
  }
  dosmem >>= 4;

  dossel = fd32_segment_to_descriptor(dosmem);
  if (dossel < 0) {
    SET_CARRY;
  } else {
    fd32_set_segment_limit(dossel, r->x.bx<<4);
    r->x.ax = dosmem;
    r->x.dx = dossel;
  }
}

/* DPMI 0.9+ - FREE DOS MEMORY BLOCK
 */
void int31_0101(union regs *r)
{
  DWORD dosmem, limit;
  int res;

  if ( (res = fd32_get_segment_base_address(r->x.dx, &dosmem)) >= 0) {
    
    if ( (res = fd32_get_segment_limit(r->x.dx, &limit)) >= 0) {
      dosmem_free(dosmem, limit);
    }
  }

  dpmi_return(res, r);
}
