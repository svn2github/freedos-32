/* DPMI Driver for FD32: driver initialization
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/hw-func.h>
#include <ll/i386/string.h>
#include <ll/i386/error.h>
#include <ll/getopt.h>

#include <exec.h>
#include <kernel.h>
#include "dpmi.h"
#include "dos_exec.h"

extern void chandler(DWORD intnum, struct registers r);
extern WORD _stubinfo_init(DWORD base, DWORD initial_size, DWORD mem_handle, char *filename, char *args, WORD cs_sel, WORD ds_sel);
extern void restore_psp(void);
extern int use_lfn;

DWORD dpmi_stack;
DWORD dpmi_stack_top;

static struct option dpmi_options[] =
{
  /* These options set a flag. */
  {"nolfn", no_argument, &use_lfn, 0},
  /* These options don't set a flag.
     We distinguish them by their indices. */
  {"dos-exec", required_argument, 0, 'X'},
  {0, 0, 0, 0}
};

#ifdef ENABLE_BIOSVGA
#include <ll/i386/x-bios.h>
static void reflect(DWORD intnum, struct registers r)
{
    struct tss *vm86_tss;
    DWORD *bos;
    DWORD isr_cs, isr_eip;
    WORD *old_esp;
    DWORD *IRQTable_entry;
    CONTEXT c = get_tr();


	fd32_log_printf("[DPMI] Emulate interrupt %lx ...\n", intnum);

    vm86_tss = vm86_get_tss(X_VM86_CALLBIOS_TSS);
    bos = (DWORD *)vm86_tss->esp0;
    if (c == X_VM86_CALLBIOS_TSS) {
		old_esp = (WORD *)(*(bos - 6) + (*(bos - 5) << 4));
		r.flags = CPU_FLAG_VM | CPU_FLAG_IOPL;
		*(old_esp - 2) = (WORD)(*(bos - 8));
		*(old_esp - 3) = (WORD)(*(bos - 9));
		*(bos - 6) -= 6;
		/* We are emulating INT 0x6d */
		IRQTable_entry = (void *)(0L);
		isr_cs= ((IRQTable_entry[0x6d]) & 0xFFFF0000) >> 16;
		isr_eip = ((IRQTable_entry[0x6d]) & 0x0000FFFF);
	
		*(bos - 8) = isr_cs;
		*(bos - 9) = isr_eip;
    }
}
#endif

/*void DPMI_init(DWORD cs, char *cmdline) */
void DPMI_init(process_info_t *pi)
{
  char **argv;
  int argc = fd32_get_argv(pi->filename, pi->args, &argv);

  if (add_call("stubinfo_init", (DWORD)_stubinfo_init, ADD) == -1)
    message("Cannot add stubinfo_init to the symbol table\n");
  if (add_call("restore_psp", (DWORD)restore_psp, ADD) == -1)
    message("Cannot add restore_psp to the symbol table\n");

  use_lfn = 1;
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
#ifdef ENABLE_BIOSVGA
  l1_int_bind(0x6d, reflect);
#endif
  l1_int_bind(0x10, chandler);
  l1_int_bind(0x16, chandler);
  l1_int_bind(0x1A, chandler);
  l1_int_bind(0x21, chandler);
  l1_int_bind(0x2F, chandler);
  l1_int_bind(0x31, chandler);
  l1_int_bind(0x33, chandler);
  message("DPMI installed.\n");
}
