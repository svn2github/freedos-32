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

int dosidle_int(union rmregs *r)
{
  RMREGS_CLEAR_CARRY;
  return 0;
}
