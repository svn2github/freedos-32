/* Dynalynk
 * Portable dynamic linker for object files
 * PE COFF parser part
 * by Hanzac Chen
 * 
 * This is free software; see GPL.txt
 */
 
#include <ll/i386/hw-data.h>
#include <ll/i386/mem.h>
#include <ll/i386/string.h>

#include "kmem.h"
#include "format.h"
#include "common.h"
#include "coff.h"
#include "pecoff.h"

#define __PECOFF_DEBUG__

struct pecoff_extra_info {
  DWORD export_symbol;
  DWORD export_symbol_size;
  DWORD import_symbol;
  DWORD import_symbol_size;
  DWORD base_reloc;
  DWORD base_reloc_size;
  DWORD base_reloc_offset;
};


DWORD PECOFF_read_headers(struct kern_funcs *kf, int f, struct table_info *tables)
{
  DWORD entry;
  struct external_filehdr header;
  struct pe_aouthdr optheader;
  struct pecoff_extra_info *pee_info = (struct pecoff_extra_info *)kf->mem_alloc(sizeof(struct pecoff_extra_info));
  kf->file_seek(f, kf->file_offset, 0);
  kf->file_read(f, &header, sizeof(struct external_filehdr));
  
  if (header.f_magic != 0x014c)
  {
    kf->message("Not a PE COFF file (Wrong Magic: 0x%x)!!!\n", header.f_magic);
    return 0;
  }
  #ifdef __PECOFF_DEBUG__
  kf->log("Number of sections: %d\n", header.f_nscns);
  kf->log("Flags: %x\n", header.f_flags);
  #endif
  tables->num_sections = header.f_nscns;
  tables->flags = /*NEED_IMAGE_RELOCATION*/ 0;
  tables->image_base = 0;
  
  entry = 0;
  if (header.f_opthdr !=0)
  {
    kf->file_read(f, &optheader, header.f_opthdr);
    if (optheader.magic != 0x010b)
    {
      kf->message("Wrong Magic (0x%x)\n", optheader.magic);
    }
    else
    {
      entry = optheader.entry;
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

int PECOFF_read_section_headers(struct kern_funcs *kf, int f, struct table_info *tables, struct section_info *s)
{
  int i, n;
  char name[9];
  int e_section = -1;
  struct pe_scnhdr h;
  struct pecoff_extra_info *pee_info;
  pee_info = (struct pecoff_extra_info *)tables->private_info;

  kf->file_seek(f, tables->section_header, 0);
  n = tables->num_sections;
  for (i = 0; i < n; i++)
  {
    kf->file_read(f, &h, sizeof(struct pe_scnhdr));

    memcpy(name, h.s_name, 8);
    name[8] = 0;
    #ifdef __PECOFF_DEBUG__
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

  /* The number of symbols should be export symbols */
  tables->num_symbols = 0;
  /* Process the export symbols : calculate the symbols' number */
  if(pee_info->export_symbol_size != 0 && e_section != -1)
  {
    struct exp_dir edir;
    kf->file_seek(f, s[e_section].fileptr+pee_info->export_symbol-s[e_section].base, 0);
    kf->file_read(f, (void *)&edir, sizeof(struct exp_dir));
    #ifdef __PECOFF_DEBUG__
    kf->log("Export symbol number: %d\n", edir.func_num);
    #endif
    tables->num_symbols = edir.func_num;
  }
  
  /* Is the bss variable needed, I only set it equal to 1 */
  return -1;
}

DWORD PECOFF_load(struct kern_funcs *kf, int f, struct table_info *tables, int n, struct section_info *s, int *size)
{
  int res;
  int i, j;
  DWORD reloc_offset = 0;
  DWORD image_memory_size;
  DWORD image_memory_start;
  DWORD section_memory_start;
  struct pecoff_extra_info *pee_info;
  DWORD image_base = tables->image_base;
  pee_info = (struct pecoff_extra_info *)tables->private_info;

  /* Load this sections to process the symbols and reloc info */
  image_memory_size = s[n - 1].base + s[n - 1].filesize - s[0].base;
  #ifdef __PECOFF_DEBUG__
  kf->log("Trying to get %lx to load the COFF\n", image_memory_size);
  #endif
  image_memory_start = s[0].base+image_base;
  res = kf->mem_alloc_region(image_memory_start, image_memory_size);
  if (res == -1) {
    #ifdef __PECOFF_DEBUG__
    kf->log("If the pe image starts at %lx, no enough memory\n", image_memory_start);
    #endif
    /* Decide whether relocatable or not */
    if(pee_info->base_reloc_size == 0)
    {
      kf->message("The pe image isn't relocated!\n");
      return -1;
    }
    image_memory_start = (DWORD)(kf->mem_alloc(image_memory_size));
    reloc_offset = image_memory_start-s[0].base-image_base;
    #ifdef __PECOFF_DEBUG__
    kf->log("Located at %lx instead\n", image_memory_start);
    kf->log("Relocated offset: %lx\n", reloc_offset);
    #endif
  }

  if (image_memory_start == 0) {
    kf->message("Not enough memory to load the pe image\n");
    kf->message("Needed memory = %lu bytes - image memory starts = 0x%lx\n", image_memory_size, s[0].base);
    return -1;
  }
  
  /* The new image base */
  image_base = image_base+reloc_offset;
  tables->image_base = image_base;
  
  if(reloc_offset != 0)
  {
    struct pe_reloc_info *r;
    /* Set the relocation method */
    /* The PE file only have one reloc info, we set it on the first section */
    tables->flags |= NEED_IMAGE_RELOCATION;
    s[0].num_reloc = 1;
    r = (struct pe_reloc_info *)kf->mem_alloc(sizeof(struct pe_reloc_info));
    r->offset = reloc_offset;
    r->addr = pee_info->base_reloc;
    r->size = pee_info->base_reloc_size;
    s[0].reloc = (struct reloc_info *)r;
  }

  /* .text & .data section & other sections */
  for (i = 0; i < n; i++) {
    section_memory_start = image_base+s[i].base;
    #ifdef __PECOFF_DEBUG__
    kf->log("Loaded section %d at 0x%lx [file offset %x]\n", i, section_memory_start, s[i].fileptr);
    #endif
    memset((void *)section_memory_start, 0, s[i].size);
    if (s[i].fileptr != 0) {
      kf->file_seek(f, s[i].fileptr, 0);
      kf->file_read(f, (void *)section_memory_start, s[i].filesize);
      #ifdef __PECOFF_DEBUG__
      kf->log("The first binary: %x section size: %x\n\n", ((DWORD *)section_memory_start)[0], s[i].size);
      #endif
    }
  }

  /* Load the import symbols : I think import symbols should be load here */
  if(pee_info->import_symbol_size != 0)
  {
    BYTE *import_symbol_dir = (BYTE *)image_base+pee_info->import_symbol;
    struct imp_desc *desc = (struct imp_desc *)import_symbol_dir;
    struct dll_table *dylt = NULL;
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
      #ifdef __PECOFF_DEBUG__
      kf->log("Try to link dynalink lib %s!\n", dll_name);
      #endif
      for(i = 0; hint[i] != 0; i++, entry += 4)
      {
        dylt = kf->get_dll_table(dll_name);
        if(dylt == NULL)
        {
          kf->message("Dynalink lib %s not found!\n", dll_name);
          /* Should try to load the target Dynalink lib in get_dll_table */
          /* Now just return 0 */
          return 0;
        }
        imp = (struct imp_name *)(image_base+hint[i]);
        /* We should use better algorithm to find the entry Á½·Ö·¨ */
        for(j = 0; j < dylt->symbol_num; j++)
          if(strcmp(dylt->symbol_array[j].name, imp->name) == 0)
          {
            func = (DWORD *)entry;
            *func = dylt->symbol_array[j].address;
            goto IMPORTED_SYMBOL;
          }
        kf->message("Import symbol %s not found in %s dynalink lib!\n", imp->name, dll_name);
        IMPORTED_SYMBOL:
          continue;
      }
      desc = (struct imp_desc *)((BYTE *)desc+sizeof(struct imp_desc));
    }
  }

  /* Save the essential info */
  *size = image_memory_size;
  return image_memory_start;
}

int PECOFF_read_symbols(struct kern_funcs *kf, int f, struct table_info *tables, struct symbol_info *syms)
{
  int i;
  struct pecoff_extra_info *pee_info;
  DWORD image_base = tables->image_base;
  pee_info = (struct pecoff_extra_info *)tables->private_info;
  
  /* Process the export symbols : anyway we read export symbols here */
  if(tables->num_symbols != 0)
  {
    struct exp_dir *edir = (struct exp_dir *)(image_base+pee_info->export_symbol);
    struct symbol *symbol_array;
    DWORD *edir_funcname = (DWORD *)(image_base+edir->name_addr);
    DWORD *edir_funcaddr = (DWORD *)(image_base+edir->func_addr);
    DWORD handle = image_base;
#ifdef __PECOFF_DEBUG__
    kf->log("Dynalink lib name: %s Func number: %x\n", image_base+edir->name, edir->func_num);
#endif
    symbol_array = (struct symbol *)kf->mem_alloc(sizeof(struct symbol)*edir->func_num);
    for(i = 0; i < edir->func_num; i++)
    {
      syms[i].name = (char *)image_base+edir_funcname[i];
      syms[i].offset = edir_funcaddr[i];
      syms[i].section = 0;
      symbol_array[i].name = (char *)image_base+edir_funcname[i];
      symbol_array[i].address = image_base+edir_funcaddr[i];
      #ifdef __PECOFF_DEBUG__
      kf->log("Func name: %s Func address: %x RVA %x\n", symbol_array[i].name, symbol_array[i].address, edir_funcaddr[i]);
      #endif
    }
    kf->add_dll_table((char *)image_base+edir->name, handle, edir->func_num, symbol_array);
  }
  
  return 1;
}

DWORD PECOFF_import_symbol(struct kern_funcs *kf, int n, struct symbol_info *syms, char *name, int *sect)
{
  int i;
  
  for(i = 0; i < n; i++)
    if(strcmp(syms[i].name, name) == 0)
    {
      *sect = syms[i].section;
      return syms[i].offset;
    }
  
  *sect = -1;
  return 0;
}

int PECOFF_relocate_section(struct kern_funcs *kf, DWORD base, DWORD bssbase, struct section_info *s, int sect, struct symbol_info *syms, struct symbol *import)
{
  int i;
  DWORD image_base = base-s[0].base;
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
  base_reloc_dir = (BYTE *)image_base + r->addr;
  base_reloc_at = 0;
  
  while(base_reloc_at < r->size)
  {
    struct base_reloc *reloc = (struct base_reloc *)(base_reloc_dir+base_reloc_at);
    DWORD reloc_number = (reloc->block_size-IMAGE_SIZEOF_BASE_RELOCATION)>>1;
    DWORD reloc_memory_start = image_base+reloc->vaddr;
    DWORD offset;
    DWORD *mem;
    
    /* Prevent the duplicated reloc block */
    if(flag == 0)
    {
      reloc_memory_first_start = reloc_memory_start;
      flag = 1;
    }
    else if(reloc_memory_first_start == reloc_memory_start)
      break;
    #ifdef __PECOFF_DEBUG__
    kf->log("Reloc vaddr: %x, block size: %x\n", reloc->vaddr, reloc->block_size);
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
          /* Don't understand this relocation.
          case IMAGE_REL_BASED_HIGHADJ:
            break;
           */
          default:
            kf->message("Unsupported reloc type : %x\n", reloc->type_offset[i].type);
            return 0;
        }
      }
    }
    base_reloc_at += reloc->block_size;
  }

  return 1;
}
void PECOFF_free_tables(struct kern_funcs *kf, struct table_info *tables, struct symbol_info *syms, struct section_info *scndata)
{
  struct pecoff_extra_info *pee_info;
  pee_info = (struct pecoff_extra_info *)tables->private_info;
  
  mem_free((DWORD)pee_info, sizeof(struct pecoff_extra_info));
  if(scndata[0].reloc != NULL)
    mem_free((DWORD)scndata[0].reloc, sizeof(struct pe_reloc_info));
  mem_free((DWORD)scndata, sizeof(struct section_info)*tables->num_sections);
  if(syms != NULL)
    mem_free((DWORD)syms, sizeof(struct symbol_info)*tables->num_symbols);
}

int isPECOFF(struct kern_funcs *kf, int f, struct read_funcs *rf)
{
  WORD magic;

  kf->file_offset = kf->file_seek(f, 0, kf->seek_cur);
  kf->file_read(f, &magic, 2);
  kf->file_seek(f, kf->file_offset + 0, kf->seek_set);
  if (magic != 0x014c)
  {
    return 0;
  }
  
  rf->read_headers = PECOFF_read_headers;
  rf->read_section_headers = PECOFF_read_section_headers;
  rf->read_symbols = PECOFF_read_symbols;
  /* we use the common load */
  rf->load_executable = PECOFF_load;
  rf->load_relocatable = PECOFF_load;
  
  rf->relocate_section = PECOFF_relocate_section;
  rf->import_symbol = PECOFF_import_symbol;
  rf->free_tables = PECOFF_free_tables;
  
  return 1;
}
