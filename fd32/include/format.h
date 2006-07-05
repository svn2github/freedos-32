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
#define NULL_SYMBOL   0xFFFF

#define REL_TYPE_ELF_ABSOLUTE   1
#define REL_TYPE_COFF_ABSOLUTE  2
#define REL_TYPE_RELATIVE       4

#define NEED_IMAGE_RELOCATION	1
#define NEED_SECTION_RELOCATION	2
#define NEED_LOAD_RELOCATABLE	4
#define NO_ENTRY		16
#define DLL_WITH_STDCALL	32

/* module formats */
/*
#define MOD_ASCII   1
#define MOD_COFF    2
#define MOD_ELF     3
#define MOD_MZ      4
#define MOD_UNKNOWN 5
#define MOD_PECOFF  6

*/

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
  
  DWORD local_bss;
  DWORD local_bss_size;
  /* In PEI, it's a pei extra info */
  DWORD private_info;
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
  const char *name;
  void *address;
};

/** \struct dll_table */
struct dll_table
{
  char *name;                  /***< DLL name */
  DWORD symbol_num;            /***< symbols' number */
  struct symbol *symbol_table; /***< symbols' list */
  /** \note All the symbols should have an ordinal number
   *        because sometimes an application just want to use
   *        the ordinal number to search for a function entry.
   *        Or maybe we can just use the array index as the ordinal
   */
};



struct kern_funcs {
    int (*file_read)(int file, void *buffer, int len);
    int (*file_seek)(int file, int position, int wence);
    DWORD (*mem_alloc)(DWORD size);
    int (*mem_alloc_region)(DWORD address, DWORD size);
    int (*mem_free)(DWORD address, DWORD size);
    int (*error)(char *fmt, ...);
    int (*message)(char *fmt, ...);
    int (*log)(char *fmt, ...);

    int (*add_dll_table)(char *dll_name, DWORD handle, DWORD symbol_num,
			struct symbol *symbol_table);
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
		int n, struct section_info *s, DWORD *size);
  DWORD (* load_executable)(struct kern_funcs *kf, int f,
		struct table_info *tables,
		int n, struct section_info *s, DWORD *size);
  int (* relocate_section)(struct kern_funcs *kf, DWORD base,
		struct table_info *tables,
		int n, struct section_info *s, int sect,
		struct symbol_info *syms, struct symbol *import);
  DWORD (* import_symbol)(struct kern_funcs *kf, int n, struct symbol_info *syms,
		char *name, int *sect);
  void (* free_tables)(struct kern_funcs *kf, struct table_info *tables, struct symbol_info *syms,
		struct section_info *scndata);
};
  
int isPEI(struct kern_funcs *kf, int f, struct read_funcs *rf);
int isCOFF(struct kern_funcs *kf, int f, struct read_funcs *rf);
int isELF(struct kern_funcs *kf, int f, struct read_funcs *rf);

typedef int (*check_func_t)(struct kern_funcs *kf, int f, struct read_funcs *rf);
typedef int (*exec_func_t)(struct kern_funcs *kf, int f, struct read_funcs *rf, char *cmdline, char *args);

/** \struct bin_format */
struct bin_format {
  const char *name;   /***< binary format name */
  check_func_t check; /***< check the file, return TRUE or FALSE */
  exec_func_t exec;   /***< exec the file, return value from the program */
  /* struct kern_funcs *kf; */
  /* struct read_funcs *rf; */

  /* struct bin_format *sub_binfmt; */ /***< used for embedded object support, e.g. MZ can embed COFF/LE/NE/PE */
};

/** \brief Binary format management functions */

/** \brief return the pointer of a format object array, NULL is the last element */
struct bin_format *fd32_get_binfmt(void);
/** \brief Set a new/old format, return TRUE(1) or FALSE(0) */
int fd32_set_binfmt(const char *name, check_func_t check, exec_func_t exec);

/** \brief Select COFF object type: "djcoff" or "pecoff" */
void COFF_set_type(char *value);


#endif
