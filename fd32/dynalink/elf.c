/* Dynalynk
 * Portable dynamic linker for object files
 * ELF parser
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */
#include <ll/i386/hw-data.h>
#include <ll/i386/mem.h>
#include <ll/i386/string.h>

#include "format.h"
#include "common.h"
#include "elf.h"

#define LOCAL_BSS 100

/*
char elf_signature[] = "\0x7FELF";
*/
char elf_signature[] = "\177ELF";

DWORD ELF_read_headers(struct kern_funcs *kf, int f, struct table_info *tables)
{
  int res;

  DWORD entry;
  struct elf_header header;
  struct elf_section_header h;

  kf->file_seek(f, 0, kf->seek_set);
  kf->file_read(f, &header, sizeof(struct elf_header));
  
  if (memcmp(header.e_ident, elf_signature, 4)) {
    kf->error("Not an ELF file\n");
    kf->message("Wrong signature: 0x%lx)!!!\n",
	*(DWORD *)header.e_ident);
    return 0;
  }

  if (header.e_ident[4] != ELFCLASS32) {
    kf->error("Wrong ELF class\n");
    kf->message("Class: 0x%x!!!\n", header.e_ident[4]);
    return 0;
  }

  if (header.e_ident[5] != ELFDATA2LSB) {
    kf->error("Wrong data ordering (not LSB)\n");
    kf->message("Ordering: 0x%x!!!\n", header.e_ident[5]);
    return 0;
  }

#ifdef __ELF_DEBUG__
  if (header.e_machine != EM_386) {
    kf->log("Warning: machine = 0x%x!!!\n", header.e_machine);
  }

  kf->log("ELF Type: 0x%x\n", header.e_type);
#endif

  if (header.e_shoff != 0) {
#ifdef __ELF_DEBUG__
    kf->log("Section headers @ %ld\n", header.e_shoff);
    kf->log("Number of sections: %d\n", header.e_shnum);
    kf->log("Section header size: %d (0x%x)\n",
    		header.e_shentsize, header.e_shentsize);
#endif
    tables->section_header = header.e_shoff;
    tables->section_header_size = header.e_shentsize;
    tables->num_sections = header.e_shnum;
  }

#ifdef __ELF_DEBUG__
  if (header.e_phoff != 0) {
    kf->log("Program header table @ %ld\n", header.e_phoff);
    kf->log("Number of segments: %d\n", header.e_phnum);
    kf->log("Segment header size: %d (0x%x)\n",
    		header.e_phentsize, header.e_phentsize);
  }

#if 0
  /* Flags... */
  /* RELOCATION */
  if (header.f_flags & F_RELFLG) {
    drenv_kf->log("No relocation info!\n");
  }
  
  if (header.f_flags & F_EXEC) {
    drenv_kf->log("Executable file (no unresolved symbols)\n");
  }

  if (header.f_flags & F_LNNO) {
    drenv_kf->log("No line numbers!\n");
  }

  if (header.f_flags & F_LSYMS) {
    drenv_kf->log("No local symbols!\n");
  }

  if (header.f_flags & F_AR32WR) {
    drenv_kf->log("32-bit little endian!\n");
  } else {
    drenv_kf->log("File type?\n");
  }
#endif

  kf->log("Section Name String Table is section number %d\n",
  		header.e_shstrndx);
#endif
  if (header.e_shstrndx > tables->num_sections) {
    kf->error("Error: SNST number > section number...\n");

    return 0;
  }
  res = kf->file_seek(f, tables->section_header + header.e_shstrndx *
  		tables->section_header_size, kf->seek_set);
  if (res < 0) {
    kf->error("Cannot seek");
    return 0;
  }

  res = kf->file_read(f, &h, sizeof(struct elf_section_header));
  if (res < 0) {
    kf->error("Cannot read");
    return 0;
  }

  tables->section_names = 0;
  if (h.sh_size != 0) {
#ifdef __ELF_DEBUG__
    kf->log("Loading Section Names...\n");
#endif
    tables->section_names = (void *)kf->mem_alloc(h.sh_size);
    if (tables->section_names == NULL) {
      kf->error("Failed to allocate space for section names...\n");
      return 0;
    }

    res = kf->file_seek(f, h.sh_offset, kf->seek_set);
    if (res < 0) {
      kf->error("Cannot seek");
      return 0;
    }
    res = kf->file_read(f, tables->section_names, h.sh_size);
    if (res < 0) {
      kf->error("Cannot read");
      return 0;
    }
  } else {
    kf->message("0 size?\n");
  }

  entry = header.e_entry;

  return entry;
}

int ELF_read_section_headers(struct kern_funcs *kf, int f, struct table_info *tables, struct section_info *scndata)
{
  int bss = -1;
  int i, j;
  struct elf_section_header h;
  int header_size;
  int stringtable = -1;
  struct elf_rel_info r;

  header_size = tables->section_header_size;
  if (header_size > sizeof(struct elf_section_header)) {
    kf->message("Section header size (%d) > sizeof(struct section_header) (%d)\n",
    		header_size, sizeof(struct elf_section_header));
    header_size = sizeof(struct elf_section_header);
  }
  for (i = 0; i < tables->num_sections; i++) {
    kf->file_seek(f, tables->section_header + i * tables->section_header_size,
    			kf->seek_set);
    kf->file_read(f, &h, sizeof(struct elf_section_header));

#ifdef __ELF_DEBUG__
    kf->log("Section %d: ", i);
#endif
    /* If this is a NULL section, skip it!!! */
    if (h.sh_type != SHT_NULL) {
      if (tables->section_names != 0) {
#ifdef __ELF_DEBUG__
        kf->log("[%s]", tables->section_names + h.sh_name);
#endif
        if (strcmp(tables->section_names + h.sh_name, ".bss") == 0) {
          bss = i;
        }
      }
#ifdef __ELF_DEBUG__
      kf->log("    <0x%lx:0x%lx> (0x%lx)\n",
	  h.sh_addr,
	  h.sh_addr + h.sh_size,
	  h.sh_offset);
#endif

      /*
         Set this stuff to 0...
         If size == 0 the section must not be loaded
       */
      scndata[i].num_reloc = 0;
      scndata[i].base = 0;
      scndata[i].size = 0;
      scndata[i].fileptr = 0;
      scndata[i].filesize = 0;
      
      if (h.sh_type == SHT_REL) {
#ifdef __ELF_DEBUG__
        kf->log("\t\tSection %d: relocation info!!!\n", i);
        kf->log("\t\tSymbol table: section number %lu\n", h.sh_link);
        kf->log("\t\tSection to modify: %lu\n", h.sh_info);
        kf->log("\t\tNumber of relocation entries: %lu\n",
      			h.sh_size / h.sh_entsize);
#endif
        if (scndata[h.sh_info].num_reloc != 0) {
          kf->error("Double relocation for section\n");
	  kf->message("%lu?\n", h.sh_info);
	  return 0;
        }
	/* So, ...let's load it!!! */
        scndata[h.sh_info].num_reloc = h.sh_size / h.sh_entsize;
        scndata[h.sh_info].reloc = (void *)kf->mem_alloc((h.sh_size / h.sh_entsize) * sizeof(struct reloc_info));
        if (scndata[h.sh_info].reloc == NULL) {
          kf->error("Failed to allocate space for relocation info...\n");
          return 0;
        }
	for (j = 0; j < h.sh_size / h.sh_entsize; j++) {
          kf->file_seek(f, h.sh_offset + j * h.sh_entsize, kf->seek_set);
          kf->file_read(f, &r, sizeof(struct elf_rel_info));
	  scndata[h.sh_info].reloc[j].offset = r.r_offset;
	  scndata[h.sh_info].reloc[j].symbol = r.r_info >> 8;
/* HACKME!!! Unify the relocation types... */
	  scndata[h.sh_info].reloc[j].type = (BYTE)r.r_info;
	  if ((BYTE)r.r_info == R_386_32){
            scndata[h.sh_info].reloc[j].type = REL_TYPE_ELF_ABSOLUTE;
          } else if ((BYTE)r.r_info == R_386_PC32) {
            scndata[h.sh_info].reloc[j].type = REL_TYPE_RELATIVE;
          }
	}
      } else if (h.sh_type == SHT_RELA) {
        kf->error("Error: unsupported relocation section!!!\n");

	return 0;
      } else if ((h.sh_type == SHT_SYMTAB) || (h.sh_type == SHT_DYNSYM)) {
#ifdef __ELF_DEBUG__
        kf->log("\t\tSection %d: symbol table!!!\n", i);
        kf->log("\t\tString table: section number %lu\n", h.sh_link);
        kf->log("\t\tLast local Symbol + 1: %lu\n", h.sh_info);
#endif
	tables->symbol = h.sh_offset;
	tables->num_symbols = h.sh_size / h.sh_entsize;
	tables->symbol_size = h.sh_size;
	if (stringtable != -1) {
	  kf->error("Error: double string table!!!\n");
	  return 0;
	}
	stringtable = h.sh_link;
	if (stringtable < i) {
	  kf->error("Strange... ");
	  kf->message("String table (%d) < Symbol Table\n", stringtable);
	  return 0;
	}
      } else if (i == stringtable) {
#ifdef __ELF_DEBUG__
        kf->log("\t\t Section %d: string table!!!\n", i);
#endif
	tables->string = h.sh_offset;
	tables->string_size = h.sh_size;
	stringtable = -1;
      } else {
        scndata[i].base = h.sh_addr;
        scndata[i].size = h.sh_size;
        scndata[i].fileptr = h.sh_offset;
        if (h.sh_type != SHT_NOBITS) {
#ifdef __ELF_DEBUG__
	  kf->log("BSS?\n");
#endif
          scndata[i].filesize = h.sh_size;
        }
      }
    } else {
#ifdef __ELF_DEBUG__
      kf->log("NULL Section\n");
#endif
    }

#if 0
    if (h.s_flags & SECT_TEXT) {
      drenv_kf->log("Executable section\n");
    }
    if (h.s_flags & SECT_INIT_DATA) {
      drenv_kf->log("Data section\n");
    }
    if (h.s_flags & SECT_UNINIT_DATA) {
      drenv_kf->log("BSS section\n");
      scndata[i].filesize = 0;
    }
#endif
  }

  return bss;
}


int ELF_read_symbols(struct kern_funcs *kf, int f, struct table_info *tables,
	struct symbol_info *syms)
{
  int i;
  int entsize;
  struct elf_symbol_info symbol;
  char *s;

  s = (void *)kf->mem_alloc(tables->string_size);
  if (s == NULL) {
    kf->error("Failed to allocate space for string table...\n");
    return 0;
  }
  kf->file_seek(f, tables->string, kf->seek_set);
  kf->file_read(f, s, tables->string_size);
   
  entsize = tables->symbol_size / tables->num_symbols;

  for (i = 0; i < tables->num_symbols; i++) {
    kf->file_seek(f, tables->symbol + i * entsize, kf->seek_set);
    kf->file_read(f, &symbol, sizeof(struct elf_symbol_info));
    syms[i].name = s + symbol.st_name;
    syms[i].section = symbol.st_shndx;
    syms[i].offset = symbol.st_value;
    if (syms[i].section == SHN_UNDEF) {
      /* extern symbol */
      syms[i].section = EXTERN_SYMBOL;
    }
    if (syms[i].section == SHN_COMMON) {
      /* extern symbol */
      syms[i].section = COMMON_SYMBOL;
      syms[i].offset = symbol.st_size;
    }
  }
  
  return 1;
}

void ELF_free_tables(struct kern_funcs *kf, struct table_info *tables, struct symbol_info *syms, struct section_info *scndata)
{
  /* TODO: Free 'em all, please!!! */
}

int isELF(struct kern_funcs *kf, int f, struct read_funcs *rf)
{
  char signature[4];

  kf->file_seek(f, 0, kf->seek_set);
  kf->file_read(f, signature, 4);

  if (memcmp(signature, elf_signature, 4)) {
    return 0;
  }

  rf->read_headers = ELF_read_headers;
  rf->read_section_headers = ELF_read_section_headers;
  rf->read_symbols = ELF_read_symbols;
  /* Fix This!!! */
  rf->load_executable = common_load_executable;
  rf->load_relocatable = common_load_relocatable;
  rf->relocate_section = common_relocate_section;
  rf->import_symbol = common_import_symbol;
  rf->free_tables = ELF_free_tables;
  
  return 1;
}
