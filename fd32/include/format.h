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

#define REL_TYPE_ELF_ABSOLUTE 1
#define REL_TYPE_COFF_ABSOLUTE 2
#define REL_TYPE_RELATIVE 3

struct symbol_info {
  char *name;
  DWORD offset;
//  DWORD off1;
  WORD section;
};

struct reloc_info {
  DWORD offset;
  int symbol;
  int type;
};

struct bss_symbols {
  int sym_idx;
  DWORD addr;
};

struct table_info {
  WORD section_header;
  WORD num_sections;
  WORD section_header_size;
  DWORD symbol;
  DWORD num_symbols;
  DWORD symbol_size;
  DWORD string;
  DWORD string_size;
  char *section_names;
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

struct kern_funcs {
    int (*file_read)(int file, void *buffer, int len);
    int (*file_seek)(int file, int position, int wence);
    DWORD (*mem_alloc)(int size);
    DWORD (*mem_alloc_region)(DWORD address, int size);
    void (*mem_free)(DWORD address, int size);
    int (*error)(char *fmt, ...);
    int (*message)(char *fmt, ...);
    int (*log)(char *fmt, ...);
    int seek_set;
    int seek_cur;
};




struct read_funcs {
  DWORD (* read_headers)(struct kern_funcs *kf, int f,
		  struct table_info *tables);
  int (* read_section_headers)(struct kern_funcs *kf, int f,
		  struct table_info *tables, struct section_info *scndata);
  int (* read_symbols)(struct kern_funcs *kf, int f,
		  struct table_info *tables, struct symbol_info *syms);
  DWORD (* load_relocatable)(struct kern_funcs *kf, int f,
		  int n, struct section_info *s, int *size);
  DWORD (* load_executable)(struct kern_funcs *kf, int f,
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
  
int isCOFF(struct kern_funcs *kf, int f, struct read_funcs *rf);
int isELF(struct kern_funcs *kf, int f, struct read_funcs *rf);
#endif
