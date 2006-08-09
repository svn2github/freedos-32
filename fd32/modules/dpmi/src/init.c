/* DPMI Driver for FD32: driver initialization
 * by Luca Abeni and Hanzac Chen
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/hw-func.h>
#include <ll/i386/string.h>
#include <ll/i386/error.h>
#include <ll/getopt.h>

#include <exec.h>
#include <kernel.h>
#include "dpmiexc.h"
#include "dosexec.h"

extern void chandler(DWORD intnum, struct registers r);
extern int _mousebios_init(void);
extern void _vga_init(void);
extern void _drive_init(void);
extern int use_lfn;
#ifdef ENABLE_BIOSVGA
extern char use_biosvga;
#endif
extern char use_biosmouse;

DWORD dpmi_stack;
DWORD dpmi_stack_top;

static struct option dpmi_options[] =
{
  /* These options set a flag. */
  {"nolfn", no_argument, &use_lfn, 0},
#ifdef ENABLE_BIOSVGA
  {"biosvga", no_argument, 0, 1},
#endif
  /* These options don't set a flag.
     We distinguish them by their indices. */
  {"dos-exec", required_argument, 0, 'X'},
  {0, 0, 0, 0}
};

/*void DPMI_init(DWORD cs, char *cmdline) */
void DPMI_init(process_info_t *pi)
{
  char **argv;
  int argc = fd32_get_argv(pi->filename, pi->args, &argv);

  use_lfn = 1;
#ifdef ENABLE_BIOSVGA
  use_biosvga = 0;
#endif
  use_biosmouse = 0;

  /* Default use direct dos_exec, only support COFF-GO32 */
  dos_exec_switch(DOS_DIRECT_EXEC);

  if (argc > 1) {
    int c, option_index = 0;
    message("DPMI Init: command line\n");
    /* Parse the command line */
    for ( ; (c = getopt_long (argc, argv, "X:", dpmi_options, &option_index)) != -1; ) {
      switch (c) {
        case 0:
          use_lfn = 0;
          message("LFN disabled\n");
          break;
#ifdef ENABLE_BIOSVGA
        case 1:
          use_biosvga = 1;
          message("BIOS vga enabled\n");
          break;
#endif
        case 'X':
          if (strcmp(optarg, "vm86") == 0)
            dos_exec_switch(DOS_VM86_EXEC);
          else if (strcmp(optarg, "direct") == 0)
            dos_exec_switch(DOS_DIRECT_EXEC);
          else if (strcmp(optarg, "wrapper") == 0)
            dos_exec_switch(DOS_WRAPPER_EXEC);
          break;
        default:
          break;
      }
    }
  }
  fd32_unget_argv(argc, argv);

#ifdef ENABLE_DIRECTBIOS
  fd32_enable_real_mode_int(0x11); /* Get BIOS equipment list */
  fd32_enable_real_mode_int(0x15); /* PS BIOS interface */
  fd32_enable_real_mode_int(0x1A); /* PCI BIOS interface */
  fd32_enable_real_mode_int(0x6d); /* VIDEO BIOS entry point */
#else
  l1_int_bind(0x11, chandler);
  l1_int_bind(0x15, chandler);
  l1_int_bind(0x1A, chandler);
  fd32_enable_real_mode_int(0x6d);
#endif

  l1_int_bind(0x10, chandler);
  l1_int_bind(0x16, chandler);
  l1_int_bind(0x21, chandler);
  l1_int_bind(0x25, chandler);
  l1_int_bind(0x2A, chandler); /* Network */
  l1_int_bind(0x2F, chandler);
  l1_int_bind(0x31, chandler);
  l1_int_bind(0x33, chandler);

  _mousebios_init();
  _vga_init();
  _drive_init();
  message("DPMI installed.\n");
}
