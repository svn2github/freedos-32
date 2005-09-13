/* Dynalynk
 * Portable dynamic linker for object files
 * Common ELF / COFF routines
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */
#include <ll/i386/hw-data.h>
#include <ll/i386/mem.h>
#include <ll/i386/string.h>

#include "format.h"
#include "exec.h"
#include "common.h"
#include "coff.h"

int common_relocate_section(struct kern_funcs *kf,DWORD base, DWORD bssbase, struct section_info *s, int sect, struct symbol_info *syms, struct symbol *import)
{
  int i, idx;
  DWORD address, destination;
  int j, done;
  int bss_max = 0;
  DWORD bss_max_addr = bssbase;
  struct bss_symbols bsstable[100];
  struct reloc_info *rel = s[sect].reloc;
  int n = s[sect].num_reloc;

  for (i = 0; i < n; i++) {
#ifdef __COFF_DEBUG__
    kf->log("Relocate 0x%lx (index 0x%x): mode %d ",
	  rel[i].offset, rel[i].symbol, rel[i].type);
#endif
    idx = rel[i].symbol;

#ifdef __COFF_DEBUG__
    kf->log("%s --> 0x%lx (section %d)\n", syms[idx].name,
	  syms[idx].offset, syms[idx].section);
#endif

    address = *((DWORD *) (rel[i].offset + base));
    destination = s[sect].base + rel[i].offset + base;

    if (syms[idx].section == COMMON_SYMBOL) {
#ifdef __COFF_DEBUG__
      kf->log("Non initialized data symbol... ");
      kf->log("Address: 0x%lx     ", address);
#endif

      /* Search symbol in BSS table... */
      j = 0; done = 0;
      while ((j < bss_max) && (!done)) {
        if (bsstable[j].sym_idx == idx) {
          done = 1;
        } else {
          j++;
        }
      }
      if (done) {
        address = bsstable[j].addr;
      } else {
        bsstable[bss_max].sym_idx = idx;
        bsstable[bss_max].addr = base + bss_max_addr;
        address = base + bss_max_addr;
        bss_max_addr = bss_max_addr + syms[idx].offset;
        if (bss_max_addr > bssbase + LOCAL_BSS) {
          kf->error("Out of local BSS memory\n");
          return -1;
	}
        bss_max++;
      }
    } else if (syms[idx].section == EXTERN_SYMBOL) {
#ifdef __COFF_DEBUG__
      kf->log("Searching for symbol %s\n", syms[idx].name);
#endif
      j = 0; done = 0;
      while ((import[j].name != 0) && (!done)) {
        if (strcmp(import[j].name, syms[idx].name)) {
          j++;
	} else {
          done = 1;
	}
      }
      if (done == 0) {
        kf->message("Symbol %s not found\n", syms[idx].name);
        return -1;
      }
      if ((rel[i].type == REL_TYPE_COFF_ABSOLUTE) || (rel[i].type == REL_TYPE_ELF_ABSOLUTE)) {
	/* Do something here!!! */
#if 0
	address += /* ??? */ base;
#else
	address = import[j].address;


/* Warn!!! Is this OK? */
address += *((DWORD *)destination);

#endif
      } else if (rel[i].type == REL_TYPE_RELATIVE) {
        address = import[j].address - (rel[i].offset + base) - 4;
      } else {
        kf->error("Unsupported relocation\n");
        kf->message("Relocation Type: %d\n", rel[i].type);
        return -1;
      }
      /*
      kf->message("Found: 0x%lx - 0x%lx = 0x%lx   ", import[j].address,
	      rel[i].offset + base, address);
      */
    } else {
#if 0 /*this check has to be reworked...*/
      if (syms[idx].section > n + 1) {
        drenv_kf->error("Unsupported relocation section\n");
        drenv_printf("Section %d > %d\n", syms[idx].section, n);

        drenv_printf("Value 0x%lx\n", syms[idx].offset);
        drenv_printf("SectNum %d\n", syms[idx].section);
/*
      printf("Type %u\n", syms[idx].type);
      printf("Class %u\n", s[idx].e_sclass);
      printf("NumAux %u\n", s[idx].e_numaux);
*/

        return -1;
      }
#endif
      /* Non-extermal symbols: only REL32 is supported */
      if ((rel[i].type != REL_TYPE_COFF_ABSOLUTE) &&
          (rel[i].type != REL_TYPE_ELF_ABSOLUTE) &&
          (rel[i].type != REL_TYPE_RELATIVE)) {
        kf->error("Unsupported relocation\n");
        kf->message("Relocation Type: %d\n", rel[i].type);
        return -1;
      }
#ifdef __COFF_DEBUG__
      kf->log("%p ---> 0x%lx\n", (DWORD *)(rel[i].offset + base), address);
#endif
      if (rel[i].type == REL_TYPE_COFF_ABSOLUTE) {
        if (coff_type == DjCOFF) {
          address += base;
          destination = rel[i].offset + base;
        } else if (coff_type == PECOFF) {
          address += syms[idx].offset + s[syms[idx].section].base + base;
          destination = rel[i].offset + base;
        } else {
          kf->error("COFF type not specified!! Regard as DjCOFF\n");
          /* NOTE: incomplete COFF types */
          address += base;
          destination = rel[i].offset + base;
        }
      } else if (rel[i].type == REL_TYPE_ELF_ABSOLUTE) {
#if 1
        address = *(DWORD *)destination;
        address += base + s[syms[idx].section].base + syms[idx].offset;
#else
	/* WARNING!!!! This is clearly a hack... Check what really
	 * must be done in this case...
	 */
        if (address > 0x1000000) {
          address = base + s[syms[idx].section].base + syms[idx].offset;
        } else {
          address += base + s[syms[idx].section].base + syms[idx].offset;
        }
#endif
      } else {
        address = (s[syms[idx].section].base + syms[idx].offset) - (rel[i].offset /*+ base*/) - 4;
#ifdef __COFF_DEBUG__
        kf->log("Reloc: 0x%lx + 0x%lx - 0x%lx = 0x%lx   ",
		      syms[idx].offset,
		      s[syms[idx].section].base,
		      rel[i].offset /*+ base*/, address);
#endif
      } 
    }
#ifdef __COFF_DEBUG__
    kf->log("0x%lx <--- 0x%lx\n", (DWORD *)(destination), address);
#endif
    *((DWORD *)destination) = address;
  }

  return 1;
}

/* Import symbol with suffix `name'
	NOTE: Any symbol with prefix `_' won't be found and be regarded as internal and hidden */
DWORD common_import_symbol(struct kern_funcs *kf, int n, struct symbol_info *syms, char *name, int *sect)
{
  int i;
  int len = strlen(name);

  for (i = 0; i < n ; i++) {
#ifdef __COFF_DEBUG__
    kf->log("Checking symbol %d [%d] --- Sect %d\n", i, n, syms[i].section);
#endif
    if ((syms[i].section != EXTERN_SYMBOL) && (syms[i].section != COMMON_SYMBOL)) {
#ifdef __COFF_DEBUG__
      kf->log("Compare %s, %s\n", syms[i].name, name);
#endif
      if (syms[i].name[0] != '_') {
        int sym_len = strlen(syms[i].name);
        if (sym_len >= len && strcmp(syms[i].name+sym_len-len, name) == 0) {
#ifdef __COFF_DEBUG__
          kf->log("Found: %s --- 0x%x : 0x%lx\n",
				syms[i].name, syms[i].section, syms[i].offset);
#endif
          break;
        }
#ifdef __COFF_DEBUG__
        else {
          kf->log("Cmp failed --- Going to %d\n", i);
        }
#endif
      }
    }
#ifdef __COFF_DEBUG__
    else {
      kf->log("Skipped symbol --- Going to %d\n", i);
    }
#endif
  }

  if (i < n) { /* Symbol found */
    *sect = syms[i].section;
    return syms[i].offset;
  } else {
    *sect = -1;
    kf->error("Symbol not found!!!\n");
    return 0;
  }
}


DWORD common_load_executable(struct kern_funcs *kf, int file,  struct table_info *tables, int n, struct section_info *s, int *size)
{
  int i;
  DWORD where_to_place;
  DWORD load_offset;
  int res;
  DWORD exec_space;
  DWORD needed_mem;

  needed_mem = s[n - 1].base + s[n - 1].size - tables->image_base;
  exec_space = tables->image_base;
#ifdef __COFF_DEBUG__
  kf->log("Trying to get %lx - %lx to load the COFF\n", exec_space, needed_mem);
#endif
  res = kf->mem_alloc_region(exec_space, needed_mem);
  if (res == -1) {
    kf->message("Warning: Coff starts at %lx before Free memory top\n", exec_space);
    exec_space = (DWORD)(kf->mem_alloc(needed_mem));
    kf->message("Located at %lx instead\n", exec_space);
    load_offset = exec_space - tables->image_base;
    kf->message("Load_offset: %lx\n", load_offset);
  } else {
    load_offset = 0;
  }    

  if (exec_space == 0) {	
    kf->error("Error: Not enough memory to load the Coff executable\n");
    kf->message("Needed Memory=%lu bytes - Coff starts=0x%lx\n",
	    needed_mem, s[0].base);
    return 0;
  }

  /* .text & .data section 			*/
  for (i = 0; i < n; i++) {
#ifdef __COFF_DEBUG__
    kf->log("Loading section %d at 0x%lx [offset %x]\n",
	    i, s[i].base,
	    s[i].fileptr);
#endif
    where_to_place = load_offset + (DWORD)(s[i].base);
#ifdef __COFF_DEBUG__
    kf->log("Placing at 0x%lx\n", where_to_place);
#endif
    /*	    where_to_place = (DWORD)(sh[i]->s_vaddr);	*/
    if ((s[i].filesize == 0) || (s[i].fileptr == 0)) {
      memset((void *)where_to_place, 0, s[i].size);
    } else {
      if (s[i].fileptr != 0) {
	kf->file_seek(file, kf->file_offset + s[i].fileptr, kf->seek_set);
	kf->file_read(file, (void *)where_to_place, s[i].filesize);
      }
    }
  }

  *size = needed_mem;
  return exec_space;
}

DWORD common_load_relocatable(struct kern_funcs *kf, int f,  struct table_info *tables, int n, struct section_info *s, int *size)
{
  int i;
  DWORD needed_mem = 0;
  DWORD local_offset = 0;
  BYTE *mem_space, *where_to_place;

  for (i = 0; i < n; i++) {
    needed_mem += s[i].size;
  }
  mem_space = (BYTE *)kf->mem_alloc(needed_mem + LOCAL_BSS);

#ifdef __ELF_DEBUG__
  kf->log("Loading relocatable @%p; size 0x%lx\n", mem_space, needed_mem);
#endif
  if (mem_space == NULL) {
    kf->error("Unable to allocate memory for the program image\n");
    return 0;
  }
  memset(mem_space, 0, needed_mem + LOCAL_BSS);

  for (i = 0; i < n; i++) {
#ifdef __ELF_DEBUG__
    kf->log("Section %d\t", i);
#endif
    if (s[i].size != 0) {
#ifdef __ELF_DEBUG__
      kf->log("Loading @ 0x%lx (0x%lx + 0x%lx)...\n",
      		(DWORD)mem_space + (DWORD)local_offset,
		(DWORD)mem_space, local_offset);
#endif
      where_to_place = mem_space + local_offset;
      s[i].base = local_offset;
      local_offset += s[i].size;
      kf->file_seek(f, kf->file_offset + s[i].fileptr, kf->seek_set);
      if (s[i].filesize > 0) {
        kf->file_read(f, where_to_place, s[i].filesize);
      }
    } else {
#ifdef __ELF_DEBUG__
      kf->log("Not to be loaded\n");
#endif
    }
  }

  *size = needed_mem /*+ LOCAL_BSS*/;
  return (DWORD)mem_space;
}

static int isMZ(struct kern_funcs *kf, int f, struct read_funcs *rf)
{
  DWORD nt_sgn;
  struct dos_header {
    WORD e_magic;	/* Magic number 			*/
    WORD e_cblp;	/* Bytes on last page of file		*/
    WORD e_cp;		/* Pages in file (size of the file in blocks)*/
    WORD e_res[27];
    DWORD e_lfanew;	/* File address of new exe header	*/
  } __attribute__ ((packed)) hdr;
  DWORD dj_header_start;

  kf->file_read(f, &hdr, sizeof(struct dos_header));
  if (hdr.e_magic != 0x5A4D) { /* "MZ" */
    return 0;
  }

  dj_header_start = hdr.e_cp * 512L;
  if (hdr.e_cblp) {
    dj_header_start -= (512L - hdr.e_cblp);
  }
  kf->file_seek(f, dj_header_start, kf->seek_set);
  
  if (isCOFF(kf, f, rf)) {
    kf->file_offset = dj_header_start;
    return 1;
  }

  kf->file_seek(f, hdr.e_lfanew, kf->seek_set);
  kf->file_read(f, &nt_sgn, 4);
  
  kf->message("The magic : %lx\n", nt_sgn);
  if (nt_sgn == 0x00004550) {
    if (isPEI(kf, f, rf)) {
      return 1;
    }
  }

  return 0;
}

/* Binary format management routines */
static struct bin_format binfmt[0x08] = {
  {"coff", isCOFF, fd32_exec_process},
  {"elf", isELF, fd32_exec_process},
  /* PEI is embedded in MZ {"pei", isPEI, fd32_exec_process}, */
  {"mz", isMZ, fd32_exec_process},
  {NULL}
};

struct bin_format *fd32_get_binfmt(void)
{
  return binfmt;
}

int fd32_set_binfmt(const char *name, check_func_t check, exec_func_t exec)
{
  DWORD i;

  /* Search for duplicated name */
  for (i = 0; binfmt[i].name != NULL; i++)
    if (strcmp(binfmt[i].name, name) == 0)
      break;

  if (i < 0x08) {
    if (binfmt[i].name != NULL)
      /* New binary format */
      binfmt[i].name = name;
    if (check != NULL)
      binfmt[i].check = check;
    if (exec != NULL)
      binfmt[i].exec = exec;
    return 1;
  } else {
    return 0;
  }
}
