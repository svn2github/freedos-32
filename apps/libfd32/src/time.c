#include <ll/i386/hw-data.h>
#include <fd32time.h>

#include <time.h>

time_t time(time_t *_timer)
{
  time_t res;
  struct fd32_date d;
  struct fd32_time t;

  fd32_get_date(&d);
  fd32_get_time(&t);

  d.Year -= 1970;
#warning Please implement time computation in time()
  /* FIXME:
   * how many seconds are d.Year years + d.Day days + d.Mon months
   * + t.Hour hours + t.Min minutes + t.Sec seconds ???
   * Have fun!!!
   */
  res = 0;
  if (*_timer) {
    *_timer = res;
  }

  return res;
}
