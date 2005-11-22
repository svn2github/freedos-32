/* FD32 DPMI services 0x00??
 * by Salvo Isaja & Luca Abeni
 *
 * This file contains parts based on CWSDPMI
 * Copyright (C) 1995,1996 CW Sandmann (sandmann@clio.rice.edu)
 * 1206 Braelinn, Sugarland, TX 77479
 * Copyright (C) 1993 DJ Delorie, 24 Kirsten Ave, Rochester NH 03867-2954
 */
 
#include<ll/i386/hw-data.h>
#include<ll/i386/hw-instr.h>
#include<ll/i386/error.h>
#include<ll/i386/cons.h>
#include<logger.h>
#include "dpmi.h"
#include "kernel.h"

#include "int31_00.h"
#include "ldtmanag.h"

/* Descriptors Management Routines
 *
 * Implemented routines:
 *   DPMI 0.9 service 0000h - Allocate LDT Descriptors
 *   DPMI 0.9 service 0001h - Free LDT Descriptor
 *   DPMI 0.9 service 0002h - Segment to Descriptor
 *   DPMI 0.9 service 0003h - Get Selector Increment Value
 *   DPMI 0.9 service 0006h - Get Segment Base Address
 *   DPMI 0.9 service 0007h - Set Segment Base Address
 *   DPMI 0.9 service 0008h - Set Segment Limit
 *   DPMI 0.9 service 0009h - Set Descriptor Access Rights
 *   DPMI 0.9 service 000Ah - Create Alias Descriptor
 *   DPMI 0.9 service 000Bh - Get Descriptor
 *   DPMI 0.9 service 000Ch - Set Descriptor
 *
 * Not yet implemented routines:
 *   DPMI 0.9 service 000Dh - Allocate Specific LDT Descriptor
 *   DPMI 1.0 service 000Eh - Get Multiple Descriptors
 *   DPMI 1.0 service 000Fh - Set Multiple Descriptors
 *
 * FIX ME: GDT is currently used instead of the LDT
 *
 * NOTE: The 000Dh function, "Allocate Specific LDT Descriptor", receive
 *       a specific selector from applications. It is supposed to be a
 *       selector of the first 10h (16) descriptors in the LDT, so this
 *       function should not work with the current GDT use.
 */


#include<ll/i386/hw-func.h>

extern WORD kern_CS, kern_DS;

/* The following routines translate the DPMI calls, which use the CPU
 * registers for input and output data, into FD32 function calls,
 * to the functions above.
 */

void int31_0000(union regs *r)
{
  int  newsel;
	
  newsel = fd32_allocate_descriptors((WORD) r->d.ecx);

#ifdef __DEBUG__
  fd32_log_printf("   Base selector: 0x%hx   Exit code: %d\n",
                  (WORD)newsel, newsel);
#endif

  /* Return the result in AX */
  dpmi_return(newsel, r);
}


void int31_0001(union regs *r)
{
  int ErrorCode;

  /* If trying to free the application's CS or DS, do nothing!!! */
  /* FIXME: WTF is DJGPP Trying to do such a funny thing??? */
  /* FIXME: Maybe this check makes the check of kern_CS/kern_DS useless... */
  if (((WORD)(r->d.ebx) == r->x.cs) || ((WORD)(r->d.ebx) == r->x.ds)) {
    CLEAR_CARRY;
    return;
  }
  /* Under DPMI 1.0 hosts, any segment registers which contain
   * the selector being freed are zeroed by this function. (from DPMI api spec.)
   */
  if (r->x.es == r->x.bx) r->d.ees = 0;
  if (r->x.fs == r->x.bx) r->d.efs = 0;
  if (r->x.gs == r->x.bx) r->d.egs = 0;
  ErrorCode = fd32_free_descriptor((WORD) r->d.ebx);

  dpmi_return(ErrorCode, r);
}


void int31_0002(union regs *r)
{
  int  newsel;

  newsel = fd32_segment_to_descriptor(r->x.bx);

#ifdef __DEBUG__
  fd32_log_printf("   Real Mode selector: 0x%hx   Exit code: %d\n",
                  (WORD)newsel, newsel);
#endif

  dpmi_return(newsel, r);
}


void int31_0003(union regs *r)
{
  r->x.ax = fd32_get_selector_increment_value();
  CLEAR_CARRY;
}

	
void int31_0006(union regs *r)
{
  int ErrorCode;
  DWORD BaseAddress;

  ErrorCode = fd32_get_segment_base_address((WORD) r->d.ebx, &BaseAddress);

#ifdef __DEBUG__
  fd32_log_printf("   Base address: 0x%lx   Exit code: 0x%x\n",
                  BaseAddress, ErrorCode);
#endif

  dpmi_return(ErrorCode, r);
  if (ErrorCode >= 0) {
    r->x.dx = (WORD) BaseAddress;
    r->x.cx = BaseAddress >> 16;
  }
}


void int31_0007(union regs *r)
{
  int  ErrorCode;
  
  /* If trying to free the kernel CS or DS, do nothing!!! */
  if (((WORD)(r->d.ebx) == kern_CS) || ((WORD)(r->d.ebx) == kern_DS)) {
    CLEAR_CARRY;
    return;
  }
  
  ErrorCode = fd32_set_segment_base_address((WORD) r->d.ebx,
					    (((WORD) r->d.ecx << 16) + 
					     (WORD) r->d.edx));

  dpmi_return(ErrorCode, r);
}


void int31_0008(union regs *r)
{
  int  ErrorCode;

  /* If trying to free the kernel CS or DS, do nothing!!! */
  if (((WORD)(r->d.ebx) == kern_CS) || ((WORD)(r->d.ebx) == kern_DS)) {
    CLEAR_CARRY;
    return;
  }
  
  ErrorCode = fd32_set_segment_limit((WORD) r->d.ebx,(((WORD) r->d.ecx << 16) +
	(WORD) r->d.edx));

  dpmi_return(ErrorCode, r);
}


void int31_0009(union regs *r)
{
  int  ErrorCode;

  /* If trying to free the kernel CS or DS, do nothing!!! */
  if (((WORD)(r->d.ebx) == kern_CS) || ((WORD)(r->d.ebx) == kern_DS)) {
    CLEAR_CARRY;
    return;
  }
  ErrorCode = fd32_set_descriptor_access_rights((WORD) r->d.ebx,
						(WORD) r->d.ecx);

  dpmi_return(ErrorCode, r);
}


void int31_000A(union regs *r)
{
  int  aliasel;

  aliasel = fd32_create_alias_descriptor((WORD) r->d.ebx);

#ifdef __DEBUG__
  fd32_log_printf("   New selector: 0x%hx   Exit code: %d\n",
                  (WORD)aliasel, aliasel);
#endif

  dpmi_return(aliasel, r);
}


void int31_000B(union regs *r)
{
  int  ErrorCode;

  ErrorCode = fd32_get_descriptor((WORD) r->d.ebx, (WORD) r->d.ees, r->d.edi);

  dpmi_return(ErrorCode, r);
}


void int31_000C(union regs *r)
{
  int  ErrorCode;

  /* If trying to free the kernel CS or DS, do nothing!!! */
  if (((WORD)(r->d.ebx) == kern_CS) || ((WORD)(r->d.ebx) == kern_DS)) {
    CLEAR_CARRY;
    return;
  }
  
  ErrorCode = fd32_set_descriptor((WORD) r->d.ebx, (WORD) r->d.ees, r->d.edi);

  dpmi_return(ErrorCode, r);
}
