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

/*
#define __DEBUG__
*/

/*
 * Error codes for Descriptor Management Routines
 * I've set them the same as the DPMI error codes
 */
#define NO_ERROR                      0
#define ERROR_DESCRIPTOR_UNAVAILABLE  0x8011
#define ERROR_INVALID_SELECTOR        0x8022


/* DPMI error codes */
#define DPMI_DESCRIPTOR_UNAVAILABLE   0x8011
#define DPMI_INVALID_SELECTOR         0x8022


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
  int  ErrorCode;
  WORD BaseSelector;
	
  ErrorCode = fd32_allocate_descriptors((WORD) r->d.ecx, &BaseSelector);

#ifdef __DEBUG__
  fd32_log_printf("   Base selector: 0x%x   Exit code: 0x%x\n",
                  BaseSelector, ErrorCode);
#endif

  /* Return the result in AX */
  r->d.eax &= 0xFFFF0000;

  switch (ErrorCode) {
    case NO_ERROR :
      r->d.eax += BaseSelector;
      CLEAR_CARRY;
      break;
    case ERROR_DESCRIPTOR_UNAVAILABLE :
      r->d.eax += DPMI_DESCRIPTOR_UNAVAILABLE;
      SET_CARRY;
      break;
    default:
      /* Should never reach this point */
      error("Unknown result");
      fd32_abort();
  }
}


void int31_0001(union regs *r)
{
  int ErrorCode;

  /* If trying to free the kernel CS or DS, do nothing!!! */
  if (((WORD)(r->d.ebx) == kern_CS) || ((WORD)(r->d.ebx) == kern_DS)) {
    CLEAR_CARRY;
    return;
  }
  ErrorCode = fd32_free_descriptor((WORD) r->d.ebx);

  if (ErrorCode == ERROR_INVALID_SELECTOR) {
    r->d.eax &= 0xFFFF0000;
    r->d.eax += DPMI_INVALID_SELECTOR;
    SET_CARRY;
  } else {
    CLEAR_CARRY;
  }
}


void int31_0002(union regs *r)
{
  int  ErrorCode;
  WORD RealModeSelector;
	
  ErrorCode = fd32_allocate_descriptors((WORD) r->d.ebx, &RealModeSelector);

#ifdef __DEBUG__
  fd32_log_printf("   Real Mode selector: 0x%x   Exit code: 0x%x\n",
                  RealModeSelector, ErrorCode);
#endif

  /* Return the result in AX */
  r->d.eax &= 0xFFFF0000;

  switch (ErrorCode)
  {
    case NO_ERROR :
      r->d.eax += RealModeSelector;
      CLEAR_CARRY;
      break;
    case ERROR_DESCRIPTOR_UNAVAILABLE :
      r->d.eax += DPMI_DESCRIPTOR_UNAVAILABLE;
      SET_CARRY;
      break;
  }
}


void int31_0003(union regs *r)
{
  r->d.eax &= 0xFFFF0000;
  r->d.eax += fd32_get_selector_increment_value();
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

  switch (ErrorCode) {
    case NO_ERROR :
      r->d.edx &= 0xFFFF0000;
      r->d.ecx &= 0xFFFF0000;
      r->d.edx += (WORD) BaseAddress;
      r->d.ecx += BaseAddress >> 16;
      CLEAR_CARRY;
      break;

    case ERROR_INVALID_SELECTOR :
      r->d.eax &= 0xFFFF0000;
      r->d.eax += DPMI_INVALID_SELECTOR;
      SET_CARRY;
      break;
  }
  return;
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

  switch (ErrorCode) {
    case NO_ERROR:
      CLEAR_CARRY;
      break;

    case ERROR_INVALID_SELECTOR:
      r->d.eax &= 0xFFFF0000;
      r->d.eax += DPMI_INVALID_SELECTOR;
      SET_CARRY;
      break;
  }
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

  switch (ErrorCode) {
    case NO_ERROR:
      CLEAR_CARRY;
      break;
    case ERROR_INVALID_SELECTOR:
      r->d.eax &= 0xFFFF0000;
      r->d.eax += DPMI_INVALID_SELECTOR;
      SET_CARRY;
      break;
  }
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

  switch (ErrorCode) {
    case NO_ERROR:
      CLEAR_CARRY;
      break;

    case ERROR_INVALID_SELECTOR:
      r->d.eax &= 0xFFFF0000;
      r->d.eax += DPMI_INVALID_SELECTOR;
      SET_CARRY;
      break;
  }
}


void int31_000A(union regs *r)
{
  int  ErrorCode;
  WORD NewSelector;

  ErrorCode = fd32_create_alias_descriptor((WORD) r->d.ebx, &NewSelector);

#ifdef __DEBUG__
  fd32_log_printf("   New selector: 0x%x   Exit code: 0x%x\n",
                  NewSelector, ErrorCode);
#endif

  /* Return the result in AX */
  r->d.eax &= 0xFFFF0000;
  switch (ErrorCode) {
    case NO_ERROR:
      r->d.eax += NewSelector;
      CLEAR_CARRY;
      break;

    case ERROR_DESCRIPTOR_UNAVAILABLE:
      r->d.eax += DPMI_DESCRIPTOR_UNAVAILABLE;
      SET_CARRY;
      break;

    case ERROR_INVALID_SELECTOR:
      r->d.eax += DPMI_INVALID_SELECTOR;
      SET_CARRY;
      break;
  }
}


void int31_000B(union regs *r)
{
  int  ErrorCode;

  ErrorCode = fd32_get_descriptor((WORD) r->d.ebx, (WORD) r->d.ees, r->d.edi);

  switch (ErrorCode)
  {
    case NO_ERROR:
      CLEAR_CARRY;
      break;

    case ERROR_INVALID_SELECTOR:
      r->d.eax &= 0xFFFF0000;
      r->d.eax += DPMI_INVALID_SELECTOR;
      SET_CARRY;
      break;
  }
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

  switch (ErrorCode)
  {
    case NO_ERROR:
      CLEAR_CARRY;
      break;

    case ERROR_INVALID_SELECTOR:
      r->d.eax &= 0xFFFF0000;
      r->d.eax += DPMI_INVALID_SELECTOR;
      SET_CARRY;
      break;
  }
}

