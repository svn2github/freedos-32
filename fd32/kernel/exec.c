/* FD32 Executable file parser
 * It uses Dynalink to load generic executables or object files.
 * The object files are drivers or native applications
 * and need dynamic linking
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
#include "format.h"
#include "kernel.h"
#include "exec.h"
#include "logger.h"

DWORD load_process(struct kern_funcs *p, int file, struct read_funcs *parser, DWORD *e_s, DWORD *image_base, int *s)
{
  void (*fun)(void);
  DWORD offset;
  int size;
  DWORD exec_space;
  int bss_sect, i;
  DWORD dyn_entry;
  int symsize;
  struct section_info *sections;
  struct symbol_info *symbols = NULL;
  struct table_info tables;

  dyn_entry = parser->read_headers(p, file, &tables);

  sections = (struct section_info *)mem_get(sizeof(struct section_info) * tables.num_sections);
  if (sections == 0) {
    error("Error allocating sections array\n");
    
    /* Should provide some error code... */
    return -1;
  }

  bss_sect = parser->read_section_headers(p, file, &tables, sections);
  symbols = NULL;

  if (tables.flags & NEED_LOAD_RELOCATABLE) {
    exec_space = parser->load_relocatable(p, file, &tables,
	    tables.num_sections, sections, &size);
  } else {
    exec_space = parser->load_executable(p, file, &tables,
	    tables.num_sections, sections, &size);
  }
  if (exec_space == 0) {
#ifdef __EXEC_DEBUG__
    message("Error decoding the Executable data\n");
#endif
    parser->free_tables(p, &tables, symbols, sections);

    return -1;
  }

  if (tables.num_symbols != 0) {
    symsize = tables.num_symbols * sizeof (struct symbol_info);
    symbols = (struct symbol_info *)mem_get(symsize);
    if (symbols == 0) {
      error("Error allocating symbols table\n");
      parser->free_tables(p, &tables, symbols, sections);

      /* Should provide some error code... */
      return -1;
    }
    parser->read_symbols(p, file, &tables, symbols);
  }

  if ((tables.flags & NEED_SECTION_RELOCATION) || (tables.flags & NEED_IMAGE_RELOCATION)) { 
    DWORD uninitspace;
    int res;
    void *kernel_symbols;
    int reloc_sections;

    if (tables.flags & NEED_SECTION_RELOCATION) {
      if ((bss_sect < 0) || (bss_sect > tables.num_sections)) {
        error("Error: strange file --- no BSS section\n");
        /* TODO: Return code... */
        parser->free_tables(p, &tables, symbols, sections);
        mem_free(exec_space, size + LOCAL_BSS);
      
	return -1;
      }
    }
    uninitspace = size;
#ifdef __EXEC_DEBUG__
    fd32_log_printf("[EXEC] Start of local BSS: 0x%lx\n", uninitspace);
#endif
    kernel_symbols = get_syscall_table();
    if (tables.flags & NEED_SECTION_RELOCATION) {
      reloc_sections = tables.num_sections;
    } else {
      reloc_sections = 1;
    }

    for (i = 0 ; i < reloc_sections; i++) {
      res = parser->relocate_section(p, exec_space,
	      uninitspace, sections, i, symbols, kernel_symbols);
      if (res < 0) {
	error("Error: relocation failed!!!\n");
	/* TODO: Return code... */
        parser->free_tables(p, &tables, symbols, sections);
        mem_free(exec_space, size);
	return -1;
      }
    }
  }

  if (tables.flags & NO_ENTRY) {
    int init_sect;

#ifdef __EXEC_DEBUG__
    fd32_log_printf("[EXEC] Searcing for the initialization function...");
#endif
    if (symbols == 0) {
      error("Error: no symbols!!!\n");
      /* TODO: Return code... */
      parser->free_tables(p, &tables, symbols, sections);
      mem_free(exec_space, size);
      return -1;
    }
    dyn_entry = (DWORD)parser->import_symbol(p, tables.num_symbols,
	    symbols, "_init", &init_sect);
    if (init_sect != -1) {
#ifdef __EXEC_DEBUG__
      fd32_log_printf("Found: (%d =  0x%x) 0x%lx\n", init_sect,
	      init_sect, dyn_entry);
      fd32_log_printf("       Section Base: 0x%lx       Exec Space: 0x%lx\n",
	      sections[init_sect].base, exec_space);
#endif
      dyn_entry += sections[init_sect].base + exec_space;
      *s = size;
      *image_base = exec_space;
      *e_s = 0;
      parser->free_tables(p, &tables, symbols, sections);
      return dyn_entry;
    } else {
      message("WARNING: Initialization function not found!\n");
      return -1;
    }
  } else {
    offset = exec_space - sections[0].base;

    fun = (void *)(dyn_entry + offset);
#ifdef __EXEC_DEBUG__
    fd32_log_printf("[EXEC] 1) Before calling 0x%lx  = 0x%lx + 0x%lx...\n",
	    (DWORD)fun, dyn_entry, offset);
#endif
    *s = size;
    *e_s = exec_space;
    *image_base = sections[0].base;
    parser->free_tables(p, &tables, symbols, sections);
    
    return dyn_entry + offset;
  }
}

/* Read an executable in memory, and execute it... */
int exec_process(struct kern_funcs *p, int file, struct read_funcs *parser, char *fname, char *args)
{
  int retval;
  int size;
  DWORD exec_space;
  DWORD dyn_entry;
  int run(DWORD address, WORD psp_sel, DWORD parm);
  DWORD base;

  dyn_entry = load_process(p, file, parser, &exec_space, &base, &size);

  if (dyn_entry == -1) {
    /* We failed... */
    return -1;
  }
  if (exec_space == 0) {
    struct process_info pi;
    
    /* No entry point... We assume that we need dynamic linking */
    pi.args = args;
    pi.memlimit = base + size + LOCAL_BSS;
#ifdef __EXEC_DEBUG__
    fd32_log_printf("       Entry point: 0x%lx\n", dyn_entry);
    fd32_log_printf("       Going to run...\n");
    message("Mem Limit: 0x%lx = 0x%lx 0x%lx\n", pi.memlimit, base, size);
#endif
    run(dyn_entry, 0, (DWORD)/*args*/&pi);
#ifdef __EXEC_DEBUG__
    fd32_log_printf("       Returned\n");
#endif
    
    retval = 0;
  } else {
#ifdef __EXEC_DEBUG__
    fd32_log_printf("[EXEC] 2) Before calling 0x%lx...\n", dyn_entry);
#endif

    retval = create_process(dyn_entry, exec_space, size, fname, args);
#ifdef __EXEC_DEBUG__
    message("Returned: %d!!!\n", retval);
#endif
    mem_free(exec_space, size);
  }
  return retval;
}
