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
#include <kernel.h>

/* Define the __BOCHS_DBG__ symbol to send log output to the Bochs window */
#define __BOCHS_DBG__


#ifndef __BOCHS_DBG__
  #define LOGBUFSIZE 65536
  static char *LogBufStart = 0;
  static char *LogBufPos   = 0;
#endif


/* Allocates the log buffer and sets write position to the beginning */
void fd32_log_init()
{
  #ifdef __BOCHS_DBG__
  message("FD32 debug logger configured for Bochs. Will not allocate log buffer.\n");
  #else
  LogBufStart = (char *) mem_get(LOGBUFSIZE);
  if (LogBufStart == 0)
  {
    message("Cannot allocate log buffer!\n");
    fd32_abort();
  }
  message("Log buffer allocated at %xh (%u bytes)\n", (unsigned) LogBufStart, LOGBUFSIZE);
  LogBufPos = LogBufStart;
  #endif
}


/* Sends formatted output from the arguments (...) to the log buffer */
int fd32_log_printf(char *fmt, ...)
{
  va_list parms;
  int     NumWritten;

  #ifdef __BOCHS_DBG__
  char    Buf[256];
  int     i;
  
  /* Format the string in Buf */
  va_start(parms, fmt);
  NumWritten =  vksprintf(Buf, fmt, parms);
  va_end(parms);
  /* And send it to the Bochs window */
  for (i = 0; i < NumWritten; i++) outp(0xE9, Buf[i]);
  #else
  /* First we check if this function is called before initing the logger */
  if (!LogBufPos) return 0;
  if (LogBufPos > LogBufStart + LOGBUFSIZE) {
      error("Out of log buff...\n");
      return 0;
  }
  /* Then we format the string appending it to the log buffer */
  va_start(parms, fmt);
  NumWritten = vksprintf(LogBufPos, fmt, parms);
  va_end(parms);
  LogBufPos += NumWritten;
  #endif
  return NumWritten;
}


/* Displays the log buffer on console screen                         */
void fd32_log_display()
{
  #ifdef __BOCHS_DBG__
  message("FD32 debug logger configured for Bochs. No log buffer allocated.\n");
  #else
  char *k;
  for (k = LogBufStart; k < LogBufPos; k++) cputc(*k);
  #endif
}


/* Shows and stores the buffer starting address, size and return current position      */
DWORD fd32_log_stats(DWORD *start_addr, DWORD *size)
{
  #ifdef __BOCHS_DBG__
  message("FD32 debug logger configured for Bochs. No log buffer allocated.\n");
  #else
  char *k;

  if (start_addr == 0 && size == 0) {
  message("Log buffer allocated at %xh (%u bytes)\n",
          (unsigned) LogBufStart, LOGBUFSIZE);
  message("Current position is %xh, %u bytes written\n",
          (unsigned) LogBufPos, (unsigned) LogBufPos - (unsigned) LogBufStart);
  } else {
    *start_addr = LogBufStart;
    *size = LogBufPos - LogBufStart;
  }
  return LogBufPos;

  #endif
}
