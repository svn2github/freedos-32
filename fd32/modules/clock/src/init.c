/* FD32 CMOS Driver
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

/* FIX ME: Commented parts are probably inherited from an old implementation... */

#include <ll/i386/cons.h>
#include <dr-env.h>

int time_read();
int date_read();

void cmos_init(void)
{
  message("Going to install CMOS driver...\n");
  if(fd32_add_call("fd32_get_time", time_read, SUBSTITUTE) == -1) {
    fd32_error("Cannot find fd32_get_time in system call table...\n");
  }
  if(fd32_add_call("fd32_get_date", date_read, SUBSTITUTE) == -1) {
    fd32_error("Cannot find fd32_get_date in system call table...\n");
  }

  message("Done\n");
}

