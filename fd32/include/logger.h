/* FD32 Logging Subsystem
 * by Salvo Isaja
 *
 * This file is part of FreeDOS 32, that is free software; see GPL.TXT
 */

#ifndef __FD32_LOGGER_H
#define __FD32_LOGGER_H

/* Allocates the log buffer and sets write position to the beginning */
void fd32_log_init();

/* Sends formatted output from the arguments (...) to the log buffer */
int  fd32_log_printf(char *fmt, ...) __attribute__ ((format(printf,1,2)));

/* Displays the log buffer on console screen                         */
void fd32_log_display();

/* Shows and stores the buffer starting address, size and return current position      */
DWORD fd32_log_stats(DWORD *start_addr, DWORD *size);

#endif /* #ifndef __FD32_LOGGER_H */

