/* FD/32 Process Creation routines
 *
 * Copyright (C) 2002-2005 by Luca Abeni
 * Copyright (C) 2005-2006 by Hanzac Chen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <ll/i386/hw-data.h>
#include <ll/i386/stdlib.h>
#include <ll/i386/string.h>
#include <ll/i386/x-bios.h>
#include <ll/getopt.h>
#include "kmem.h"
#include "exec.h"
#include "kernel.h"
#include "filesys.h"
#include "list.h"

/* #define __PROCESS_DEBUG__ */


/* extern DWORD current_SP; */
/* Top/kernel process info */
static struct
{
  ListItem item;
  process_info_t pi;
} top = {
  { NULL, NULL },
  { NULL, NULL, NULL, 0, "fd32.bin", NULL, 0, NULL },
};
static List process_list = { &top.item, &top.item, 1 };
static process_info_t *cur_P = &top.pi;

/* Gets the Current Directiry List for the current process. */
/* Returns the pointer to the address of the first element. */
void **fd32_get_cdslist()
{
  return &cur_P->cds_list;
}

/* Gets the JFT for the current process.                          */
/* Returns the pointer to the JFT and fills the JftSize parameter */
/* with the number of entries of the JFT. JftSize may be NULL.    */
void *fd32_get_jft(int *JftSize)
{
  if (JftSize) *JftSize = cur_P->jft_size;
  return cur_P->jft;
}

process_info_t *fd32_get_current_pi(void)
{
  return cur_P;
}

void fd32_set_current_pi(process_info_t *ppi)
{
  cur_P = ppi;
}

void fd32_set_previous_pi(process_info_t *ppi)
{
  ListItem *p = (ListItem *)ppi;
  p--;

  /* Set to a previous process */
  p = p->prev;
  if (p != NULL)
    cur_P = (process_info_t *)++p;
}

process_info_t *fd32_new_process(char *filename, char *args, unsigned int file_size)
{
  ListItem *p = (ListItem *)mem_get(sizeof(ListItem)+sizeof(process_info_t));
  list_push_back(&process_list, p);
  p++;
  process_info_t *ppi = (process_info_t *)p;
  ppi->type = NORMAL_PROCESS;
  ppi->psp = NULL;
  ppi->cds_list = NULL; /* Pointer set by FS */
  ppi->args = args;
  ppi->filename = filename;
  ppi->memlimit = 0;

  /* Sets the JFT for the current process. */
  ppi->jft = fd32_init_jft(file_size);
  ppi->jft_size = file_size;

  /* Create the Job File Table */
  /* TODO: Why!?!? Help me Luca!
           For the moment I always create a new JFT */
  /* if (npsp->link == NULL) Create new JFT
     else Copy JFT from npsp->link->Jft
  */

  extern void restore_sp(int res) __attribute__ ((noreturn));
  ppi->_exit = restore_sp;
  return ppi;
}

/* TODO: It's probably better to separate create_process and run_process... */
int fd32_start_process(process_info_t *ppi, process_params_t *pparams)
{
  int res;
  char current_drive[2];
  char current_path[FD32_LFNPMAX];
  /* from run.S */
  extern int dll_run(DWORD address);
  extern int run(DWORD address, WORD psp_sel, DWORD parm);

  /* TODO: New imp? inherit the current path from the previous process */
  current_drive[0] = fd32_get_default_drive();
  current_drive[1] = '\0';
  fd32_getcwd(current_drive, current_path);
  cur_P = ppi;
  fd32_chdir(current_path);

#ifdef __PROCESS_DEBUG__
  fd32_log_printf("[PROCESS] Going to run 0x%lx, size 0x%lx\n", entry, size);
  message("Mem Limit: 0x%lx = 0x%lx 0x%lx\n", ppi->memlimit, base, size);
#endif
  switch (ppi->type&0x00FF) {
    case NORMAL_PROCESS:
      ppi->memlimit = pparams->normal.base + pparams->normal.size;
      res = run(pparams->normal.entry, pparams->normal.fs_sel, (DWORD)/*args*/ppi);
      break;
    case DLL_PROCESS:
      ppi->memlimit = pparams->normal.base + pparams->normal.size;
      res = dll_run(pparams->normal.entry);
      break;
    case VM86_PROCESS:
      ppi->memlimit = 0;
      res = vm86_call(pparams->vm86.ip, pparams->vm86.sp, pparams->vm86.in_regs, pparams->vm86.out_regs, pparams->vm86.seg_regs, pparams->vm86.prev_cpu_context, NULL);
      break;
    default:
      res = 1;
      break;
  }

#ifdef __PROCESS_DEBUG__
  fd32_log_printf("[PROCESS] Returned 0x%lx\n", res);
#endif

  return res;
}

void fd32_stop_process(process_info_t *ppi)
{
  ListItem *p = (ListItem *)ppi;
  p--;

  /* Set to a previous process */
  if (p->prev != NULL)
    cur_P = (process_info_t *)(p->prev+1);

  list_erase(&process_list, p);
  mem_free(p, sizeof(ListItem)+sizeof(process_info_t));
}

/* TODO: # Environ variables management
*/

/* Process arguments management*/
int fd32_get_argv(char *filename, char *args, char ***_pargv)
{
  DWORD i;
  int _argc = 1, is_newarg;

  /* Get the argc */
  if (args != NULL) {
    /* Add one at the beginning */
    for (i = 0, _argc++, is_newarg = 1; args[i] != 0; i++) {
      if (is_newarg) {
        if (args[i] == ' ')
          is_newarg = 0;
      } else {
        if (args[i] != ' ')
          _argc++, is_newarg = 1;
      }
    }
  }

  /* Allocate a argv and fill */
  *_pargv = (char **)mem_get(sizeof(char *)*_argc);
  (*_pargv)[0] = filename; /* The first argument is the filename */
  _argc = 1;
  if (args != NULL) {
    /* Add one at the beginning */
    for (i = 0, (*_pargv)[1] = args, _argc++, is_newarg = 1; args[i] != 0; i++) {
      if (is_newarg) {
        if (args[i] == ' ')
          args[i] = 0, is_newarg = 0;
      } else {
        if (args[i] != ' ')
          (*_pargv)[_argc] = args+i, _argc++, is_newarg = 1;
      }
    }
  }

  return _argc;
}

int fd32_unget_argv(int _argc, char *_argv[])
{
  DWORD i;

  /* NOTE: Reset getopt (for kernel modules) here */
  optind = 0;

  for (i = 0; i < _argc-1; i++)
    _argv[i][strlen(_argv[i])] = ' ';
  return mem_free((DWORD)_argv, sizeof(char *)*_argc);
}
