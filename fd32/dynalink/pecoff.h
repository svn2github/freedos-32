/* Dynalynk
 * Portable dynamic linker for object files
 * PE COFF parser part
 * by Hanzac Chen
 * 
 * This is free software; see GPL.txt
 */

#ifndef __PECOFF_HDR__
#define __PECOFF_HDR__

/* from w32api/include/winnt.h */
#define IMAGE_SIZEOF_FILE_HEADER        20
// File types
#define IMAGE_FILE_RELOCS_STRIPPED           0x0001  // Relocation info stripped from file.
#define IMAGE_FILE_EXECUTABLE_IMAGE          0x0002  // File is executable  (i.e. no unresolved externel references).
#define IMAGE_FILE_LINE_NUMS_STRIPPED        0x0004  // Line nunbers stripped from file.
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED       0x0008  // Local symbols stripped from file.
#define IMAGE_FILE_AGGRESIVE_WS_TRIM         0x0010  // Agressively trim working set
#define IMAGE_FILE_LARGE_ADDRESS_AWARE       0x0020  // App can handle >2gb addresses
#define IMAGE_FILE_BYTES_REVERSED_LO         0x0080  // Bytes of machine word are reversed.
#define IMAGE_FILE_32BIT_MACHINE             0x0100  // 32 bit word machine.
#define IMAGE_FILE_DEBUG_STRIPPED            0x0200  // Debugging info stripped from file in .DBG file
#define IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP   0x0400  // If Image is on removable media, copy and run from the swap file.
#define IMAGE_FILE_NET_RUN_FROM_SWAP         0x0800  // If Image is on Net, copy and run from the swap file.
#define IMAGE_FILE_SYSTEM                    0x1000  // System File.
#define IMAGE_FILE_DLL                       0x2000  // File is a DLL.
#define IMAGE_FILE_UP_SYSTEM_ONLY            0x4000  // File should only be run on a UP machine
#define IMAGE_FILE_BYTES_REVERSED_HI         0x8000  // Bytes of machine word are reversed.
#define IMAGE_FILE_MACHINE_UNKNOWN           0
#define IMAGE_FILE_MACHINE_I386              0x014c  // Intel 386.
#define IMAGE_FILE_MACHINE_ARM               0x01c0  // ARM Little-Endian
#define IMAGE_FILE_MACHINE_POWERPC           0x01F0  // IBM PowerPC Little-Endian
#define IMAGE_FILE_MACHINE_IA64              0x0200  // Intel 64
#define IMAGE_FILE_MACHINE_AMD64             0x8664  // AMD64 (K8)

// Directory Entries
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

// Subsystem Values
#define IMAGE_SUBSYSTEM_UNKNOWN              0   // Unknown subsystem.
#define IMAGE_SUBSYSTEM_NATIVE               1   // Image doesn't require a subsystem.
#define IMAGE_SUBSYSTEM_WINDOWS_GUI          2   // Image runs in the Windows GUI subsystem.
#define IMAGE_SUBSYSTEM_WINDOWS_CUI          3   // Image runs in the Windows character subsystem.
#define IMAGE_SUBSYSTEM_OS2_CUI              5   // image runs in the OS/2 character subsystem.
#define IMAGE_SUBSYSTEM_POSIX_CUI            7   // image runs in the Posix character subsystem.
#define IMAGE_SUBSYSTEM_NATIVE_WINDOWS       8   // image is a native Win9x driver.
#define IMAGE_SUBSYSTEM_WINDOWS_CE_GUI       9   // Image runs in the Windows CE subsystem.
#define IMAGE_SUBSYSTEM_EFI_APPLICATION      10
#define IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER  11
#define IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER   12
#define IMAGE_SUBSYSTEM_EFI_ROM              13
#define IMAGE_SUBSYSTEM_XBOX                 14

#define IMAGE_SIZEOF_BASE_RELOCATION         8
// Based relocation types.
#define IMAGE_REL_BASED_ABSOLUTE             0
#define IMAGE_REL_BASED_HIGH                 1
#define IMAGE_REL_BASED_LOW                  2
#define IMAGE_REL_BASED_HIGHLOW              3
#define IMAGE_REL_BASED_HIGHADJ              4


struct data_directory {
  DWORD vaddr;
  DWORD size;
};

#define PE32MAGIC  0x010B

struct pe_aouthdr 
{
  WORD  magic;        /* type of file */
  WORD  vstamp;       /* linker version stamp */
  DWORD code_size;    /* text size */
  DWORD data_size;    /* initialized data */
  DWORD bss_size;     /* uninitialized data  */
  DWORD entry;        /* entry pt. */
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
  
  DWORD stackr_size;   /* stack reserved size */
  DWORD stackc_size;   /* stack commit size */
  DWORD heapr_size;    /* heap reserved size */
  DWORD heapc_size;    /* heap commit size */
  DWORD loader_flags;
  DWORD rva_num;
  
  struct data_directory data_dir[16]; /* tables and description */
};

struct pe_scnhdr {
  char  s_name[8];   /* section name */
  DWORD s_vsize;     /* virtual size */
  DWORD s_vaddr;     /* related address to imagebase */
  DWORD s_psize;     /* physical size */
  DWORD s_paddr;     /* physical address (file offset) */
  DWORD s_reserved[3];
  DWORD s_flags;     /* flags */
};

struct imp_name {
  WORD hint;
  BYTE name[1];
};

union thunk_data {
  BYTE *fstring;
  DWORD *function;
  DWORD ordinal;
  struct imp_name *daddr;
};

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
