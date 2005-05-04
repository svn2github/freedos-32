#include <ll/i386/hw-data.h>
#include <fd32time.h>

#include <sys/time.h>
#include <sys/times.h>

int _gettimeofday(struct timeval *__p, struct timezone *__z)
{
  struct fd32_date d;
  struct fd32_time t;
  unsigned long tmp1, tmp2;
  int elapsed_days[] = {0,31,59,90,120,151,181,212,243,273,304,334};

  if (__z != NULL) {
    /* We have no timezones now... */
    __z->tz_minuteswest = 0;
    __z->tz_dsttime = 0;
  }
  if (__p != NULL) {
    fd32_get_date(&d);
    fd32_get_time(&t);

    /* full years */
    tmp1 = d.Year;
    tmp1 -= 1970;
    /* full days */
    tmp2 = tmp1;
    tmp1 *= 365;
    tmp2++;
    tmp2 /= 4;
    tmp1 += tmp2;
    tmp1 += elapsed_days[d.Mon - 1];
    if((d.Year % 4 == 0) && (d.Mon > 2))
        tmp1++;
    tmp1 += d.Day - 1;
    /* full hours */
    tmp1 *= 24;
    tmp1 += t.Hour;
    /* full minutes */
    tmp1 *= 60;
    tmp1 += t.Min;
    /* full seconds */
    tmp1 *= 60;
    tmp1 += t.Sec;
    __p->tv_sec = tmp1;
    __p->tv_usec = t.Hund * 10000;	/* We can do better, I know... */
  }

  return 0;
}

/* times Timing information for current process. */

clock_t _times(struct tms *buf)
{
  return -1;
}
