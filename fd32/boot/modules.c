/* FD/32 Modules Parser
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/hw-data.h>
#include <ll/i386/hw-instr.h>
#include <ll/i386/stdlib.h>
#include <ll/i386/cons.h>
#include <ll/i386/error.h>
#include <ll/i386/mem.h>
#include <ll/ctype.h>

#include "kmem.h"
#include "mods.h"
#include "modfs.h"
#include "format.h"
#include "kernel.h"
#include "logger.h"


static struct kern_funcs kf;

int istext(struct kern_funcs *kf, int file)
{
  char p[256];
  int done, res, i;

  done = 0;
  res = 256;
  while ((!done) && (res == 256)) {
    res = kf->file_read(file, p, 256);
#ifdef __MOD_DEBUG__
    fd32_log_printf("%d bytes read...", res);
    fd32_log_printf("%c %u|", *p, (unsigned int)*p);
#endif
    for (i = 0; i < res; i++) {
      if ((p[i] < 32) && (p[i] != 13) && (p[i] != 10) && (p[i] != '\t')) {
        fd32_log_printf("Not text\n");
        done = 1;
      }
    }
#ifdef __MOD_DEBUG__
    fd32_log_printf("\n");
#endif
  }
  return !done;
}

void process_ascii_module(struct kern_funcs *p, int file)
{
  char c[256];
  int res;

  res = 256;
  while (res == 256) {
    res = p->file_read(file, c, 255);
    c[res] = 0;
    message("%s", c);
  }
}

void process_modules(int mods_count, DWORD mods_addr)
{
  DWORD i, j;
  struct read_funcs rf;
  struct bin_format *binfmt;
  char *command_line;
  char *args;

  /* Initialize the Modules Pseudo-FS... */
  modfs_init(&kf, mods_addr, mods_count);
  /* Now module #n is accesible as file n... */
 
 /*
  kf.file_read = mod_read;
  kf.file_seek = mod_seek;
  */
  kf.mem_alloc = mem_get;
  kf.mem_alloc_region = mem_get_region;
  kf.mem_free = mem_free;
  kf.message = message;
  kf.log = fd32_log_printf;
  kf.error = message;
  kf.get_dll_table = get_dll_table;
  kf.add_dll_table = add_dll_table;
  kf.seek_set = 0;
  kf.seek_cur = 1;

  /* Get the binary format object table, ending with NULL name */
  binfmt = fd32_get_binfmt();
  for (i = 0; i < mods_count; i++) {
#ifdef __MOD_DEBUG__
    fd32_log_printf("[BOOT] Processing module #%d\n", i);
#endif
    message("Processing module #%d", (int)i);
    /* Pseudo-FS open */
    modfs_open(mods_addr, i);
    command_line = module_cl(mods_addr, i);

    /* Reset the file status in kf */
    kf.file_offset = 0;

    for (args = command_line; ; args++) {
      if (*args == 0) {
        args = NULL;
        break;
      } else if (*args == ' ') {
        *args++ = 0;
        break;
      }
    }

    message(": %s\n", command_line);
    if (*((DWORD *)(command_line+1)) == *((DWORD *)"hd0,")) {
      command_line[5] = 'C';
      command_line[6] = ':';
      command_line = command_line+5;
    }
    /* Load different modules in various binary format */
    for (j = 0; binfmt[j].name != NULL; j++)
    {
      if (binfmt[j].check(&kf, i, &rf)) {
        binfmt[j].exec(&kf, i, &rf, command_line, args);
        break;
      } else {
        fd32_log_printf("[MOD] Not '%s' format\n", binfmt[j].name);
      }
      /* p->file_seek(file, p->file_offset, p->seek_set); */
    }

    if (binfmt[j].name == NULL && istext(&kf, i))
      process_ascii_module(&kf, i);

    mem_release_module(mods_addr, i, mods_count);
  }
}


void module_address(DWORD mods_addr, int i, DWORD *start, DWORD *end)
{
  struct mods_struct *curr_mod;

  curr_mod = (struct mods_struct *)mods_addr;

  *start = (DWORD)curr_mod[i].mod_start;
  *end = (DWORD)curr_mod[i].mod_end;
}

char *module_cl(DWORD mods_addr, int i)
{
  struct mods_struct *curr_mod;

  curr_mod = (struct mods_struct *)mods_addr;
  return curr_mod[i].string;
}
