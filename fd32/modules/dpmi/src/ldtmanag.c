/* FD32 LDT Management Services
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
 *   DPMI 1.0 service 000Eh - Get Multiple Descriptors
 *   DPMI 1.0 service 000Fh - Set Multiple Descriptors
 *
 * Not yet implemented routines:
 *   DPMI 0.9 service 000Dh - Allocate Specific LDT Descriptor
 *
 * FIX ME: GDT is currently used instead of the LDT
 *
 * NOTE: The 000Dh function, "Allocate Specific LDT Descriptor", receive
 *       a specific selector from applications. It is supposed to be a
 *       selector of the first 10h (16) descriptors in the LDT, so this
 *       function should not work with the current GDT use.
 */


#include<ll/i386/hw-func.h>

#include "ldtmanag.h"


#define RUN_RING   0      /* Not too sure */
#define USE_GDT           /* We use the GDT instead of the LDT */

#ifdef USE_GDT
  extern union gdt_entry *GDT_base;
  #define DESCRIPTOR_TABLE(i)         GDT_base[i].d
  #define DESCRIPTOR_TABLE_INF        12
  #define DESCRIPTOR_TABLE_SUP        256
  #define INDEX_TO_SELECTOR(x)        (((x)*8) | RUN_RING)
#else
  /* FIX ME: This is not defined at all! */
  #define DESCRIPTOR_TABLE(i)         LDT_base[i].d
  #define DESCRIPTOR_TABLE_INF        16
  #define DESCRIPTOR_TABLE_SUP        128
  #define INDEX_TO_SELECTOR(x)        (((x)*8) | 4 | RUN_RING)
#endif


/* fd32_allocate_descriptors - Implementation of DPMI 0.9 service 0000h
 *                             "Allocate LDT Descriptors"
 *
 * FIX ME: Although the service is "Allocate LDT Descriptors", this
 *         routine actually allocates descriptors into the GDT.
 *
 * This code is based on CWSDPMI 5.0, file exphdlr.c
 * by Salvo Isaja
 */
int fd32_allocate_descriptors(WORD NumSelectors)
{
  WORD i, j;
  WORD selector;

#ifdef __DEBUG__
  fd32_log_printf("[DPMI] Allocate Descriptors: 0x%x selector(s)\n", NumSelectors);
#endif

  for (i = DESCRIPTOR_TABLE_INF; (i + NumSelectors) < DESCRIPTOR_TABLE_SUP; i++)
  {
    for (j = 0; (j < NumSelectors) && (!DESCRIPTOR_TABLE(i + j).access); j++);
    if (j >= NumSelectors) break;
  }

  if ((i + NumSelectors) < DESCRIPTOR_TABLE_SUP)
  {
    for(j = 0; j < NumSelectors; j++, i++)
    {
      /*
       * Allocates a descriptor with 0 base and limit
       * Access: Present, data, r/w
       * Granularity: 32-bit, byte granularity, used
       */
#ifdef USE_GDT
      GDT_place(INDEX_TO_SELECTOR(i), 0, 0, 0x92 | (RUN_RING << 5), 0x40);
#else
      /* FIX ME: This is not defined at all! */
      LDT_place(INDEX_TO_SELECTOR(i), 0, 0, 0x92 | (RUN_RING << 5), 0x40);
#endif
    }
    /* Return the first selector allocated */
    selector = INDEX_TO_SELECTOR(i - NumSelectors);
    return selector;
  }
  return ERROR_DESCRIPTOR_UNAVAILABLE;
}


/* Convert a selector into a descriptor table index,
 * checking if the selector is valid, and returning
 * ERROR_INVALID_SELECTOR on failure.
 *
 * This code is based on CWSDPMI 5.0, file exphdlr.c
 * by Salvo Isaja
 */
static int sel_to_ldt_index(WORD Selector, WORD *Index)
{
#ifdef USE_GDT
  if (!(Selector & 0x4))
#else
  if (Selector & 0x4)
#endif
  {
    Selector /= 8;
    if ((Selector < DESCRIPTOR_TABLE_SUP  ) &&
        (DESCRIPTOR_TABLE(Selector).access))
    {
      *Index = Selector;
      return NO_ERROR;
    }
  }
  return ERROR_INVALID_SELECTOR;
}


/* fd32_free_descriptor - Implementation of DPMI 0.9 service 0001h
 *                        "Free LDT Descriptor"
 *
 * FIX ME: Although the service is "Free LDT Descriptor", this
 *         routine actually frees a descriptor into the GDT.
 *
 * FIX ME: The DPMI Specifications say also that:
 *         Under DPMI 1.0 hosts, any segment registers which contain the
 *         selector being freed are zeroed by this function.
 *
 * This code is based on CWSDPMI 5.0, file exphdlr.c
 * by Salvo Isaja
 */
int fd32_free_descriptor(WORD Selector)
{
  WORD i;
  int  ErrorCode;

#ifdef __DEBUG__
  fd32_log_printf("[DPMI] Free Descriptor: Selector 0x%x\n", Selector);
#endif

  ErrorCode = sel_to_ldt_index(Selector, &i);
  if (ErrorCode < 0) return ErrorCode;

  DESCRIPTOR_TABLE(i).access = 0;

  /* FIX ME: CWSDPMI provides the following code to clear all registers
   *         containing the freed selector.
   *         We need something different to do that.
   */

/* DPMI 1.0 free descriptor clears any sregs containing value.  Don't bother
   with CS, SS since illegal and will be caught in task switch */

/*
  word16 tval = LDT_SEL(i);
  if(tss_ptr->tss_ds == tval) tss_ptr->tss_ds = 0;
  if(tss_ptr->tss_es == tval) tss_ptr->tss_es = 0;
  if(tss_ptr->tss_fs == tval) tss_ptr->tss_fs = 0;
  if(tss_ptr->tss_gs == tval) tss_ptr->tss_gs = 0;
*/
  return NO_ERROR;
}


/* fd32_segment_to_descriptor - Implementation of DPMI 0.9 service 0002h
 *                              "Segment to Descriptor"
 *
 * NOTE: The DPMI Specifications say that:
 *       "Descriptors created by this function can never be modified or freed.
 *        For this reason, the function should be used sparingly. Clients
 *        which need to examine various real mode addresses using the same
 *        selector should allocate a descriptor with Int 31H Function 0000H
 *        and change the base address in the descriptor as necessary, using
 *        the Set Segment Base Address function (Int 31H Function 0007H"
 *       But I don't see such limitations in our current code.
 *
 * This code is based on CWSDPMI 5.0, file exphdlr.c
 * by Salvo Isaja
 */
int fd32_segment_to_descriptor(WORD RealModeSegment)
{
  WORD i, selector;
  int  ErrorCode;
  WORD base_lo  = RealModeSegment << 4;
  WORD base_med = RealModeSegment >> 12;

#ifdef __DEBUG__
  fd32_log_printf("[DPMI] Segment to Descriptor: RealModeSegment 0x%x\n", RealModeSegment);
#endif

  /* The DPMI Specifications says that:
   * Multiple calls to this function with the same segment address
   * will return the same selector. So we search for same descriptor.
   */
  for(i = DESCRIPTOR_TABLE_INF; i < DESCRIPTOR_TABLE_SUP; i++)
  {
    if ((!DESCRIPTOR_TABLE(i).gran                ) &&
        (!DESCRIPTOR_TABLE(i).base_hi             ) &&
        ( DESCRIPTOR_TABLE(i).lim_lo   == 0xFFFF  ) &&
        ( DESCRIPTOR_TABLE(i).access              ) &&
        ( DESCRIPTOR_TABLE(i).base_lo  == base_lo ) &&
        ( DESCRIPTOR_TABLE(i).base_med == base_med))
    {
      selector = INDEX_TO_SELECTOR(i);
      return selector;
    }
  }

  /* A selector for that real mode segment is not allocated.
   * Allocate one now.
   */
  ErrorCode = fd32_allocate_descriptors(1);
  if (ErrorCode < 0) return ErrorCode;
  i = ErrorCode;
  /* Return the selector just allocated */
  selector = i;

  /* Then convert that selector into a descriptor table index to
   * initialize our descriptor.
   * We can safely ignore the exit code of the following
   */
  sel_to_ldt_index(i,&i);

  DESCRIPTOR_TABLE(i).lim_lo   = 0xFFFF;
  DESCRIPTOR_TABLE(i).base_lo  = base_lo;
  DESCRIPTOR_TABLE(i).base_med = base_med;
  DESCRIPTOR_TABLE(i).gran     = 0; /* 16-bit, not 32 */

  return selector;
}


/* fd32_get_selector_increment_value - Implementation of DPMI 0.9 service 0003h
 *                                     "Get Selector Increment Value"
 *
 * NOTE: This function always succeeds
 *
 * This code is based on CWSDPMI 5.0, file exphdlr.c
 * by Salvo Isaja
 */
WORD fd32_get_selector_increment_value()
{
  /* CWSDPMI implements it simply by returning 8. Is it really so stupid!?!?
   * The DPMI Specifications say that:
   * "The increment value is always a power of two"
   */
#ifdef __DEBUG__
  fd32_log_printf("[DPMI] Get Selector Increment Value Rights\n");
#endif

  return 8;
}


/* fd32_get_segment_base_address - Implementation of DPMI 0.9 service 0006h
 *                                 "Get Segment Base Address"
 *
 * This code is based on CWSDPMI 5.0, file exphdlr.c
 * by Salvo Isaja
 */
int fd32_get_segment_base_address(WORD Selector, DWORD *BaseAddress)
{
  WORD i;
  int  ErrorCode;

#ifdef __DEBUG__
  fd32_log_printf("[DPMI] Get Segment Base Address: Selector 0x%x\n", Selector);
#endif

  ErrorCode = sel_to_ldt_index(Selector, &i);
  if (ErrorCode < 0) {
    return ErrorCode;
  }

  *BaseAddress =  DESCRIPTOR_TABLE(i).base_lo
               + (DESCRIPTOR_TABLE(i).base_med << 16)
               + (DESCRIPTOR_TABLE(i).base_hi  << 24);

  return NO_ERROR;
}


/* fd32_set_segment_base_address - Implementation of DPMI 0.9 service 0007h
 *                                 "Set Segment Base Address"
 *
 * This code is based on CWSDPMI 5.0, file exphdlr.c
 * by Salvo Isaja
 *
 * FIX ME: DPMI Specifications indicate the following error codes to return
 *         that are currently not considered (by CWSDPMI too, however):
 *
 *           8025h  invalid linear address (changing the base would cause the
 *                  descriptor to reference a linear address range outside
 *                  that allowed for DPMI clients)
 *
 * FIX ME: The DPMI Specifications say also that:
 *         A DPMI 1.0 host will reload any segment registers which contain
 *         the selector specified in register BX. It is suggested that
 *         DPMI 0.9 hosts also implement this.
 */
int fd32_set_segment_base_address(WORD Selector, DWORD BaseAddress)
{
  WORD i;
  int  ErrorCode;

#ifdef __DEBUG__
  fd32_log_printf("[DPMI] Set Segment Base Address: Selector 0x%x  Base Address 0x%lx\n",Selector,BaseAddress);
#endif

  ErrorCode = sel_to_ldt_index(Selector,&i);
  if (ErrorCode < 0) {
    return ErrorCode;
  }

  DESCRIPTOR_TABLE(i).base_lo  = (WORD)  BaseAddress;
  DESCRIPTOR_TABLE(i).base_med = (BYTE) (BaseAddress>>16);
  DESCRIPTOR_TABLE(i).base_hi  = (BYTE) (BaseAddress>>24);

  return NO_ERROR;
}


/* fd32_set_segment_limit - Implementation of DPMI 0.9 service 0008h
 *                          "Set Segment Limit"
 *
 * This code is based on CWSDPMI 5.0, file exphdlr.c
 * by Salvo Isaja
 *
 * FIX ME: DPMI Specifications indicate the following error codes to return
 *         that are currently not considered (by CWSDPMI too, however):
 *
 *           8021h  invalid value (CX <> 0 on a 16-bit DPMI host; or the
 *                  limit is greater than 1 MB, but the low twelve bits are
 *                  not set)
 *           8025h  invalid linear address (changing the limit would cause
 *                  the descriptor to reference a linear address range
 *                  outside that allowed for DPMI clients.)
 *
 * FIX ME: The DPMI Specifications say also that:
 *         A DPMI 1.0 host will reload any segment registers which contain
 *         the selector specified in register BX. It is suggested that
 *         DPMI 0.9 hosts also implement this.
 */
int fd32_set_segment_limit(WORD Selector, DWORD Limit)
{
  WORD i,j,k;
  int  ErrorCode;

#ifdef __DEBUG__
  fd32_log_printf("[DPMI] Set Segment Limit: Selector 0x%x  Limit 0x%lx\n",Selector,Limit);
#endif

  ErrorCode = sel_to_ldt_index(Selector, &i);
  if (ErrorCode < 0) {
    return ErrorCode;
  }

  j = (WORD) Limit;
  k = Limit >> 16;

  if (k > 0xF)
  {
    j = (j >> 12) | (k << 4);
    k = 0x80 | (k >> 12);
  }

  DESCRIPTOR_TABLE(i).lim_lo  = j;
  DESCRIPTOR_TABLE(i).gran   &= 0x70;
  DESCRIPTOR_TABLE(i).gran   |= k;

  return NO_ERROR;
}


/* fd32_set_descriptor_access_rights - Implementation of DPMI 0.9 service 0009h
 *                                     "Set Descriptor Access Rights"
 *
 * This code is based on CWSDPMI 5.0, file exphdlr.c
 * by Salvo Isaja
 *
 * FIX ME: DPMI Specifications indicate the following error codes to return
 *         that are currently not considered (by CWSDPMI too, however):
 *
 *           8021h  invalid value (access rights/type bytes invalid)
 *           8025h  invalid linear address (changing the access rights/type
 *                  bytes would cause the descriptor to reference a linear
 *                  address range outside that allowed for DPMI clients.)
 *
 * FIX ME: The DPMI Specifications say also that:
 *         A DPMI 1.0 host will reload any segment registers which contain
 *         the selector specified in register BX. It is suggested that
 *         DPMI 0.9 hosts also implement this.
 */
int fd32_set_descriptor_access_rights(WORD Selector, WORD Rights)
{
  WORD i;
  int  ErrorCode;

#ifdef __DEBUG__
  fd32_log_printf("[DPMI] Set Descriptor Access Rights: Selector 0x%x   Rights 0x%x\n", Selector, Rights);
#endif

  ErrorCode = sel_to_ldt_index(Selector,&i);
  if (ErrorCode < 0) {
    return ErrorCode;
  }

  DESCRIPTOR_TABLE(i).access = 0x10 | (BYTE) Rights;
  DESCRIPTOR_TABLE(i).gran   = (0x0F & DESCRIPTOR_TABLE(i).gran) | ((Rights >> 8) & 0xD0);

  return NO_ERROR;
}


/* fd32_create_alias_descriptor - Implementation of DPMI 0.9 service 000Ah
 *                                "Create Alias Descriptor"
 *
 * This code is based on CWSDPMI 5.0, file exphdlr.c
 * by Salvo Isaja
 */
int fd32_create_alias_descriptor(WORD Selector)
{
  WORD Source, Dest, new_selector;
  int  ErrorCode;

#ifdef __DEBUG__
  fd32_log_printf("[DPMI] Create Alias Descriptor: Selector 0x%x\n", Selector);
#endif

  ErrorCode = sel_to_ldt_index(Selector, &Source);
  if (ErrorCode < 0) return ErrorCode;

  ErrorCode = fd32_allocate_descriptors(1);
  if (ErrorCode < 0) return ErrorCode;
  Dest = ErrorCode;
  /* We can safely ignore the exit code of the following */
  sel_to_ldt_index(Dest,&Dest);

  new_selector = INDEX_TO_SELECTOR(Dest);
  DESCRIPTOR_TABLE(Dest) = DESCRIPTOR_TABLE(Source);
  /* Data, exp-up, wrt */
  DESCRIPTOR_TABLE(Dest).access = 2 | (DESCRIPTOR_TABLE(Dest).access & 0xF0);

  return new_selector;
}


/* fd32_get_descriptor - Implementation of DPMI 0.9 service 000Bh
 *                       "Get Descriptor"
 *
 * FIX ME: CWSDPMI performs a check on the address of the destination
 *         buffer that I've ignored for simplicity.
 *         CWSDPMI also performs a more complicated copy process that
 *         is still unknown to me.
 *
 * This code is based on CWSDPMI 5.0, file exphdlr.c
 * by Salvo Isaja
 */
int fd32_get_descriptor(WORD Selector, WORD BufferSelector, DWORD BufferOffset)
{
  WORD i;
  int  ErrorCode;

#ifdef __DEBUG__
  fd32_log_printf("[DPMI] Get Descriptor: Selector 0x%x   Buffer: 0x%x:0x%lx\n",
                  Selector, BufferSelector, BufferOffset);
#endif

  /* TO DO: CHECK_POINTER(tss_ptr->tss_es, tss_ptr->tss_edi); */

  ErrorCode = sel_to_ldt_index(Selector, &i);
  if (ErrorCode < 0) return ErrorCode;

  /* The following assembly code copies the descriptor into */
  /* the buffer pointed by the specified 48-bit pointer.    */
  asm("push   %%es               \n"
      "mov    %%ax,%%es          \n"
      "mov    (%%ebx),%%edx      \n"
      "mov    %%edx,%%es:(%%edi) \n"
      "mov    4(%%ebx),%%edx     \n"
      "mov    %%edx,%%es:4(%%edi)\n"
      "pop    %%es               \n"
      :
      : "b" (&DESCRIPTOR_TABLE(i)), "a" (BufferSelector), "D" (BufferOffset)
      : "edx");
  return NO_ERROR;
}


/* fd32_set_descriptor - Implementation of DPMI 0.9 service 000Ch
 *                       "Set Descriptor"
 *
 * FIX ME: CWSDPMI performs a check on the address of the destination
 *         buffer that I've ignored for simplicity.
 *         CWSDPMI also performs a more complicated copy process that
 *         is still unknown to me.
 *
 * FIX ME: DPMI Specifications indicate the following error codes to return
 *         that are currently not considered (by CWSDPMI too, however):
 *
 *           8021h  invalid value (access rights/type bytes invalid)
 *           8025h  invalid linear address (descriptor references a linear
 *                  address range outside that allowed for DPMI clients)
 *
 * FIX ME: The DPMI Specifications say also that:
 *         "If the descriptor's present bit is not set, then the only error
 *          checking is that the client's CPL must be equal to the
 *          descriptor's DPL field and the "must be 1" bit in the descriptor's
 *          byte 5 must be set."
 *         This check is currently not performed by FD32 nor by CWSDPMI.
 *
 * FIX ME: The DPMI Specifications say also that:
 *         A DPMI 1.0 host will reload any segment registers which contain
 *         the selector specified in register BX. It is suggested that
 *         DPMI 0.9 hosts also implement this.
 *
 * This code is based on CWSDPMI 5.0, file exphdlr.c
 * by Salvo Isaja
 */
int fd32_set_descriptor(WORD Selector, WORD BufferSelector, DWORD BufferOffset)
{
  WORD i;
  int  ErrorCode;

#ifdef __DEBUG__
  fd32_log_printf("[DPMI] Set Descriptor: Selector 0x%x   Buffer: 0x%x:0x%lx\n",
                  Selector, BufferSelector, BufferOffset);
#endif

  /* TO DO: CHECK_POINTER(tss_ptr->tss_es, tss_ptr->tss_edi); */

  ErrorCode = sel_to_ldt_index(Selector, &i);
  if (ErrorCode < 0) return ErrorCode;

  /* The following assembly code copies the buffer pointed */
  /* by the specified 48-bit pointer into the descriptor.  */
  asm("push   %%es               \n"
      "mov    %%ax,%%es          \n"
      "mov    %%es:(%%ebx),%%edx \n"
      "mov    %%edx,(%%edi)      \n"
      "mov    %%es:4(%%ebx),%%edx\n"
      "mov    %%edx,4(%%edi)     \n"
      "pop    %%es               \n"
      :
      : "D" (&DESCRIPTOR_TABLE(i)), "a" (BufferSelector), "b" (BufferOffset)
      : "edx");
  DESCRIPTOR_TABLE(i).access |= 0x10; /* Make sure non-system, non-zero */
  return NO_ERROR;
}


/* fd32_get_multiple_descriptors - Implementation of DPMI 1.0 service 000Eh
 *                                 "Get Multiple Descriptors"
 * by Salvo Isaja
 */
int fd32_get_multiple_descriptors(WORD Descriptors, WORD BufferSelector, DWORD BufferOffset)
{
  WORD i;
  WORD Selector;
  int  Count;
  int  ErrorCode;

#ifdef __DEBUG__
  fd32_log_printf("[DPMI] Get Multiple Descriptor: Descriptors: 0x%x   Buffer: 0x%x:0x%lx\n",
                  Descriptors, BufferSelector, BufferOffset);
#endif

  /* TO DO: CHECK_POINTER(tss_ptr->tss_es, tss_ptr->tss_edi); */

  asm("push %%fs     \n"
      "mov  %%ax,%%fs\n"
      :
      : "a" (BufferSelector));

  for (Count = 0; Count < Descriptors; Count++)
  {
    asm("mov %%fs:(%%edi),%%ax"
        : "=a" (Selector)
        : "D" (BufferOffset));

    ErrorCode = sel_to_ldt_index(Selector, &i);
    if (ErrorCode < 0) return ErrorCode; /* FIX ME: Should return Count */

    asm("mov (%%ebx),%%eax      \n"
        "mov %%eax,%%fs:2(%%edi)\n"
        "mov 4(%%ebx),%%eax     \n"
        "mov %%eax,%%fs:6(%%edi)\n"
        :
        : "b" (&DESCRIPTOR_TABLE(i)), "D" (BufferOffset)
        : "eax");
    BufferOffset += 10;
  }
  asm("pop %fs");
  return NO_ERROR;
}


/* fd32_set_multiple_descriptors - Implementation of DPMI 1.0 service 000Fh
 *                                 "Set Multiple Descriptors"
 * by Salvo Isaja
 *
 * FIX ME: DPMI Specifications indicate the following error codes to return
 *         that are currently not considered:
 *
 *           8021h  invalid value (access rights/type bytes invalid)
 *           8025h  invalid linear address (descriptor references a linear
 *                  address range outside that allowed for DPMI clients)
 *
 * FIX ME: The DPMI Specifications say also that:
 *         "If the descriptor's present bit is not set, then the only error
 *          checking is that the client's CPL must be equal to the
 *          descriptor's DPL field and the "must be 1" bit in the descriptor's
 *          byte 5 must be set."
 *         This check is currently not performed.
 *
 * FIX ME: The DPMI Specifications say also that:
 *         A DPMI 1.0 host will reload any segment registers which contain
 *         the selector specified in register BX. It is suggested that
 *         DPMI 0.9 hosts also implement this.
 */
int fd32_set_multiple_descriptors(WORD Descriptors, WORD BufferSelector, DWORD BufferOffset)
{
  WORD i;
  WORD Selector;
  int  Count;
  int  ErrorCode;

#ifdef __DEBUG__
  fd32_log_printf("[DPMI] Set Multiple Descriptor: Descriptors: 0x%x   Buffer: 0x%x:0x%lx\n",
                  Descriptors, BufferSelector, BufferOffset);
#endif

  /* TO DO: CHECK_POINTER(tss_ptr->tss_es, tss_ptr->tss_edi); */

  asm("push %%fs     \n"
      "mov  %%ax,%%fs\n"
      :
      : "a" (BufferSelector));

  for (Count = 0; Count < Descriptors; Count++)
  {
    asm("mov %%fs:(%%ebx),%%ax"
        : "=a" (Selector)
        : "b" (BufferOffset));

    ErrorCode = sel_to_ldt_index(Selector, &i);
    if (ErrorCode < 0) return ErrorCode; /* FIX ME: Should return Count */

    asm("mov %%fs:2(%%ebx),%%eax\n"
        "mov %%eax,(%%edi)      \n"
        "mov %%fs:6(%%ebx),%%eax\n"
        "mov %%eax,4(%%edi)     \n"
        :
        : "b" (BufferOffset), "D" (&DESCRIPTOR_TABLE(i))
        : "eax");
    BufferOffset += 10;
    DESCRIPTOR_TABLE(i).access |= 0x10; /* Make sure non-system, non-zero */
  }
  asm("pop %fs");
  return NO_ERROR;
}
