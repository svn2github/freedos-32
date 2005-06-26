/* Dynalynk
 * Portable dynamic linker for object files
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#ifndef __FORMAT_H__
#define __FORMAT_H__

#define EXTERN_SYMBOL 0xFF00
#define COMMON_SYMBOL 0xFF01

#define REL_TYPE_ELF_ABSOLUTE   1
#define REL_TYPE_COFF_ABSOLUTE  2
#define REL_TYPE_RELATIVE       4

#define NEED_IMAGE_RELOCATION	1
#define NEED_SECTION_RELOCATION	2
#define NEED_LOAD_RELOCATABLE	4
#define NO_ENTRY		16
#define DLL_WITH_STDCALL	32

struct symbol_info {
  char *name;
  DWORD offset;
  WORD section;
};

/* HACKME!!! */
struct reloc_info {
  DWORD offset;
  int symbol;
  int type;
};
struct pe_reloc_info {
  DWORD offset;
  DWORD addr;
  DWORD size;
};

struct bss_symbols {
  int sym_idx;
  DWORD addr;
};

struct table_info {
  DWORD section_header;
  WORD num_sections;
  WORD section_header_size;
  WORD flags;
  DWORD symbol;
  DWORD num_symbols;
  DWORD symbol_size;
  DWORD string;
  DWORD string_size;
  DWORD string_buffer;
  char *section_names;
  DWORD section_names_size;
  
  DWORD image_base;
  DWORD private_info;  /* support pe format */
};

struct section_info {
  DWORD base;
  DWORD size;
  DWORD fileptr;
  DWORD filesize;
  int num_reloc;
  struct reloc_info *reloc;
};

/* Only used for ``import'' symbols... */
struct symbol {
  char *name;
  DWORD address;
};

struct dll_table
{
  char *name;                  /* identify the dll name */
  DWORD symbol_num;
  struct symbol *symbol_array; /* symbols' infomation */
  /* Note that all the symbols should have an ordinal number
   * Because sometimes an application just want to use the
   * ordinal number to search for a function entry.
   * Or maybe we can just use the array index as the ordinal */
};



struct kern_funcs {
    int (*file_read)(int file, void *buffer, int len);
    int (*file_seek)(int file, int position, int wence);
#if 0
    DWORD (*mem_alloc)(int size);
    DWORD (*mem_alloc_region)(DWORD address, int size);
    void (*mem_free)(DWORD address, int size);
#else
    DWORD (*mem_alloc)(DWORD size);
    int (*mem_alloc_region)(DWORD address, DWORD size);
    int (*mem_free)(DWORD address, DWORD size);
#endif
    int (*error)(char *fmt, ...);
    int (*message)(char *fmt, ...);
    int (*log)(char *fmt, ...);


    int (*add_dll_table)(char *dll_name, DWORD handle, DWORD symbol_num,
		    struct symbol *symbol_array);
    struct dll_table *(*get_dll_table)(char *dll_name);

    
    int seek_set;
    int seek_cur;

    int file_offset;
};




struct read_funcs {
  DWORD (* read_headers)(struct kern_funcs *kf, int f,
		  struct table_info *tables);
  int (* read_section_headers)(struct kern_funcs *kf, int f,
		  struct table_info *tables, struct section_info *scndata);
  int (* read_symbols)(struct kern_funcs *kf, int f,
		  struct table_info *tables, struct symbol_info *syms);
  DWORD (* load_relocatable)(struct kern_funcs *kf, int f,
		  struct table_info *tables,
		  int n, struct section_info *s, int *size);
  DWORD (* load_executable)(struct kern_funcs *kf, int f,
		  struct table_info *tables,
		  int n, struct section_info *s, int *size);
  int (* relocate_section)(struct kern_funcs *p,DWORD base, DWORD bssbase,
		struct section_info *s, int sect,
		struct symbol_info *syms,
		struct symbol *import);
  DWORD (* import_symbol)(struct kern_funcs *p, int n, struct symbol_info *syms,
		char *name, int *sect);
  void (* free_tables)(struct kern_funcs *p, struct table_info *tables, struct symbol_info *syms,
		  struct section_info *scndata);
};
  
int isPECOFF(struct kern_funcs *kf, int f, struct read_funcs *rf);
int isCOFF(struct kern_funcs *kf, int f, struct read_funcs *rf);
int isELF(struct kern_funcs *kf, int f, struct read_funcs *rf);

#define LOCAL_BSS 64 * 1024


#endif
