#include <ll/i386/hw-data.h>
#include <ll/i386/hw-func.h>
#include <ll/i386/hw-instr.h>
#include <ll/i386/stdlib.h>
#include <ll/i386/cons.h>
#include <ll/i386/error.h>
#include <ll/i386/mem.h>
#include <ll/ctype.h>

#include "kmem.h"
#include "format.h"
#include "kernel.h"
#include "exec.h"
#include "logger.h"


/* NOTE: Move the structure here, Correct? */
struct stubinfo {
  char magic[16];
  DWORD size;
  DWORD minstack;
  DWORD memory_handle;
  DWORD initial_size;
  WORD minkeep;
  WORD ds_selector;
  WORD ds_segment;
  WORD psp_selector;
  WORD cs_selector;
  WORD env_size;
  char basename[8];
  char argv0[16];
  char dpmi_server[16];
  /* FD/32 items */
  DWORD dosbuf_handler;
};


#define STACKSIZE 1024 * 4

/* from dpmi/src/dos_exec.c, using dynamic linking */
extern WORD stubinfo_init(DWORD base, DWORD initial_size, DWORD mem_handle, char *filename, char *args);
extern void restore_psp(void);
extern void my_close(int id);
extern WORD user_cs, user_ds;

#define __WRAP_DEBUG__

static void stubinfo_set_segments(WORD stubinfo_sel, WORD cs, WORD ds)
{
  DWORD base;
  struct stubinfo *p;

  base = gdt_read(stubinfo_sel, NULL, NULL, NULL);
  p = (struct stubinfo *)base;

  p->cs_selector = cs;
  p->ds_selector = ds;
}

/* FIXME: Simplify ---> user_stack is probably not needed! */
int my_create_process(DWORD entry, DWORD base, DWORD size,
		DWORD user_stack, char *name)
{
  WORD stubinfo_sel;
  int res;
  int wrap_run(DWORD, WORD, DWORD, WORD, WORD, DWORD);
  char *args;

  args = name;
  while ((*args != 0) && (*args != ' ')) {
    args++;
  }
  if (*args != 0) {
    *args = 0;
    args++;
  } else {
    args = NULL;
  }

#ifdef __WRAP_DEBUG__
  fd32_log_printf("[WRAP] Going to run 0x%lx, size 0x%lx\n",
	  entry, size);
#endif
  /* HACKME!!! size + stack_size... */
  /* FIXME!!! WTH is user_stack (== base + size) needed??? */
  stubinfo_sel = stubinfo_init(base, size + /*STACKSIZE*/base, 0, name, args);
  if (stubinfo_sel == 0) {
    error("Error! No stubinfo!!!\n");
    return -1;
  }
  stubinfo_set_segments(stubinfo_sel, user_cs, user_ds);
#ifdef __WRAP_DEBUG__
  fd32_log_printf("[WRAP] Calling run 0x%lx 0x%lx (0x%x 0x%x) --- 0x%lx\n",
		  entry, size, user_cs, user_ds, user_stack);
#endif

  res = wrap_run(entry, stubinfo_sel, 0, user_cs, user_ds, user_stack);
#ifdef __WRAP_DEBUG__
  fd32_log_printf("[WRAP] Returned 0x%x: now restoring PSP\n", res);
#endif

  restore_psp();

  return res;
}


/* Read an executable in memory, and execute it... */
int my_exec_process(struct kern_funcs *p, int file, struct read_funcs *parser, char *cmdline)
{
  int retval;
  int size;
  DWORD exec_space;
  DWORD dyn_entry;
  DWORD base;
  int res;

  dyn_entry = fd32_load_process(p, file, parser, &exec_space, &base, &size);
  my_close(file);

  fd32_log_printf("[Wrapper] Process Loaded\n");
  if (dyn_entry == -1) {
    /* We failed... */
    return -1;
  }
  if (exec_space == 0) {
    /* No entry point... We assume that we need dynamic linking */
    /* This is not supported by the wrapper!!! */
    message("Wrapper: Trying to load a dynamic executable???\n");

    return -1;
  }
  
#ifdef __WRAP_DEBUG__
  fd32_log_printf("[Wrapper] DynEntry = 0x%lx ExecSpace = 0x%lx Base = 0x%lx\n",
		  dyn_entry, exec_space, base);
#endif
  res = mem_get_region(exec_space + size, STACKSIZE);
  if (res < 0) {
    message("Cannot allocate 0x%lx, %d\n", exec_space + size, STACKSIZE);

    return -1;
  }
  /* Note: use the real entry */
  retval = my_create_process(dyn_entry, exec_space, size + STACKSIZE,
		  base + size + STACKSIZE, cmdline);
  #undef ENTRY_CALL_OFFSET
#ifdef __WRAP_DEBUG__
  message("Returned: %d!!!\n", retval);
#endif
  mem_free(exec_space, size);
  mem_free(exec_space + size, STACKSIZE);
  /* Well... And this???
  mem_free((DWORD)sections, sizeof(struct section_info));
  if (symbols != NULL) {
    mem_free((DWORD)symbols, symsize);
  }
  */
  
  return retval;
}
