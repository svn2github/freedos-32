#include <dr-env.h>

int epoch = 1900;
/* From Lin... */
/**********************************************************************
 * register summary
 **********************************************************************/
#define RTC_SECONDS             0
#define RTC_MINUTES             2
#define RTC_HOURS               4

#define RTC_DAY_OF_WEEK         6
#define RTC_DAY_OF_MONTH        7
#define RTC_MONTH               8
#define RTC_YEAR                9

#define RTC_CONTROL     11

#define RTC_DM_BINARY   0x04

#define BCD2BIN(n) ((n >> 4) * 10 + (n & 0x0F))

inline BYTE cmos_read(BYTE addr)
{
  fd32_outb(0x70, addr);
  return fd32_inb(0x71);
}

int time_read(struct fd32_time *t)
{
  BYTE ctrl;

  if (t == 0) {
    return -1;
  }
  t->Hund = 0;                  /* For the moment... */
  t->Sec = cmos_read(RTC_SECONDS);
  t->Min = cmos_read(RTC_MINUTES);
  t->Hour = cmos_read(RTC_HOURS);
  ctrl = cmos_read(RTC_CONTROL);
  
#if 1
  if (!(ctrl & RTC_DM_BINARY)) {
#else 
  {
#endif
    /* Convert from bcd to binary... */
    t->Sec = BCD2BIN(t->Sec);
    t->Min = BCD2BIN(t->Min);
    t->Hour = BCD2BIN(t->Hour);
  }

  return 1;
}

int date_read(struct fd32_date *d)
{
  BYTE ctrl;

  if (d == 0) {
    return -1;
  }
  d->weekday = cmos_read(RTC_DAY_OF_WEEK);
  d->Day = cmos_read(RTC_DAY_OF_MONTH);
  d->Mon = cmos_read(RTC_MONTH);
  d->Year = cmos_read(RTC_YEAR);
  ctrl = cmos_read(RTC_CONTROL);

#if 1
  if (!(ctrl & RTC_DM_BINARY)) {
#else 
  {
#endif
    /* Convert from bcd to binary... */
    d->Day = BCD2BIN(d->Day);
    d->Mon = BCD2BIN(d->Mon);
    d->Year = BCD2BIN(d->Year);
  }
  d->Year += epoch - 1900;
  if (d->Year <= 69) {
    d->Year += 100;
  }
  d->Year += 1900;

  /*
    mon--;         Do we need this?
  */
  
  return 1;
}
