#ifndef __FD32_DRENV_DATETIME_H
#define __FD32_DRENV_DATETIME_H

#include <dos.h>

/* FIX ME: According to the DOS call "get system date", fd32_get_date     */
/*         should also return the day of week. Moreover fd32_set_date     */
/*         and fd32_get_time may fail if an invalid date or time is       */
/*         specified.                                                     */
/*         I've defined them in this way for compatibility with the libc. */

typedef struct Fd32Date
{
  WORD Year;
  BYTE Day;
  BYTE Mon;
}
fd32_date_t;


typedef struct Fd32Time
{
  BYTE Min;
  BYTE Hour;
  BYTE Hund;
  BYTE Sec;
}
fd32_time_t;


#define fd32_get_date(x) getdate((struct date *) x);
#define fd32_set_date(x) setdate((struct date *) x);
#define fd32_get_time(x) gettime((struct time *) x);
#define fd32_set_time(x) settime((struct time *) x);


#endif /* #ifndef __FD32_DRENV_DATETIME_H */

