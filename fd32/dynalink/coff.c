/* Dynalynk
 * Portable dynamic linker for object files
 * COFF parser
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */
#include <ll/i386/hw-data.h>
#include <ll/i386/mem.h>
#include <ll/i386/string.h>

#include "format.h"
#include "common.h"
#include "coff.h"
#include "symbol.h"

#define LOCAL_BSS 100


DWORD COFF_read_headers(struct kern_funcs *kf, int f, struct table_info *tables)
{
  DWORD entry;
  struct external_filehdr header;
  struct aout_hdr optheader;

  kf->file_seek(f, 0, kf->seek_set);
  kf->file_read(f, &header, sizeof(struct external_filehdr));

  if (header.f_magic != 0x014c) {
#ifdef __COFF_DEBUG__
    kf->log("Not a DJ COFF file (Wrong Magic: 0x%x)!!!\n",
	header.f_magic);
#endif
    return 0;
  }

#ifdef __COFF_DEBUG__
  kf->log("Number of sections: %d\n", header.f_nscns);
#endif
  tables->num_sections = header.f_nscns;
  if ((header.f_symptr != 0) && (header.f_nsyms != 0)) {
#ifdef __COFF_DEBUG__
    kf->log("Symbol table at offset %ld\n", header.f_symptr);
    kf->log("It contains %ld symbols\n", header.f_nsyms);
#endif

    tables->symbol = header.f_symptr;
    tables->num_symbols = header.f_nsyms;
  } else {
    tables->symbol = 0;
    tables->num_symbols = 0;
  }

#ifdef __COFF_DEBUG__
  if (header.f_opthdr != 0) {
    kf->log("Optional Header Size: %d\n", header.f_opthdr);
  } else {
    ptintf("No Optional Header\n");
  }

  if (header.f_flags & F_RELFLG) {
    kf->log("No relocation info!\n");
  }

  if (header.f_flags & F_EXEC) {
    kf->log("Executable file (no unresolved symbols)\n");
  }

  if (header.f_flags & F_LNNO) {
    kf->log("No line numbers!\n");
  }

  if (header.f_flags & F_LSYMS) {
    kf->log("No local symbols!\n");
  }

  if (header.f_flags & F_AR32WR) {
    kf->log("32-bit little endian!\n");
  } else {
    kf->log("File type?\n");
  }
#endif

  entry = 0;
#ifdef __COFF_DEBUG__
  kf->log("Finished with the file header;");
#endif
  if (header.f_opthdr !=0) {
#ifdef __COFF_DEBUG__
    kf->log(" now reading optional header\n");
#endif

    kf->file_read(f, &optheader, header.f_opthdr);
    if (optheader.magic != 0x010b) {
#ifdef __COFF_DEBUG__
      kf->log("Wrong Magic (0x%x)\n", optheader.magic);
#endif
    } else {
      entry = optheader.entry;
    }
  }
#if 0
  tables->section_header = fd->tell(f);
#else
  tables->section_header = kf->file_seek(f, 0, kf->seek_cur);
#endif
  
  return entry;
}

int COFF_read_section_headers(struct kern_funcs *kf, int f, struct table_info *tables, struct section_info *scndata)
/*
int COFF_read_section_headers(FILE *f, int num, struct section_info *scndata)
*/
{
  int bss = -1;
  int i, j;
  int pos;
  char name[9];
  struct external_scnhdr h;
  struct coff_reloc_info rel;

  kf->file_seek(f, tables->section_header, kf->seek_set);
  for (i = 0; i < tables->num_sections; i++) {
    kf->file_read(f, &h, sizeof(struct external_scnhdr));

    memcpy(name, h.s_name, 8);
    name[8] = 0;
#ifdef __COFF_DEBUG__
    kf->log("[%s]: <0x%lx:0x%lx> (0x%lx)\n", name,
	  h.s_paddr,
	  h.s_paddr + h.s_size,
	  h.s_scnptr);
#endif
    if (strcmp(name, ".bss") == 0) {
      bss = i;
    }
    scndata[i].base = h.s_paddr;
    scndata[i].size = h.s_size;
    scndata[i].fileptr = h.s_scnptr;
    scndata[i].filesize = h.s_size;
    if (h.s_nreloc !=0) {
#ifdef __COFF_DEBUG__
      kf->log("Relocation info @ %ld (%d entries)\n",
	  h.s_relptr, h.s_nreloc);
#endif
      scndata[i].num_reloc = h.s_nreloc;
      scndata[i].reloc = (void *)kf->mem_alloc(sizeof(struct reloc_info) * h.s_nreloc);
      pos = kf->file_seek(f, 0, kf->seek_cur);
      kf->file_seek(f, h.s_relptr, kf->seek_set);
      for(j = 0; j < h.s_nreloc; j++) {
        kf->file_read(f, &rel, sizeof(struct coff_reloc_info));
	scndata[i].reloc[j].offset = rel.r_vaddr;
	scndata[i].reloc[j].symbol = rel.r_symndx;
/* HACKME!!! Unify the relocation types... */
	scndata[i].reloc[j].type = rel.r_type;
	if (rel.r_type == 6){
          scndata[i].reloc[j].type = REL_TYPE_COFF_ABSOLUTE;
        } else if (rel.r_type == 20) {
          scndata[i].reloc[j].type = REL_TYPE_RELATIVE;
        }
#if 0
kf->log("Relocate 0x%lx: symbol %d type %d\n",
scndata[i].reloc[j].offset,
scndata[i].reloc[j].symbol,
scndata[i].reloc[j].type);
#endif
      }
      kf->file_seek(f, pos, kf->seek_set);
    } else {
#ifdef __COFF_DEBUG__
      kf->log("No relocation info: %u:0x%lx\n",
	h.s_nreloc, h.s_relptr);
#endif
      scndata[i].num_reloc = 0;
    }
#ifdef __COFF_DEBUG__
    if (h.s_nlnno !=0) {
      kf->log("Line Number Table Pointer @ %ld (%d entries)\n",
	  h.s_lnnoptr, h.s_nlnno);
    } else {
      kf->log("No Line Number Table\n");
    }

    if (h.s_flags & SECT_TEXT) {
      kf->log("Executable section\n");
    }
    if (h.s_flags & SECT_INIT_DATA) {
      kf->log("Data section\n");
    }
#endif
    if (h.s_flags & SECT_UNINIT_DATA) {
#ifdef __COFF_DEBUG__
      kf->log("BSS section\n");
#endif
      scndata[i].filesize = 0;
    }
  }

  return bss;
}


/*
int COFF_read_symbols(FILE *f, int n, int p, struct symbol_info *sym, char **s)
*/
int COFF_read_symbols(struct kern_funcs *kf, int f, struct table_info *tables,
	struct symbol_info *syms)
{
  int i;
  int size;
  struct coff_symbol_info *symbol;
  int numinlined;
  char *s;
  BYTE *inlined;

  numinlined = 0;

#if 0
kf->log("Reading %ld symbols\n", tables->num_symbols);
#endif
  symbol = (void *)kf->mem_alloc(tables->num_symbols * sizeof(struct coff_symbol_info));
  kf->file_seek(f, tables->symbol, kf->seek_set);
  for (i = 0; i < tables->num_symbols; i++) {
    kf->file_read(f, &symbol[i], sizeof(struct coff_symbol_info));
    if ((BYTE)symbol[i].e.e.e_zeroes != 0) {
      numinlined++;
    }
  }  
  
#ifdef __COFF_DEBUG__
  kf->log("Reading strings\n");
#endif
  kf->file_read(f, &size, 4);
#ifdef __COFF_DEBUG__
  kf->log("size: %d\n", size);
#endif
  s = (void *)kf->mem_alloc(size + numinlined * 9);
  kf->file_read(f, (char *)(s + 4), size - 4);
  inlined = s + size;

  for (i = 0; i < tables->num_symbols; i++) {
    if ((BYTE)symbol[i].e.e.e_zeroes != 0) {
      /* Inlined symbol */
      memcpy(inlined, symbol[i].e.e_name, 8);
      inlined[8] = 0;
      syms[i].name = inlined + 1;
      inlined += 9;
    } else {
#ifdef __COFF_DEBUG__
      kf->log("In strings... Offset 0x%lx\n", symbol[i].e.e.e_offset);
#endif
      syms[i].name = s + symbol[i].e.e.e_offset + 1;
    }
    syms[i].offset = symbol[i].e_value;
    syms[i].section = symbol[i].e_scnum;
#if 0
kf->log("Symbol %d: %s @ %d:0x%lx\n", i, syms[i].name, syms[i].section, syms[i].offset);
#endif
    if (syms[i].section == 0) {
      if (syms[i].offset == 0) {
        /* extern symbol */
        syms[i].section = EXTERN_SYMBOL;
      } else {
      /* common symbol */
        syms[i].section = COMMON_SYMBOL;
      }
    } else {
      syms[i].section--;
    }
  }
  kf->mem_free((DWORD)symbol, tables->num_symbols * sizeof(struct coff_symbol_info));

  return 1;
}

void COFF_free_tables(struct kern_funcs *kf, struct table_info *tables, struct symbol_info *syms, struct section_info *scndata)
{
  /* TODO: Free 'em all, please!!! */
}

int isCOFF(struct kern_funcs *kf, int f, struct read_funcs *rf)
{
  WORD magic;

  kf->file_seek(f, 0, kf->seek_set);
  kf->file_read(f, &magic, 2);

  if (magic != 0x014c) {
    return 0;
  }

  rf->read_headers = COFF_read_headers;
  rf->read_section_headers = COFF_read_section_headers;
  rf->read_symbols = COFF_read_symbols;
  rf->load_executable = common_load_executable;
  rf->load_relocatable = common_load_relocatable;
  rf->relocate_section = common_relocate_section;
  rf->import_symbol = common_import_symbol;
  rf->free_tables = COFF_free_tables;
  
  return 1;
}
