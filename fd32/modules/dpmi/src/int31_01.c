#include <ll/i386/hw-data.h>
#include <ll/i386/error.h>

#include "dpmi.h"
#include "dosmem.h"
#include "ldtmanag.h"
#include "int31_01.h"

/* DPMI 0.9+ - ALLOCATE DOS MEMORY BLOCK
 */
void int31_0100(union regs *r)
{
  WORD dosseg;

  /* NOTE: Assuming the dosmem is allocated at 0x10 boundary */
  dosseg = dos_alloc(r->x.bx);
  if (dosseg != 0) {
    int dossel = fd32_segment_to_descriptor(dosseg);
    if (dossel >= 0) {
      fd32_set_segment_limit(dossel, r->x.bx<<4);
      r->x.ax = dosseg;
      r->x.dx = dossel;
    } else {
      SET_CARRY;
    }
  } else {
    SET_CARRY;
  }
}

/* DPMI 0.9+ - FREE DOS MEMORY BLOCK
 */
void int31_0101(union regs *r)
{
  DWORD dosmem;
  int res;

  if ( (res = fd32_get_segment_base_address(r->x.dx, &dosmem)) >= 0) {
    fd32_free_descriptor(r->x.dx);
    dos_free(dosmem>>4);
  }

  dpmi_return(res, r);
}
