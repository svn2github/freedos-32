/* Dynalynk
 * Portable dynamic linker for object files
 * PEI (Portable Executable Image format) parser part
 * by Hanzac Chen
 * 
 * This is free software; see GPL.txt
 */
 
#include <ll/i386/hw-data.h>
#include <ll/i386/mem.h>
#include <ll/i386/string.h>
char *strstr(const char *haystack, const char *needle); /* FIXME: Place this in oslib/ll/i386/string.h */

#include "kmem.h"
#include "format.h"
#include "common.h"
#include "coff.h"
#include "pei.h"

/* #define __PEI_DEBUG__ */

struct pei_extra_info {
  DWORD export_symbol;
  DWORD export_symbol_size;
  DWORD import_symbol;
  DWORD import_symbol_size;
  DWORD base_reloc;
  DWORD base_reloc_size;
  DWORD base_reloc_offset;
  DWORD section_0_base;
};


DWORD pei_read_headers(struct kern_funcs *kf, int f, struct table_info *tables)
{
  DWORD entry;
  struct external_filehdr header;
  struct pe_aouthdr optheader;
  struct pei_extra_info *pee_info = (struct pei_extra_info *)kf->mem_alloc(sizeof(struct pei_extra_info));
  kf->file_seek(f, kf->file_offset, 0);
  kf->file_read(f, &header, sizeof(struct external_filehdr));
  
  /* Note: check for the target machine */
  if (header.f_magic != IMAGE_FILE_MACHINE_I386)
  {
    kf->message("[PECOFF] Target machine not supported or wrong magic: 0x%x!!!\n", header.f_magic);
    return 0;
  }
  #ifdef __PEI_DEBUG__
  kf->log("[PECOFF] Number of sections: %d\n", header.f_nscns);
  kf->log("[PECOFF] Flags: %x\n", header.f_flags);
  #endif
  tables->num_sections = header.f_nscns;
  tables->flags = 0; /* Note: Do not set NEED_IMAGE_RELOCATION, only when the base_reloc_size != 0 */
  tables->image_base = 0;
  
  entry = 0;
  if (header.f_opthdr !=0)
  {
    kf->file_read(f, &optheader, header.f_opthdr);
    if (optheader.magic != PE32MAGIC) {
      kf->message("[PECOFF] ERROR: Wrong magic (0x%x)\n", optheader.magic);
    } else {
      /* Get the absolute entry */
      entry = optheader.entry+optheader.image_base;
      if (header.f_flags&IMAGE_FILE_DLL) {
        if (optheader.subsystem != IMAGE_SUBSYSTEM_NATIVE) {
          tables->flags |= DLL_WITH_STDCALL;
          entry -= optheader.image_base;
        } else {
          tables->flags |= NO_ENTRY;
        }
      }
      /* Note: Otherwise the DLL could be a FD32 driver */
      tables->image_base = optheader.image_base;
      pee_info->export_symbol = optheader.data_dir[IMAGE_DIRECTORY_ENTRY_EXPORT].vaddr;
      pee_info->export_symbol_size = optheader.data_dir[IMAGE_DIRECTORY_ENTRY_EXPORT].size;
      pee_info->base_reloc = optheader.data_dir[IMAGE_DIRECTORY_ENTRY_BASERELOC].vaddr;
      pee_info->base_reloc_size = optheader.data_dir[IMAGE_DIRECTORY_ENTRY_BASERELOC].size;
      pee_info->import_symbol = optheader.data_dir[IMAGE_DIRECTORY_ENTRY_IMPORT].vaddr;
      pee_info->import_symbol_size = optheader.data_dir[IMAGE_DIRECTORY_ENTRY_IMPORT].size;
      tables->private_info = (DWORD)pee_info;
    }
  }
  /* After this seek no longer need offset, we use direct */
  tables->section_header = kf->file_seek(f, 0, kf->seek_cur);
  /* Section header size is not used (we can figer out from the section num).*/
  
  return entry;
}

int pei_read_section_headers(struct kern_funcs *kf, int f, struct table_info *tables, struct section_info *s)
{
  int i, n;
  char name[9];
  int e_section = -1;
  struct pe_scnhdr h;
  struct pei_extra_info *pee_info;
  pee_info = (struct pei_extra_info *)tables->private_info;

  kf->file_seek(f, tables->section_header, 0);
  n = tables->num_sections;
  for (i = 0; i < n; i++)
  {
    kf->file_read(f, &h, sizeof(struct pe_scnhdr));

    memcpy(name, h.s_name, 8);
    name[8] = 0;
    #ifdef __PEI_DEBUG__
    kf->log("[%s]: <0x%lx:0x%lx> (0x%lx)\n", name, h.s_vaddr, h.s_vaddr+h.s_vsize, h.s_paddr);
    #endif
    s[i].base = h.s_vaddr;
    s[i].size = h.s_vsize;
    s[i].fileptr = h.s_paddr;
    s[i].filesize = h.s_psize;
    s[i].reloc = NULL;
    s[i].num_reloc = 0;
    
    /* Calculate the section of the image directories */
    if(h.s_vaddr <= pee_info->export_symbol && h.s_vaddr+h.s_vsize > pee_info->export_symbol)
      e_section = i;    
  }
  /* Save the section[0] base */
  pee_info->section_0_base = s[0].base;

  /* The number of symbols should be export symbols */
  tables->num_symbols = 0;
  /* Process the export symbols : calculate the symbols' number */
  if(pee_info->export_symbol_size != 0 && e_section != -1)
  {
    struct exp_dir edir;
    kf->file_seek(f, s[e_section].fileptr+pee_info->export_symbol-s[e_section].base, 0);
    kf->file_read(f, (void *)&edir, sizeof(struct exp_dir));
    #ifdef __pei_DEBUG__
    kf->log("Export symbol number: %d\n", edir.func_num);
    #endif
    tables->num_symbols = edir.func_num;
  }
  
  /* PE BSS section no need to relocation */
  return -1;
}

DWORD pei_load(struct kern_funcs *kf, int f, struct table_info *tables, int n, struct section_info *s, int *size)
{
  int res;
  int i, j;
  DWORD reloc_offset = 0;
  DWORD image_memory_size;
  DWORD image_memory_start;
  DWORD section_memory_start;
  struct pei_extra_info *pee_info;
  DWORD image_base = tables->image_base;
  pee_info = (struct pei_extra_info *)tables->private_info;

  /* Load this sections to process the symbols and reloc info */
  image_memory_size = s[n - 1].base + s[n - 1].filesize;
  #ifdef __PEI_DEBUG__
  kf->log("Allocate %lx to load the PE image ...\n", image_memory_size);
  #endif
  image_memory_start = image_base;
  
  /* Relocate if having BASERELOC */
  if(pee_info->base_reloc_size != 0)
  {
    image_memory_start = (DWORD)(kf->mem_alloc(image_memory_size));
    reloc_offset = image_memory_start-image_base;
    image_base = image_memory_start;
    #ifdef __pei_DEBUG__
    kf->log("Relocated to: %lx\n", image_memory_start);
    kf->log("Relocated offset: %lx\n", reloc_offset);
    #endif
  } else {
    res = kf->mem_alloc_region(image_memory_start, image_memory_size);
    if(res == -1)
    {
      kf->message("WARNING: The PE image memory base isn't available and image not relocatable!\n");
      return -1;
    }
  }

  if (image_memory_start == 0) {
    kf->message("WARNING: Not enough memory to load the PE image\n");
    kf->message("Needed memory = %lu bytes - image memory starts = 0x%lx\n", image_memory_size, s[0].base);
    return -1;
  }
  
  /* Save the image base */
  tables->image_base = image_base;
  
  if(reloc_offset != 0)
  {
    struct pe_reloc_info *r;
    /* Set the relocation method */
    tables->flags |= NEED_IMAGE_RELOCATION;
    /* PE only have one RELOC info, we set it on the first section */
    s[0].num_reloc = 1;
    r = (struct pe_reloc_info *)kf->mem_alloc(sizeof(struct pe_reloc_info));
    r->offset = reloc_offset;
    r->addr = pee_info->base_reloc+image_base;
    r->size = pee_info->base_reloc_size;
    s[0].reloc = (struct reloc_info *)r;
  }

  memset((void *)image_base, 0, image_memory_size);
  /* .text & .data section & other sections */
  for (i = 0; i < n; i++) {
    section_memory_start = image_base+s[i].base;
    #ifdef __PEI_DEBUG__
    kf->log("Loaded section %d at 0x%lx [file offset %x]\n", i, section_memory_start, s[i].fileptr);
    #endif
    if (s[i].fileptr != 0) {
      kf->file_seek(f, s[i].fileptr, 0);
      kf->file_read(f, (void *)section_memory_start, s[i].filesize);
      #ifdef __pei_DEBUG__
      kf->log("The first binary: %x section size: %x\n\n", ((DWORD *)section_memory_start)[0], s[i].size);
      #endif
    }
  }

  /* Load the import symbols : I think import symbols should be load here */
  if(pee_info->import_symbol_size != 0)
  {
    BYTE *import_symbol_dir = (BYTE *)image_base+pee_info->import_symbol;
    struct imp_desc *desc = (struct imp_desc *)import_symbol_dir;
    struct dll_table *dt = NULL;
    char *dll_name;
    struct imp_name *imp;
    DWORD *hint;
    DWORD entry;
    DWORD *func;
    /* 
     * desc->tstamp is used to decide whether the symbol entry is set
     * desc->fchain is used to do forwarding
     */
    while(desc->orithunk != 0 || desc->name != 0 || desc->thunk != 0)
    {
      dll_name = (char *)image_base+desc->name;
      hint = (DWORD *)(image_base+desc->orithunk);
      entry = image_base+desc->thunk;
      #ifdef __PEI_DEBUG__
      kf->log("Find and Link DLL %s ...\n", dll_name);
      #endif
      for(i = 0; hint[i] != 0; i++, entry += 4)
      {
        dt = kf->get_dll_table(dll_name);
        if(dt == NULL)
        {
          kf->message("WARNING: DLL %s not found!\n", dll_name);
          /* Should try to load the target Dynalink lib in get_dll_table */
          /* Now just return 0 */
          return 0;
        }
        imp = (struct imp_name *)(image_base+hint[i]);
        /* TODO: QSORT and QSEARCH? */
        for(j = 0; j < dt->symbol_num; j++)
          if(strcmp(dt->symbol_table[j].name, imp->name) == 0)
          {
            func = (DWORD *)entry;
            *func = dt->symbol_table[j].address;
            break;
          }
        if(j == dt->symbol_num)
          kf->message("WARNING: Import Symbol %s not found in %s DLL!\n", imp->name, dll_name);
      }
      desc = (struct imp_desc *)((BYTE *)desc+sizeof(struct imp_desc));
    }
  }

  /* Save the essential info */
  *size = image_memory_size;
  /* Return the exec space */
  return image_base;
}

int pei_read_symbols(struct kern_funcs *kf, int f, struct table_info *tables, struct symbol_info *syms)
{
  int i;
  struct pei_extra_info *pee_info;
  DWORD image_base = tables->image_base;
  pee_info = (struct pei_extra_info *)tables->private_info;
  
  /* Process the export symbols : anyway we read export symbols here */
  if(tables->num_symbols != 0)
  {
    struct exp_dir *edir = (struct exp_dir *)(image_base+pee_info->export_symbol);
    struct symbol *symbol_array;
    DWORD *edir_funcname = (DWORD *)(image_base+edir->name_addr);
    DWORD *edir_funcaddr = (DWORD *)(image_base+edir->func_addr);
    DWORD handle = image_base;
#ifdef __PEI_DEBUG__
    kf->log("[PECOFF] DLL name: %s symbol number: %x\n", image_base+edir->name, edir->func_num);
#endif
    symbol_array = (struct symbol *)kf->mem_alloc(sizeof(struct symbol)*edir->func_num);
    for(i = 0; i < edir->func_num; i++)
    {
      syms[i].name = (char *)image_base+edir_funcname[i];
      /* Get the symbol's memory offset to section */
      syms[i].offset = edir_funcaddr[i]-pee_info->section_0_base;
      syms[i].section = 0;
      symbol_array[i].name = (char *)image_base+edir_funcname[i];
      symbol_array[i].address = image_base+edir_funcaddr[i];
      #ifdef __pei_DEBUG__
      kf->log("[PECOFF] Symbol name: %s\taddress: 0x%08x RVA 0x%08x\n", symbol_array[i].name, symbol_array[i].address, edir_funcaddr[i]);
      #endif
    }
    kf->add_dll_table((char *)image_base+edir->name, handle, edir->func_num, symbol_array);
  }
  
  return 1;
}

DWORD pei_import_symbol(struct kern_funcs *kf, int n, struct symbol_info *syms, char *name, int *sect)
{
  int i;
  
  for(i = 0; i < n; i++)
    if(strstr(syms[i].name, name) != 0)
    {
      *sect = syms[i].section;
      return syms[i].offset;
    }
  
  *sect = -1;
  return 0;
}

int pei_relocate_section(struct kern_funcs *kf, DWORD image_base, struct table_info *tables, int n, struct section_info *s, int sect, struct symbol_info *syms, struct symbol *import)
{
  int i;
  /* DWORD image_base = base-s[0].base; */
  /* Use to prevent the duplicated reloc block */
  DWORD reloc_memory_first_start = 0;
  int reloc_offset;
  int flag = 0;
  struct pe_reloc_info *r;
  DWORD base_reloc_at;
  BYTE *base_reloc_dir;

  /* The PE file only have one reloc info, we set it on the first section */
  if(s[0].reloc != NULL)
    r = (struct pe_reloc_info *)s[0].reloc;
  else
    return 0;
  reloc_offset = r->offset;
  /* Process the reloc symbols */
  base_reloc_dir = (BYTE *)r->addr;
  base_reloc_at = 0;
  while(base_reloc_at < r->size)
  {
    struct base_reloc *reloc = (struct base_reloc *)(base_reloc_dir+base_reloc_at);
    DWORD reloc_number = (reloc->block_size-IMAGE_SIZEOF_BASE_RELOCATION)>>1;
    DWORD reloc_memory_start = image_base+reloc->vaddr;
    DWORD offset;
    DWORD *mem;
    
    /* Prevent the duplicated reloc block */
    if(flag == 0) {
      reloc_memory_first_start = reloc_memory_start;
      flag = 1;
    } else if(reloc_memory_first_start == reloc_memory_start)
      break;
    #ifdef __PEI_DEBUG__
    kf->log("[PECOFF] RELOC vaddr: %x, block size: %x\n", reloc->vaddr, reloc->block_size);
    #endif
    for(i = 0; i < reloc_number; i++)
    {
      offset = reloc->type_offset[i].offset;
      if(offset != 0)
      {
        mem = (DWORD *)(reloc_memory_start+offset);
       
        switch(reloc->type_offset[i].type)
        {
          case IMAGE_REL_BASED_ABSOLUTE:
          /* This is a NOP. The fixup is skipped. */
            break;
          case IMAGE_REL_BASED_HIGH:
            *mem += reloc_offset&0xffff0000;
            break;
          case IMAGE_REL_BASED_LOW:
            *mem += reloc_offset&0x0000ffff;
            break;
          case IMAGE_REL_BASED_HIGHLOW:
            *mem += reloc_offset;
            break;
          default:
            kf->message("[PECOFF] Unsupported reloc type : %x\n", reloc->type_offset[i].type);
            return 0;
        }
      }
    }
    base_reloc_at += reloc->block_size;
  }

  return 1;
}

void pei_free_tables(struct kern_funcs *kf, struct table_info *tables, struct symbol_info *syms, struct section_info *scndata)
{
  struct pei_extra_info *pee_info;
  pee_info = (struct pei_extra_info *)tables->private_info;
  
  mem_free((DWORD)pee_info, sizeof(struct pei_extra_info));
  if(scndata[0].reloc != NULL)
    mem_free((DWORD)scndata[0].reloc, sizeof(struct pe_reloc_info));
  mem_free((DWORD)scndata, sizeof(struct section_info)*tables->num_sections);
  if(syms != NULL)
    mem_free((DWORD)syms, sizeof(struct symbol_info)*tables->num_symbols);
}

int isPEI(struct kern_funcs *kf, int f, struct read_funcs *rf)
{
  WORD magic;

  kf->file_offset = kf->file_seek(f, 0, kf->seek_cur);
  kf->file_read(f, &magic, 2);
  kf->file_seek(f, kf->file_offset + 0, kf->seek_set);
  if (magic != 0x014c)
  {
    return 0;
  }
  
  rf->read_headers = pei_read_headers;
  rf->read_section_headers = pei_read_section_headers;
  rf->read_symbols = pei_read_symbols;
  /* we use the common load */
  rf->load_executable = pei_load;
  rf->load_relocatable = pei_load;
  
  rf->relocate_section = pei_relocate_section;
  rf->import_symbol = pei_import_symbol;
  rf->free_tables = pei_free_tables;
  
  return 1;
}
