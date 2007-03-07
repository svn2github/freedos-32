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

#include "wrap.h"

#define STACKSIZE 1024 * 4

#define __WRAP_DEBUG__

/* FIXME: Simplify ---> user_stack is probably not needed! */
int my_create_process(DWORD entry, DWORD base, DWORD size,
		DWORD user_stack, char *filename, char *args)
{
  WORD stubinfo_sel;
  int res;
  int wrap_run(DWORD, WORD, DWORD, WORD, WORD, DWORD);

#ifdef __WRAP_DEBUG__
  fd32_log_printf("[WRAP] Going to run 0x%lx, size 0x%lx\n",
		entry, size);
#endif
  /* HACKME!!! size + stack_size... */
  /* FIXME!!! WTH is user_stack (== base + size) needed??? */
  stubinfo_sel = stubinfo_init(base, size + /*STACKSIZE*/base, 0, filename, args, user_cs, user_ds);
  if (stubinfo_sel == 0) {
    error("Error! No stubinfo!!!\n");
    return -1;
  }
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
int my_exec_process(struct kern_funcs *p, int file, struct read_funcs *parser, char *filename, char *args)
{
  int retval;
  DWORD entry;
  executable_info_t info;

  entry = fd32_load_executable(p, file, parser, &info);
  fd32_kernel_close(file);

  fd32_log_printf("[Wrapper] Process Loaded\n");
  if (entry == -1) {
    /* We failed... */
    return -1;
  }
  if (info.exec_space == 0) {
    /* No entry point... We assume that we need dynamic linking */
    /* This is not supported by the wrapper!!! */
    message("Wrapper: Trying to load a dynamic executable???\n");

    return -1;
  }
  
#ifdef __WRAP_DEBUG__
  fd32_log_printf("[Wrapper] DynEntry = 0x%lx ExecSpace = 0x%lx Base = 0x%lx\n",
		  entry, info.exec_space, info.image_base);
#endif
  retval = mem_get_region(info.exec_space + info.size, STACKSIZE);
  if (retval < 0) {
    message("Cannot allocate 0x%lx, %d\n", info.exec_space + info.size, STACKSIZE);

    return -1;
  }
  /* Note: use the real entry */
  retval = my_create_process(entry, info.exec_space, info.size + STACKSIZE,
		info.image_base + info.size + STACKSIZE, filename, args);
#ifdef __WRAP_DEBUG__
  message("Returned: %d!!!\n", retval);
#endif
  mem_free(info.exec_space, info.size);
  mem_free(info.exec_space + info.size, STACKSIZE);
  
  return retval;
}
