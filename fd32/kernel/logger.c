/* FD32 Logging Subsystem
 * by Salvo Isaja
 *
 * This file is part of FreeDOS 32, that is free software; see GPL.TXT
 */

#include <ll/i386/hw-data.h>
#include <ll/i386/hw-instr.h>
#include <ll/i386/error.h>
#include <ll/stdio.h>
#include <ll/stdarg.h>
#include <kmem.h>
#include <logger.h>

#include "kernel.h"

#define LOGBUFSIZE 65536

static char *LogBufStart = 0;
static char *LogBufPos   = 0;

#define __BOCHS_DBG__

/* Allocates the log buffer and sets write position to the beginning */
void fd32_log_init()
{
  LogBufStart = (char *) mem_get(LOGBUFSIZE);
  if (LogBufStart == 0)
  {
    message("Cannot allocate log buffer!\n");
    fd32_abort();
  }
  message("Log buffer allocated at %xh (%u bytes)\n", (unsigned) LogBufStart, LOGBUFSIZE);
  LogBufPos = LogBufStart;
}


/* Sends formatted output from the arguments (...) to the log buffer */
int fd32_log_printf(char *fmt, ...)
{
  va_list parms;
  int     NumWritten;

  /* First we check if this function is called before initing the logger */
  if (!LogBufPos) return 0;
  /* Then we format the string appending it to the log buffer */
  va_start(parms,fmt);
  NumWritten = vksprintf(LogBufPos,fmt,parms);
  va_end(parms);
  LogBufPos += NumWritten;
  return NumWritten;
}


/* Displays the log buffer on console screen                         */
void fd32_log_display()
{
  char *k;
  for (k = LogBufStart; k < LogBufPos; k++) cputc(*k);
}


/* Shows the buffer starting address, size and current position      */
void fd32_log_stats()
{
  char *k;
  message("Log buffer allocated at %xh (%u bytes)\n",
          (unsigned) LogBufStart, LOGBUFSIZE);
  message("Current position is %xh, %u bytes written",
          (unsigned) LogBufPos, (unsigned) LogBufPos - (unsigned) LogBufStart);

#ifdef __BOCHS_DBG__
  for (k = LogBufStart; k < LogBufPos; k++) {
    if (*k == '\n') {
      outp(0xE9, '\t');
    }
    outp(0xE9, *k);
  }
#endif /* __BOCHS_DBG__ */
}

