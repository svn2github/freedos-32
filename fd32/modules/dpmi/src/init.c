/* DPMI Driver for FD32: driver initialization
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/hw-func.h>
#include <ll/i386/string.h>
#include <ll/i386/error.h>

extern void int0x21(void);
extern void int0x31(void);
extern int use_lfn;

void DPMI_init(char *cmdline)
{
  int done = 0;
  char *c;

  message("DPMI Init: command line = %s\n", cmdline);

  use_lfn = 1;
  /* Parse the command line */
  c = cmdline;
  while (!done) {
    if (*c == 0) {
      done = 1;
    } else {
      if ((*c =='-') && (*(c + 1) == '-')) {
	if (strcmp(c + 2, "nolfn") == 0) {
	  use_lfn = 0;
	  message("LFN disabled\n");
	}
      }
      c++;
    }
  }
  IDT_place(0x21, int0x21);
  IDT_place(0x31, int0x31);
}
