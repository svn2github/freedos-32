#include <ll/i386/hw-data.h>
#include <fd32time.h>

#include <sys/time.h>

int gettimeofday(struct timeval *__p, struct timezone *__z)
{
  struct fd32_date d;
  struct fd32_time t;

  if (__z != NULL) {
    /* We have no timezones now... */
    __z->tz_minuteswest = 0;
    __z->tz_dsttime = 0;
  }
  if (__p != NULL) {
    fd32_get_date(&d);
    fd32_get_time(&t);

    d.Year -= 1970;
#warning Please implement time computation in time()
    __p->tv_sec = 0;
    /* FIXME:
     * how many seconds are d.Year years + d.Day days + d.Mon months ???
     * please fill __p->tv_sec accordingly!
     * Have fun!!!
     */
    __p->tv_sec += t.Hour * 60 * 60 + t.Sec;
    __p->tv_usec = t.Hund * 10000;	/* We can do better, I know... */
  }

  return 0;
}
