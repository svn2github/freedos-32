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

/* #define __EXEC_DEBUG__ */

DWORD fd32_load_executable(struct kern_funcs *kf, int file, struct read_funcs *rf, executable_info_t *execinfo)
{
  DWORD size;
  DWORD exec_space;
  int bss_sect, i;
  DWORD entry;
  struct section_info *sections;
  struct symbol_info *symbols = NULL;
  struct table_info tables;

  /* Initialize the table info */
  tables.section_names_size = 0;
  tables.global_data_size = 0;
  tables.private_info = 0;

  entry = rf->read_headers(kf, file, &tables);

  sections = (struct section_info *)mem_get(sizeof(struct section_info) * tables.num_sections);
  if (sections == 0) {
    error("Can't allocate the sections info!\n");
    /* Should provide some error code... */
    return -1;
  }

  bss_sect = rf->read_section_headers(kf, file, &tables, sections);

  /* Load symbols (if it's objects, calculate the local bss size) */
  if (tables.num_symbols != 0) {
    symbols = (struct symbol_info *)mem_get(tables.num_symbols * sizeof (struct symbol_info));
    if (symbols == 0) {
      error("Error allocating symbols table\n");
      rf->free_tables(kf, &tables, symbols, sections);
      /* Should provide some error code... */
      return -1;
    }
    rf->read_symbols(kf, file, &tables, symbols);
  }

  if (tables.flags & NEED_LOAD_RELOCATABLE) {
    exec_space = rf->load_relocatable(kf, file, &tables,
		tables.num_sections, sections, &size);
  } else {
    exec_space = rf->load_executable(kf, file, &tables,
		tables.num_sections, sections, &size);
  }

  if (exec_space == 0) {
#ifdef __EXEC_DEBUG__
    message("Error decoding the Executable data\n");
#endif
    rf->free_tables(kf, &tables, symbols, sections);

    return -1;
  }

  if (tables.flags & (NEED_SECTION_RELOCATION|NEED_IMAGE_RELOCATION)) {
    int res;
    int reloc_sections;

    if (tables.flags & NEED_SECTION_RELOCATION) {
      if ((bss_sect < 0) || (bss_sect > tables.num_sections)) {
        error("Error: strange file --- no BSS section\n");
        /* TODO: Return code... */
        rf->free_tables(kf, &tables, symbols, sections);
        mem_free(exec_space, size);

        return -1;
      }
    }
#ifdef __EXEC_DEBUG__
    fd32_log_printf("[EXEC] Start of local BSS: 0x%lx\n", tables.private_info);
#endif
    if (tables.flags & NEED_SECTION_RELOCATION) {
      reloc_sections = tables.num_sections;
    } else {
      reloc_sections = 1;
    }

    for (i = 0 ; i < reloc_sections; i++) {
      res = rf->relocate_section(kf, exec_space, &tables, tables.num_sections, sections, i, symbols, NULL);
      if (res < 0) {
        error("Error: relocation failed!!!\n");
        /* TODO: Return code... */
        rf->free_tables(kf, &tables, symbols, sections);
        mem_free(exec_space, size);
        return -1;
      }
    }
  }

  /* The size of the execution space */
  execinfo->size = size;

  if (tables.flags & NO_ENTRY) {
    int init_sect;
    /* No entry point... We assume that we need dynamic linking */	 
#ifdef __EXEC_DEBUG__
    fd32_log_printf("[EXEC] Searcing for the initialization function...");
#endif
    if (symbols == 0) {
      error("Error: no symbols!!!\n");
      /* TODO: Return code... */
      rf->free_tables(kf, &tables, symbols, sections);
      mem_free(exec_space, size);
      return -1;
    }
    entry = (DWORD)rf->import_symbol(kf, tables.num_symbols, symbols, "_init", &init_sect);
    if (init_sect != -1) {
#ifdef __EXEC_DEBUG__
      fd32_log_printf("Found: (%d =  0x%x) 0x%lx\n", init_sect,
	      init_sect, entry);
      fd32_log_printf("       Section Base: 0x%lx       Exec Space: 0x%lx\n",
	      sections[init_sect].base, exec_space);
#endif
      entry += sections[init_sect].base + exec_space;
      execinfo->image_base = exec_space;
      execinfo->exec_space = 0;
      rf->free_tables(kf, &tables, symbols, sections);
      return entry;
    } else {
      message("WARNING: Initialization function not found!\n");
      return -1;
    }
  } else if (tables.flags & DLL_WITH_STDCALL) {
    entry += exec_space;
    execinfo->image_base = exec_space;
    execinfo->exec_space = -1; /* Note: Just to notify the exec_process it"s DLL with STDCALL entry */
    rf->free_tables(kf, &tables, symbols, sections);

    return entry;
  } else {
    /* Reloc the entry if it's a relocated (PE) executable image */
    if (tables.flags & NEED_IMAGE_RELOCATION)
      entry += ((struct pe_reloc_info *)sections[0].reloc)->offset;
#ifdef __EXEC_DEBUG__
    fd32_log_printf("[EXEC] 1) Before calling 0x%lx ...\n", entry);
#endif
    execinfo->exec_space = exec_space;
    execinfo->image_base = tables.image_base;
    rf->free_tables(kf, &tables, symbols, sections);

    return entry;
  }
}

/* Read an executable in memory, and execute it... */
int fd32_exec_process(struct kern_funcs *kf, int file, struct read_funcs *rf, char *filename, char *args)
{
  process_info_t *ppi;
  process_params_t params;
  executable_info_t info;
  DWORD exec_space;
  DWORD offset;
  int retval;

  params.normal.entry = fd32_load_executable(kf, file, rf, &info);
  params.normal.base = info.image_base;
  params.normal.size = info.size;
  exec_space = info.exec_space;

  if (params.normal.entry == -1)
    return -1;
  params.normal.fs_sel = 0;

  ppi = fd32_new_process(filename, args, 0);
  if (exec_space == 0) {
    ppi->type = NORMAL_PROCESS|RESIDENT;
    exec_space = params.normal.base; /* TODO: Redesign a better way */
  } else if (exec_space == -1) {
    ppi->type = DLL_PROCESS|RESIDENT;
    exec_space = params.normal.base; /* TODO: ... */
  } else {
#ifdef __EXEC_DEBUG__
    fd32_log_printf("[EXEC] 2) Before calling 0x%lx...\n", params.normal.entry);
#endif
    ppi->type = NORMAL_PROCESS;
    offset = exec_space - params.normal.base;
    params.normal.entry += offset;
    params.normal.base = exec_space;
  }

  retval = fd32_start_process(ppi, &params);
  if (!(ppi->type&RESIDENT))
    mem_free(exec_space, params.normal.size);
  /* Back to the previous process NOTE: TSR native programs? */
  fd32_stop_process(ppi);

#ifdef __EXEC_DEBUG__
  message("Returned: %d!!!\n", retval);
#endif
  return retval;
}
