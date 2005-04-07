#ifndef __FD32_TIME_H
#define __FD32_TIME_H

/* FIX ME: fd32_set_date and fd32_get_time may fail if an invalid date or */
/*         time is specified.                                             */
/*         I've defined them in this way for compatibility with the libc. */

typedef struct fd32_date 
{
  WORD Year;
  BYTE Day;
  BYTE Mon;
  BYTE weekday;
}
fd32_date_t;

typedef struct fd32_time 
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


#endif /* #ifndef __FD32_TIME_H */
