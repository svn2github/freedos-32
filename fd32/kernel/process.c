/* FD/32 Process Creation ruoutines
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/hw-data.h>
#include <ll/i386/hw-instr.h>
#include <ll/i386/hw-func.h>
#include <ll/i386/x-dosmem.h>
#include <ll/i386/stdlib.h>
#include <ll/i386/string.h>
#include <ll/i386/mem.h>
#include <ll/i386/stdio.h>
#include <ll/i386/error.h>
#include <ll/i386/cons.h>

/*
#define __PROCESS_DEBUG__
*/

#include "stubinfo.h"
#include "jft.h"
#include "kmem.h"
#include "kernel.h"
#include "devices.h"
#include "dev/char.h"

WORD kern_CS, kern_DS;

/* Still to check:
     minstack  OK? (used to allocate initial stack...)
     memory_handle!!!        <--- Search how to compute it...

   The correctness of the following still has to be checked...
     env_size
     base_name
     argv0
     dpmi_server
*/

#define RUN_RING   0
#define ENV_SIZE 256

#define MAX_OPEN_FILES 20

/*
  extern DWORD current_SP;
  */
extern DWORD current_SP;
struct psp *current_psp = 0;

void set_psp(struct psp *npsp, WORD env_sel, char * args, WORD info_sel, DWORD base, DWORD end, DWORD m)
{
  struct jft *NewJft;
  fd32_dev_char_t *console_ops;
  int console_type;

  /* Set the PSP */
  npsp->environment_selector = env_sel;
  npsp->info_sel = info_sel;
  npsp->link = current_psp;
  current_psp = npsp;
#ifdef __PROCESS_DEBUG__
  fd32_log_printf("New PSP: 0x%x\n", current_psp);
#endif
  npsp->old_stack = current_SP;
  npsp->memlimit = base + end;

  /* Allocate a JFT, and associate it to the current PSP */
//  newjft = (struct jft *)mem_get(sizeof(struct jft) * MAX_OPEN_FILES);
  NewJft = (struct jft *) mem_get(sizeof(struct jft) * MAX_OPEN_FILES);
  if (npsp->link == NULL) {
//    memset(newjft, 0, sizeof(struct jft) * MAX_OPEN_FILES);
    memset(NewJft, 0, sizeof(struct jft) * MAX_OPEN_FILES);
    console_ops = fd32_dev_query("con", FD32_DEV_CHAR);
    if (console_ops == 0) {
      message("No console driver... Task will not be able to do input!!!\n");
    } else {
      console_type = (console_ops->ioctl(0, FD32_SET_NONBLOCKING_IO, 0) > 0);
      NewJft[0].Ops = (fd32_dev_ops_t *) console_ops;
      if (!console_type) {
	extern fd32_dev_char_t fake_console;
	int fake_console_input(void *p, DWORD len, BYTE *buffer);
	
	if (fake_console.read == NULL) {
	  memcpy(&fake_console, console_ops, sizeof(fd32_dev_char_t));
	  fake_console.P = console_ops;
	  fake_console.read = fake_console_input;
	  NewJft[0].Ops = (fd32_dev_ops_t *) &fake_console;
	}
      }
      //      newjft[0].driver_type = console_type;
      message("Console Type:%d\n", console_type);//newjft[0].driver_type);
      if ((console_type & DRIVER_TYPE_WRITE) == 0) {
	message("Limited console capabilities: using cprintf for task output!!!");
      } else {
	NewJft[1].Ops = (fd32_dev_ops_t *) console_ops;
	NewJft[2].Ops = (fd32_dev_ops_t *) console_ops;
	//newjft[1].driver_type = console_type;
	//newjft[2].driver_type = console_type;
      }
    }
  }else {
    memcpy(NewJft, npsp->link->Jft, sizeof(struct jft));
  }
  npsp->JftSize = MAX_OPEN_FILES;
  npsp->Jft     = NewJft;

  /* And now... Set the arg list!!! */
  npsp->command_line_len = strlen(args);
  strcpy(npsp->command_line, args);
  npsp->DOS_mem_buff = m;
}

WORD stubinfo_init(DWORD base, DWORD image_end, DWORD mem_handle, char *filename)
{
  struct stubinfo *info;
  struct psp *newpsp;
  char *envspace;
  WORD sel, psp_selector, env_selector;
  BYTE acc;
  int done;
  DWORD m;
  BYTE *allocated;
  int size;
  char *c, *argname; int i;
  char prgname[128];
  int env_size;

  c = filename; done = 0; i = 0;
  while ((*c != 0) && (*c != ' ')) {
    if (*c != ' ') {
      prgname[i] = *c;
    } else {
      prgname[i] = 0;
    }
    i++;
    c++;
  }

  /*Environment lenght + 2 zeros + 1 word + program name... */
  env_size = 2 + 2 + strlen(prgname);
  size = sizeof(struct stubinfo) + sizeof(struct psp) + env_size;
  /* Always allocate a fixed amount of memory...
   * Keep some space for adding environment variables
   */
  allocated = (BYTE *)mem_get(sizeof(struct stubinfo) + sizeof(struct psp) + ENV_SIZE);
  if (allocated == NULL) {
    return NULL;
  }
  info = (struct stubinfo *)allocated;

  done = 0; sel = 8;
  while (sel < 256 && (!done)) {
    GDT_read(sel, NULL, &acc, NULL);
    if (acc == 0) {
      done = 1;
    } else {
      sel += 8;
    }
  }
#ifdef __PROCESS_DEBUG__
  fd32_log_printf("StubInfo Selector Allocated: = 0x%x\n", sel);
#endif
  if (done == 0) {
    mem_free((DWORD)allocated, size);
    return NULL;
  }
  GDT_place(sel, (DWORD)info, sizeof(struct stubinfo), 0x92 | (RUN_RING << 5), 0x40);
  
  newpsp = (struct psp *)(allocated + sizeof(struct stubinfo));
  
  done = 0; psp_selector = sel;
  while (psp_selector < 256 && (!done)) {
    GDT_read(psp_selector, NULL, &acc, NULL);
    if (acc == 0) {
      done = 1;
    } else {
      psp_selector += 8;
    }
  }

  if (done == 0) {
    mem_free((DWORD)allocated, size);
    GDT_place(sel, 0, 0, 0, 0);
    return NULL;
  }
#ifdef __PROCESS_DEBUG__
  fd32_log_printf("PSP Selector Allocated: = 0x%x\n", psp_selector);
#endif
  GDT_place(psp_selector, (DWORD)newpsp, sizeof(struct psp), 0x92 | (RUN_RING << 5), 0x40);

  /* Allocate Environment Space & Selector */
  envspace = (char *)(allocated + sizeof(struct stubinfo) + sizeof(struct psp));
  memset(envspace, 0, env_size);
  *((WORD *)envspace + 2) = 1;
  strcpy(envspace + 2 + sizeof(WORD), prgname); /* No environment... */

  done = 0; env_selector = psp_selector;
  while (env_selector < 256 && (!done)) {
    GDT_read(env_selector, NULL, &acc, NULL);
    if (acc == 0) {
      done = 1;
    } else {
      env_selector += 8;
    }
  }

  if (done == 0) {
    mem_free((DWORD)allocated , size);
    GDT_place(sel, 0, 0, 0, 0);
    GDT_place(psp_selector, 0, 0, 0, 0);
    return NULL;
  }
#ifdef __PROCESS_DEBUG__
  fd32_log_printf("Environment Selector Allocated: = 0x%x\n", env_selector);
#endif
  GDT_place(env_selector, (DWORD)envspace, ENV_SIZE, 0x92 | (RUN_RING << 5), 0x40);
  
  ksprintf(info->magic, "go32stub, v 2.00");
  info->size = sizeof(struct stubinfo);

  info->minstack = 0x40000;        /* ??? */
  info->memory_handle = mem_handle;
	/* Memory pre-allocated by the kernel... */
  info->initial_size = image_end;        /* align? */

	/* If minkeep == 0, ds_segment is not used!!! */
#if 1
  info->minkeep = 0x1000;        /* DOS buffer size... If 0, allocate new one */
  m = (DWORD)DOS_alloc(0x1010);
  info->ds_segment = (m >> 4) + 1;

#else
  info->minkeep = 0;        /* DOS buffer size... If 0, allocate new one */
  info->ds_segment = 0;         /* RM DS... does not exist!!! */
#endif
  info->ds_selector = get_DS();
  info->cs_selector = get_CS();

  info->psp_selector = psp_selector;

  info->env_size = env_size;

  argname = filename; c = filename; done = 0; i = 0;
  while (*c != 0) {
    if ((*c == ' ') && (argname == filename)) {
      argname = c;
      prgname[i] = 0;
      done = 1;
    }
    if ((*c == ':') || (*c == '/') || (*c == '\\')) {
      i = 0;
    } else {
      if (done != 1) {
	prgname[i] = *c;
	i++;
      }
    }
    c++;
  }
  prgname[15] = 0;

#if 0
  /* Seems that this is not used... */
  ksprintf(info->basename, filename);
  info->basename[0] = 0;
  strcpy(info->argv0, prgname);
#else 
  info->basename[0] = 0;
  info->argv0[0] = 0;
#endif
  ksprintf(info->dpmi_server, "Freedos/32");

  set_psp(newpsp, env_selector, argname, sel, base, image_end, m);
  
  return sel;
}

void restore_psp(void)
{
  WORD stubinfo_sel;
  DWORD base, base1;
  int ok;
  struct stubinfo *info;

  /* Free memory & selectors... */
  stubinfo_sel = current_psp->info_sel;
  base = GDT_read(stubinfo_sel, NULL, NULL, NULL);
#ifdef __PROCESS_DEBUG__
  fd32_log_printf("Stubinfo Sel: 0x%x --- 0x%lx\n", stubinfo_sel, base);
#endif
  info = (struct stubinfo *)base;
  base1 = GDT_read(info->psp_selector, NULL, NULL, NULL);
  if (base1 != (DWORD)current_psp) {
    error("Restore PSP: paranoia check failed...\n");
    message("Stubinfo Sel: 0x%x;    Base: 0x%lx\n", stubinfo_sel, base);
    message("Current psp: 0x%lx;    Base1: 0x%lx\n", (DWORD)current_psp, base1);
    fd32_abort();
  }
  GDT_place(stubinfo_sel, 0, 0, 0, 0);
  GDT_place(info->psp_selector, 0, 0, 0, 0);
  GDT_place(current_psp->environment_selector, 0, 0, 0, 0);
  ok = DOS_free((void *)current_psp->DOS_mem_buff, 0x1010);
  if (ok != 1) {
    error("Restore PSP panicing while freeing DOS memory...\n");
    fd32_abort();
  }

  ok = mem_free(base, sizeof(struct stubinfo) + sizeof(struct psp) + ENV_SIZE);
  if (ok != 1) {
    error("Restore PSP panicing while freeing memory...\n");
    fd32_abort();
  }

  current_psp = current_psp->link;
  if (current_psp != NULL) {
    current_SP = current_psp->old_stack;
  } else {
    current_SP = NULL;
  }
}


/* It's probably better to separate create_process and run_process... */

int create_process(DWORD entry, DWORD base, DWORD size, char *name)
{
  WORD stubinfo_sel;
  int res;
  int run(DWORD, WORD);

#ifdef __PROCESS_DEBUG__
  fd32_log_printf("Going to run 0x%lx, size 0x%lx\n", entry, size);
#endif
  stubinfo_sel = stubinfo_init(base, size, 0, name);
  if (stubinfo_sel == 0) {
    error("Error! No stubinfo!!!\n");
    return -1;
  }
#ifdef __PROCESS_DEBUG__
  fd32_log_printf("Calling run 0x%lx 0x%lx\n", entry, size);
#endif

  res = run(entry, stubinfo_sel);
#ifdef __PROCESS_DEBUG__
  fd32_log_printf("Returned 0x%lx: now restoring PSP\n", res);
#endif

  restore_psp();

  return res;
}
