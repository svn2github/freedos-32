#ifndef __FD32_DRENV_DATETIME_H
#define __FD32_DRENV_DATETIME_H

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


int fd32_get_date(fd32_date_t *);
int fd32_set_date(fd32_date_t *);
int fd32_get_time(fd32_time_t *);
int fd32_set_time(fd32_time_t *);


#endif /* #ifndef __FD32_DRENV_DATETIME_H */

