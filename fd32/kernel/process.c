/* FD/32 Process Creation ruoutines
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/hw-data.h>
#include <ll/i386/hw-instr.h>
#include <ll/i386/hw-func.h>
/* #include <ll/i386/x-dosmem.h> */
#include <ll/i386/stdlib.h>
#include <ll/i386/string.h>
#include <ll/i386/mem.h>
#include <ll/i386/stdio.h>
#include <ll/i386/error.h>
#include <ll/i386/cons.h>
#include "stubinfo.h"
#include "kmem.h"
#include "kernel.h"

//#define __PROCESS_DEBUG__

WORD kern_CS, kern_DS;

/* TODO:
   Still to check:
     minstack  OK? (used to allocate initial stack...)

   There probably is some error in one of the following
   (see argtest)...
     env_size
     base_name
     argv0
*/

#define RUN_RING   0
#define ENV_SIZE 256

#define MAX_OPEN_FILES 40

extern DWORD current_SP;
struct psp *current_psp = 0;

/* Gets the Current Directiry List for the current process. */
/* Returns the pointer to the address of the first element. */
void **fd32_get_cdslist()
{
  return &current_psp->cds_list;
}

/* Gets the JFT for the current process.                          */
/* Returns the pointer to the JFT and fills the JftSize parameter */
/* with the number of entries of the JFT. JftSize may be NULL.    */
void *fd32_get_jft(int *JftSize)
{
  if (JftSize) *JftSize = (int) current_psp->jft_size;
  return current_psp->jft;
}

/* Sets the JFT for the current process. */
void fd32_set_jft(void *Jft, int JftSize)
{
  current_psp->jft      = Jft;
  current_psp->jft_size = (WORD) JftSize;
}

void set_psp(struct psp *npsp, WORD env_sel, char *args, WORD info_sel, DWORD base, DWORD end, DWORD m)
{
  /* Set the PSP */
  npsp->environment_selector = env_sel;
  npsp->info_sel = info_sel;
  npsp->link = current_psp;
  current_psp = npsp;
#ifdef __PROCESS_DEBUG__
  fd32_log_printf("[PROCESS] New PSP: 0x%x\n", current_psp);
#endif
  npsp->old_stack = current_SP;
  npsp->memlimit = base + end;

  /* Create the Job File Table */
#if 0
  /* TODO: Why!?!? Help me Luca!
           For the moment I always create a new JFT */
  if (npsp->link == NULL) /* Create new JFT */;
                     else /* Copy JFT from npsp->link->Jft */
#else
  npsp->jft_size = MAX_OPEN_FILES;
  npsp->jft      = fd32_init_jft(MAX_OPEN_FILES);
#endif
  npsp->dta      = &npsp->command_line_len;
  npsp->cds_list = 0;

  /* And now... Set the arg list!!! */
  if (args != NULL) {
    npsp->command_line_len = strlen(args);
    strcpy(npsp->command_line, args);
  } else {
    npsp->command_line_len = 0;
  }
  npsp->DOS_mem_buff = m;
}

WORD stubinfo_init(DWORD base, DWORD image_end, DWORD mem_handle, char *prgname, char *args)
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
#if 0
  char *c, *argname; int i;
  char prgname[128];
#endif
  int env_size;

#if 0
  c = filename; done = 0; i = 0;
  while ((*c != 0) && (*c != ' ')) {
    prgname[i] = *c;
    i++;
    c++;
  }
  prgname[i] = 0;
#endif

  /*Environment lenght + 2 zeros + 1 word + program name... */
  env_size = 2 + 2 + strlen(prgname) + 1;
  size = sizeof(struct stubinfo) + sizeof(struct psp) + ENV_SIZE;
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
    gdt_read(sel, NULL, &acc, NULL);
    if (acc == 0) {
      done = 1;
    } else {
      sel += 8;
    }
  }
#ifdef __PROCESS_DEBUG__
  fd32_log_printf("[PROCESS] StubInfo Selector Allocated: = 0x%x\n", sel);
#endif
  if (done == 0) {
    mem_free((DWORD)allocated, size);
    return NULL;
  }
  gdt_place(sel, (DWORD)info, sizeof(struct stubinfo),
	  0x92 | (RUN_RING << 5), 0x40);
  
  newpsp = (struct psp *)(allocated + sizeof(struct stubinfo));
  
  done = 0; psp_selector = sel;
  while (psp_selector < 256 && (!done)) {
    gdt_read(psp_selector, NULL, &acc, NULL);
    if (acc == 0) {
      done = 1;
    } else {
      psp_selector += 8;
    }
  }

  if (done == 0) {
    mem_free((DWORD)allocated, size);
    gdt_place(sel, 0, 0, 0, 0);
    return NULL;
  }
#ifdef __PROCESS_DEBUG__
  fd32_log_printf("[PROCESS] PSP Selector Allocated: = 0x%x\n", psp_selector);
#endif
  gdt_place(psp_selector, (DWORD)newpsp, sizeof(struct psp),
	  0x92 | (RUN_RING << 5), 0x40);

  /* Allocate Environment Space & Selector */
  envspace = (char *)(allocated + sizeof(struct stubinfo) +
	  sizeof(struct psp));
  memset(envspace, 0, env_size);
  *((WORD *)envspace + 2) = 1;
  strcpy(envspace + 2 + sizeof(WORD), prgname); /* No environment... */

  done = 0; env_selector = psp_selector;
  while (env_selector < 256 && (!done)) {
    gdt_read(env_selector, NULL, &acc, NULL);
    if (acc == 0) {
      done = 1;
    } else {
      env_selector += 8;
    }
  }

  if (done == 0) {
    mem_free((DWORD)allocated , size);
    gdt_place(sel, 0, 0, 0, 0);
    gdt_place(psp_selector, 0, 0, 0, 0);
    return NULL;
  }
#ifdef __PROCESS_DEBUG__
  fd32_log_printf("[PROCESS] Environment Selector Allocated: = 0x%x\n", env_selector);
#endif
  gdt_place(env_selector, (DWORD)envspace, ENV_SIZE,
	  0x92 | (RUN_RING << 5), 0x40);
  
  ksprintf(info->magic, "go32stub, v 2.00");
  info->size = sizeof(struct stubinfo);

  info->minstack = 0x40000;        /* FIXME: Check this!!! */
  info->memory_handle = mem_handle;
	/* Memory pre-allocated by the kernel... */
  info->initial_size = image_end;        /* align? */

  info->minkeep = 0x1000;        /* DOS buffer size... */
  m = dosmem_get(0x1010);
  info->ds_segment = (m >> 4) + 1;

  info->ds_selector = get_ds();
  info->cs_selector = get_cs();

  info->psp_selector = psp_selector;

  /* TODO: There should be some error down here... */
  info->env_size = env_size;

#if 0
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
#endif
#if 0
  /* TODO: Seems that this is not used... */
  ksprintf(info->basename, filename);
  strcpy(info->argv0, prgname);
#else 
  info->basename[0] = 0;
  info->argv0[0] = 0;
#endif
  ksprintf(info->dpmi_server, "Freedos/32");
#if 0
  if (argname == filename) {
    argname = NULL;
  }
#endif

  set_psp(newpsp, env_selector, args, sel, base, image_end, m);
  
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
  base = gdt_read(stubinfo_sel, NULL, NULL, NULL);
#ifdef __PROCESS_DEBUG__
  fd32_log_printf("[PROCESS] Stubinfo Sel: 0x%x --- 0x%lx\n", stubinfo_sel, base);
#endif
  info = (struct stubinfo *)base;
  base1 = gdt_read(info->psp_selector, NULL, NULL, NULL);
  if (base1 != (DWORD)current_psp) {
    error("Restore PSP: paranoia check failed...\n");
    message("Stubinfo Sel: 0x%x;    Base: 0x%lx\n",
	    stubinfo_sel, base);
    message("Current psp: 0x%lx;    Base1: 0x%lx\n",
	    (DWORD)current_psp, base1);
    fd32_abort();
  }
  gdt_place(stubinfo_sel, 0, 0, 0, 0);
  gdt_place(info->psp_selector, 0, 0, 0, 0);
  gdt_place(current_psp->environment_selector, 0, 0, 0, 0);
  ok = dosmem_free(current_psp->DOS_mem_buff, 0x1010);
  if (ok != 1) {
    error("Restore PSP panicing while freeing DOS memory...\n");
    fd32_abort();
  }

  fd32_free_jft(current_psp->jft, current_psp->jft_size);
  
  ok = mem_free(base, sizeof(struct stubinfo) + sizeof(struct psp)
	  + ENV_SIZE);
  if (ok != 1) {
    error("Restore PSP panicing while freeing memory...\n");
    fd32_abort();
  }

#ifdef WRONG
  current_psp = current_psp->link;
  if (current_psp != NULL) {
    current_SP = current_psp->old_stack;
  } else {
    current_SP = NULL;
  }
#else
  current_SP = current_psp->old_stack;
  current_psp = current_psp->link;
#endif
}


/* TODO: It's probably better to separate create_process and run_process... */

int create_process(DWORD entry, DWORD base, DWORD size, char *name, char *args)
{
  WORD stubinfo_sel;
  int res;
  int run(DWORD, WORD, DWORD);

#ifdef __PROCESS_DEBUG__
  fd32_log_printf("[PROCESS] Going to run 0x%lx, size 0x%lx\n",
	  entry, size);
#endif
  stubinfo_sel = stubinfo_init(base, size, 0, name, args);
  if (stubinfo_sel == 0) {
    error("Error! No stubinfo!!!\n");
    return -1;
  }
#ifdef __PROCESS_DEBUG__
  fd32_log_printf("[PROCESS] Calling run 0x%lx 0x%lx\n", entry, size);
#endif

  res = run(entry, stubinfo_sel, 0);
#ifdef __PROCESS_DEBUG__
  fd32_log_printf("[PROCESS] Returned 0x%lx: now restoring PSP\n", res);
#endif

  restore_psp();

  return res;
}
