/* DPMI Driver for FD32: handler for INT 0x31, Service 0x02
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include<ll/i386/hw-data.h>
#include<ll/i386/hw-instr.h>
#include<ll/i386/hw-func.h>
#include<ll/i386/error.h>
#include<ll/i386/cons.h>

#include "logger.h"

#include "dpmi.h"
#include "dpmiexc.h"
#include "int31_02.h"

/*
#define __DEBUG__
*/

void int31_0200(union regs *r)
{
  BYTE intnum;

  intnum = (BYTE)r->d.ebx;
#ifdef __DEBUG__
  fd32_log_printf("[DPMI] Get Real Mode Interrupt Vector: interrupt number %d\n",
                  intnum);
#endif

  fd32_get_real_mode_int(intnum, &(r->x.cx), &(r->x.dx));
  CLEAR_CARRY;

  return;
}

void int31_0201(union regs *r)
{
  BYTE intnum;

  intnum = (BYTE)r->d.ebx;
#ifdef __DEBUG__
  fd32_log_printf("[DPMI] Set Real Mode Interrupt Vector: interrupt number %d\n",
                  intnum);
#endif
  fd32_set_real_mode_int(intnum, r->x.cx, r->x.dx);
  CLEAR_CARRY;
  
  return;
}


void int31_0202(union regs *r)
{
  int res;
  
#ifdef __DEBUG__
  fd32_log_printf("[DPMI] Get Exception Handler: exception number %d\n", r->h.bl);
#endif

  res = fd32_get_exception_handler(r->h.bl, &(r->x.cx), &(r->d.edx));

  dpmi_return(res, r);
}

void int31_0203(union regs *r)
{
  int res;

#ifdef __DEBUG__
  fd32_log_printf("[DPMI] Set Exception Handler: exception number %d\n", r->h.bl);
  fd32_log_printf("            Handler: 0x%lx:0x%lx\n", r->d.ecx, r->d.edx);
#endif

  res = fd32_set_exception_handler(r->h.bl, r->x.cx, r->d.edx);
  
  dpmi_return(res, r);
}

void int31_0204(union regs *r)
{
  int res;

#ifdef __DEBUG__
  fd32_log_printf("[DPMI] Get Protect Mode Interrupt Vector: interrupt number %d\n",
                  r->h.bl);
#endif

  res = fd32_get_protect_mode_int(r->h.bl, &(r->x.cx), &(r->d.edx));

  dpmi_return(res, r);
}

void int31_0205(union regs *r)
{
  int res;

#ifdef __DEBUG__
  fd32_log_printf("[DPMI] Set Protect Mode Interrupt Vector: interrupt number %d\n",
                  r->h.bl);
  fd32_log_printf("            Handler: 0x%lx:0x%lx\n", r->d.ecx, r->d.edx);
#endif

  res = fd32_set_protect_mode_int(r->h.bl, r->x.cx, r->d.edx);
  
  dpmi_return(res, r);
}
