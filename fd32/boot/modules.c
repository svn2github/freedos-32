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
#include "format.h"
#include "kernel.h"
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


int identify_module(struct kern_funcs *p, int file, struct read_funcs *parser)
{
  char signature[4];
  
  /* Ok, the file is not ELF nor COFF... do something else... */
  p->file_read(file, signature, 4);
  p->file_seek(file, p->file_offset, p->seek_set);

  if (memcmp(signature,"MZ", 2) == 0) {
    return MOD_MZ;
  }

  if (isCOFF(p, file, parser)) {
    return MOD_COFF;
  }

  if (isELF(p, file, parser)) {
    return MOD_ELF;
  }
  
  if (1) {
  	DWORD nt_sgn;
  	struct dos_header hdr;
  	
  	p->file_seek(file, 0, 0);
  	p->file_read(file, &hdr, sizeof(struct dos_header));
    p->file_seek(file, hdr.e_lfanew, 0);
    p->file_read(file, &nt_sgn, 4);
    
    if (nt_sgn == 0x00004550) {
    if (isPECOFF(p, file, parser)) {
#ifdef __MOD_DEBUG__
      fd32_log_printf("It seems to be an NT PE\n");
#endif
      return 6; /* MOD_PECOFF */
    }
    }
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
    message("%s", c);
  }
}

void process_dos_module(struct kern_funcs *p, int file,
	struct read_funcs *parser, char *cmdline, char *args)
{
  struct dos_header hdr;
  DWORD nt_sgn;
  DWORD dj_header_start;

#ifdef __MOD_DEBUG__
  fd32_log_printf("    Seems to be a DOS file...\n");
  fd32_log_printf("    Perhaps a PE? Only them are supported...\n");
#endif

  p->file_read(file, &hdr, sizeof(struct dos_header));

  dj_header_start = hdr.e_cp * 512L;
  if (hdr.e_cblp) {
    dj_header_start -= (512L - hdr.e_cblp);
  }
  p->file_offset = dj_header_start;
  p->file_seek(file, dj_header_start, p->seek_set);
  
  if (identify_module(p, file, parser) == MOD_COFF) {
#ifdef __MOD_DEBUG__
    fd32_log_printf("    DJGPP COFF File\n");
#endif
    exec_process(p, file, parser, cmdline, args);
    return;
  }
  p->file_seek(file, hdr.e_lfanew, p->seek_set);
  p->file_read(file, &nt_sgn, 4);
  
  message("The magic : %lx\n", nt_sgn);
  if (nt_sgn == 0x00004550) {
    if (isPECOFF(p, file, parser)) {
#ifdef __MOD_DEBUG__
      fd32_log_printf("It seems to be an NT PE\n");
#endif
      exec_process(p, file, parser, cmdline, args);
    }
  }

  return;
}

void process_modules(int mods_count, DWORD mods_addr)
{
  int i, mod_type;
  struct read_funcs parser;
  char *command_line;
  void mem_release_module(DWORD mstart, int m, int mnum);	/* FIXME! */
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
  for (i = 0; i < mods_count; i++) {
#ifdef __MOD_DEBUG__
    fd32_log_printf("[BOOT] Processing module #%d\n", i);
#endif
    message("Processing module #%d ", i);
    command_line = module_cl(mods_addr, i);

    mod_type = identify_module(&kf, i, &parser);

    args = command_line;
    while ((*args != 0) && (*args != ' ')) {
      args++;
    }
    if (*args != 0) {
      *args = 0;
      args++;
    } else {
      args = NULL;
    }

    switch(mod_type) {
    case MOD_ASCII:
      
      process_ascii_module(&kf, i);
      break;
    case MOD_ELF:
#ifdef __MOD_DEBUG__
      fd32_log_printf("    ELF Module\n");
#endif
      message("going to exec ELF...\n");
      exec_process(&kf, i, &parser, command_line, args);
      break;
    case MOD_COFF:
#ifdef __MOD_DEBUG__
      fd32_log_printf("    COFF Module\n");
#endif
      message("going to exec COFF...\n");
      exec_process(&kf, i, &parser, command_line, args);
      break;
    case MOD_MZ:
      process_dos_module(&kf, i, &parser, command_line, args);
      break;
    default:
      message("Unknown module type\n");
    }
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
