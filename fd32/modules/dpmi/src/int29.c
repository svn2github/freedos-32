/* DPMI Driver for FD32: int 0x29 services
 * by Luca Abeni
 *
 * This is free software; see GPL.txt
 */
 
#include <ll/i386/hw-data.h>
#include <ll/i386/hw-instr.h>
#include <ll/i386/hw-func.h>
#include <ll/i386/mem.h>
#include <ll/i386/string.h>
#include <ll/i386/error.h>
#include <ll/i386/cons.h>
#include <logger.h>
#include <devices.h>
#include <kernel.h>
#include "rmint.h"

int fastconsole_int(union rmregs *r)
{
  cputc(r->h.al);
  
  RMREGS_CLEAR_CARRY;
  return 0;
}
