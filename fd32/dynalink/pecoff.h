/* Dynalynk
 * Portable dynamic linker for object files
 * PE COFF parser part
 * by Hanzac Chen
 * 
 * This is free software; see GPL.txt
 */

#ifndef __PECOFF_HDR__
#define __PECOFF_HDR__

/* winnt.h */
#define IMAGE_DIRECTORY_ENTRY_EXPORT          0
#define IMAGE_DIRECTORY_ENTRY_IMPORT          1
#define IMAGE_DIRECTORY_ENTRY_RESOURCE        2
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION       3
#define IMAGE_DIRECTORY_ENTRY_SECURITY        4
#define IMAGE_DIRECTORY_ENTRY_BASERELOC       5
#define IMAGE_DIRECTORY_ENTRY_DEBUG           6
#define IMAGE_DIRECTORY_ENTRY_COPYRIGHT       7
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR       8
#define IMAGE_DIRECTORY_ENTRY_TLS             9
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG     10
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT    11
#define IMAGE_DIRECTORY_ENTRY_IAT             12
#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT    13 // Delay Load Import Descriptors
#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR  14 // COM Runtime descriptor

#define IMAGE_SUBSYSTEM_UNKNOWN     0
#define IMAGE_SUBSYSTEM_NATIVE      1
#define IMAGE_SUBSYSTEM_WINDOWS_GUI 2
#define IMAGE_SUBSYSTEM_WINDOWS_CUI 3
#define IMAGE_SUBSYSTEM_OS2_CUI     5
#define IMAGE_SUBSYSTEM_POSIX_CUI   7
#define IMAGE_SUBSYSTEM_XBOX        14

#define IMAGE_SIZEOF_BASE_RELOCATION         8

//
// Based relocation types.
//

#define IMAGE_REL_BASED_ABSOLUTE              0
#define IMAGE_REL_BASED_HIGH                  1
#define IMAGE_REL_BASED_LOW                   2
#define IMAGE_REL_BASED_HIGHLOW               3
#define IMAGE_REL_BASED_HIGHADJ               4
#define IMAGE_REL_BASED_MIPS_JMPADDR          5
#define IMAGE_REL_BASED_MIPS_JMPADDR16        9
#define IMAGE_REL_BASED_IA64_IMM64            9
#define IMAGE_REL_BASED_DIR64                 10


struct data_directory {
  DWORD vaddr;
  DWORD size;
};

struct pe_aouthdr 
{
  WORD  magic;      /* type of file        */
  WORD  vstamp;      /* linker version stamp    */
  DWORD code_size;    /* text size        */
  DWORD data_size;    /* initialized data      */
  DWORD bss_size;    /* uninitialized data    */
  DWORD entry;      /* entry pt.        */
  DWORD text_base;    /* base of text used for this file */
  DWORD data_base;    /* base of data used for this file */
  DWORD image_base;   /* image linear memory base */
  
  DWORD sect_align;
  DWORD file_align;
  DWORD version[4];
  DWORD image_size;
  DWORD hdr_size;
  DWORD checksum;
  WORD  subsystem;
  WORD  dllflags;
  
  DWORD stackr_size;  /* stack reserved size  */
  DWORD stackc_size;  /* stack commit size  */
  DWORD heapr_size;    /* heap reserved size  */
  DWORD heapc_size;    /* heap commit size    */
  DWORD loader_flags;
  DWORD rva_num;
  
  struct data_directory data_dir[16]; /* tables and description */
};

struct pe_scnhdr {
  char  s_name[8];   /* section name    */
  DWORD s_vsize;    /* virtual size    */
  DWORD s_vaddr;    /* related address to imagebase */
  DWORD s_psize;  /* physical size  */
  DWORD s_paddr;  /* physical address (file offset) */
  DWORD s_reserved[3];
  DWORD s_flags;  /* flags      */
};

struct imp_name {
  WORD hint;
  BYTE name[1];
};

#if 0
struct thunk_data {
  union {
    BYTE *fstring;
    DWORD *function;
    DWORD ordinal;
    struct imp_name *daddr;
  };
};
#endif

struct imp_desc {
  DWORD orithunk;
  DWORD tstamp;
  DWORD fchain;
  DWORD name;
  DWORD thunk;
};

struct base_reloc {
  DWORD   vaddr;
  DWORD   block_size;
  struct
  {
    WORD offset : 12;
    WORD type   : 4;
  } type_offset[1];
};

struct exp_dir {
    DWORD characteristics;
    DWORD tstamp;
    DWORD version;
    DWORD name;
    DWORD base;
    DWORD func_num;
    DWORD name_num;
    DWORD func_addr;     // RVA from base of image
    DWORD name_addr;     // RVA from base of image
    DWORD nameord_addr;  // RVA from base of image
};


#endif
