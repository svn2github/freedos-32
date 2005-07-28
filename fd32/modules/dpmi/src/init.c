/* DPMI Driver for FD32: driver initialization
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/hw-func.h>
#include <ll/i386/string.h>
#include <ll/i386/error.h>

#include <kernel.h>
#include "dos_exec.h"

extern void chandler(DWORD intnum, struct registers r);
extern int use_lfn;

/*void DPMI_init(DWORD cs, char *cmdline) */
void DPMI_init(struct process_info *p)
{
  int done = 0;
  char *c, *cmdline;

  cmdline = args_get(p);
  use_lfn = 1;
  /* Default use direct dos_exec, only support COFF-GO32 */
  dos_exec_switch(DOS_DIRECT_EXEC);
  
  if (cmdline != NULL) {
    message("DPMI Init: command line = %s\n", cmdline);

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
          } else if (strcmp(c + 2, "dos-exec=vm86") == 0) {
            dos_exec_switch(DOS_VM86_EXEC);
          } else if (strcmp(c + 2, "dos-exec=direct") == 0) {
            dos_exec_switch(DOS_DIRECT_EXEC);
          }
        }
        c++;
      }
    }
  }
  l1_int_bind(0x21, chandler);
  l1_int_bind(0x2F, chandler);
  l1_int_bind(0x31, chandler);

  message("DPMI installed.\n");
}
