/* DPMI Driver for FD32: driver initialization
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/hw-func.h>
#include <ll/i386/string.h>
#include <ll/i386/error.h>
#include <ll/getopt.h>

#include <kernel.h>
#include "dos_exec.h"

extern void chandler(DWORD intnum, struct registers r);
extern WORD stubinfo_xxxx(DWORD base, DWORD initial_size, DWORD mem_handle, char *filename, char *args);
extern void restore_psp(void);
extern int use_lfn;

static struct option dpmi_options[] =
{
  /* These options set a flag. */
  {"nolfn", no_argument, &use_lfn, 0},
  /* These options don't set a flag.
     We distinguish them by their indices. */
  {"dos-exec", required_argument, 0, 'X'},
  {0, 0, 0, 0}
};

/*void DPMI_init(DWORD cs, char *cmdline) */
void DPMI_init(struct process_info *p)
{
  char **argv;
  int argc = fd32_get_argv(name_get(p), args_get(p), &argv);

  if (add_call("stubinfo_init", (DWORD)stubinfo_xxxx, ADD) == -1)
    message("Cannot add stubinfo_init to the symbol table\n");
  if (add_call("restore_psp", (DWORD)restore_psp, ADD) == -1)
    message("Cannot add restore_psp to the symbol table\n");

  use_lfn = 1;
  /* Default use direct dos_exec, only support COFF-GO32 */
  dos_exec_switch(DOS_DIRECT_EXEC);

  if (argc > 0) {
    int c, option_index = 0;
    message("DPMI Init: command line\n");
    /* Parse the command line */
    for ( ; (c = getopt_long (argc, argv, "D:", dpmi_options, &option_index)) != -1; ) {
      switch (c) {
        case 0:
          message("LFN disabled\n");
          break;
        case 'X':
          if (strcmp(optarg, "vm86") == 0)
            dos_exec_switch(DOS_VM86_EXEC);
          else if (strcmp(optarg, "direct") == 0)
            dos_exec_switch(DOS_DIRECT_EXEC);
          break;
        default:
          break;
      }
    }
  }
  l1_int_bind(0x21, chandler);
  l1_int_bind(0x2F, chandler);
  l1_int_bind(0x31, chandler);
  message("DPMI installed.\n");
}
