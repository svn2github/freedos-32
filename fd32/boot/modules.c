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

#include "dev/char.h"
#include "kmem.h"
#include "mods.h"
#include "format.h"
//#include "coff.h"
#include "modfs.h"
#include "doshdr.h"
#include "format.h"
#include "exec.h"
#include "logger.h"

#define MOD_ASCII   1
#define MOD_COFF    2
#define MOD_ELF     3
#define MOD_MZ      4
#define MOD_UNKNOWN 5


DWORD global_SP;

struct fd32_dev_char pseudofs;
struct kern_funcs kf;

struct mods_struct {
  void *mod_start;
  void *mod_end;
  char *string;
  DWORD reserved;
};

int istext(struct kern_funcs *kf, int file)
{
  char p[256];
  int done, res, i;

  done = 0;
  res = 256;
  while ((!done) && (res == 256)) {
    res = kf->file_read(file, p, 256);
#ifdef __ASCII_DEBUG__
    fd32_log_printf("%c %u|", *p, (unsigned int)*p);
#endif
    for (i = 0; i < res; i++) {
      if ((p[i] < 32) && (p[i] != 13) && (p[i] != 10) && (p[i] != '\t')) {
        fd32_log_printf("Not text\n");
        done = 1;
      }
    }
#ifdef __ASCII_DEBUG__
    else fd32_log_printf("\n");
#endif
  }
  return !done;
}


int identify_module(struct kern_funcs *p, int file, struct read_funcs *parser)
{
  char signature[4];

  if (isCOFF(p, file, parser)) {
    return MOD_COFF;
  }

  if (isELF(p, file, parser)) {
    return MOD_ELF;
  }    

  /* Ok, the file is not ELF nor COFF... do something else... */
  p->file_seek(file, 0, 0);
  p->file_read(file, signature, 4);
  p->file_seek(file, 0, 0);

  if (memcmp(signature,"MZ", 2) == 0) {
    return MOD_MZ;
  }

  if (istext(p, file)) {
    return MOD_ASCII;
  }

  return MOD_UNKNOWN;
}

void process_ascii_module(struct kern_funcs *p, int file)
{
  char c[256];
  int res;

  res = 256;
  while (res == 256) {
    res = p->file_read(file, c, 255);
    c[res] = 0;
    fd32_log_printf("%s", c);
  }
}

void process_dos_module(struct kern_funcs *p, int file)
{
  struct dos_header hdr;
  DWORD nt_sgn;

#ifdef __ASCII_DEBUG__
  fd32_log_printf("Seems to be a DOS file...\n");
  fd32_log_printf("Perhaps a PE? Only them are supported...\n");
#endif

  p->file_read(file, &hdr, sizeof(struct dos_header));

  p->file_seek(file, hdr.e_lfanew, 0);
  p->file_read(file, &nt_sgn, 4);
    
  if (nt_sgn == 0x00004550) {
    fd32_log_printf("It seems to be an NT PE\n");
  }
  p->file_seek(file, hdr.e_lfanew + 4, 0);
  /* ULTRAWARN!!!! THIS HAS TO BE FIXED!!!*/
  exec_process(p, file, NULL, NULL);
  return;
}

/*
#define __COFF_DEBUG__
*/

static int mod_read(int id, void *b, int len)
{
  return pseudofs.read((void *)id, len, b);
}

static int mod_seek(int id, int pos, int w)
{
  return pseudofs.seek((void *)id, pos, w);
}

void process_modules(int mods_count, DWORD mods_addr)
{
  int i, mod_type;
  struct read_funcs parser;
  char *command_line;

  /* Initialize the Modules Pseudo-FS... */
  modfs_init(&pseudofs, mods_addr, mods_count);
  /* Now module #n is accesible as file n... */
  
  kf.file_read = mod_read;
  kf.file_seek = mod_seek;
  kf.mem_alloc = mem_get;
  kf.mem_alloc_region = mem_get_region;
  kf.mem_free = mem_free;
  kf.message = message;
  kf.log = fd32_log_printf;
  kf.error = message;
  kf.seek_set = 0;
  kf.seek_cur = 1;
  for (i = 0; i < mods_count; i++) {
#ifdef __ASCII_DEBUG__
    fd32_log_printf("Processing module #%d\n", i);
#endif
    message("Processing module #%d\n", i);
    command_line = module_cl(mods_addr, i);

    mod_type = identify_module(&kf, i, &parser);
    switch(mod_type) {
    case MOD_ASCII:
      
      process_ascii_module(&kf, i);
      break;
    case MOD_ELF:
      message("going to exec ELF...\n");
      exec_process(&kf, i, &parser, command_line);
      break;
    case MOD_COFF:
      message("going to exec COFF...\n");
      exec_process(&kf, i, &parser, command_line);
      break;
    case MOD_MZ:
      process_dos_module(&kf, i);
      break;
    default:
#ifdef __ASCII_DEBUG__
      fd32_log_printf("Unknown module type\n");
#endif
    }
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
