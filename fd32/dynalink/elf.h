/* Dynalynk
 * Portable dynamic linker for object files
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#define	ELFCLASS32	1
#define ELFDATA2LSB	1
#define ET_EXEC		2 
#define EM_386   3

#define SHT_NULL	0
#define SHT_SYMTAB	2
#define SHT_RELA	4
#define SHT_NOBITS	8
#define SHT_REL		9
#define SHT_DYNSYM	11

#define R_386_32	1
#define R_386_PC32	2

#define SHN_UNDEF	0
#define SHN_COMMON	0xFFF2

#define EI_MAG0		0
#define EI_MAG1		1
#define EI_MAG2		2
#define EI_MAG3		3 
#define EI_CLASS	4
#define EI_DATA		5


#define EI_NIDENT	16

#define ELFMAG0		0x7f
#define ELFMAG1		'E'
#define ELFMAG2		'L'
#define ELFMAG3		'F'

#define PT_LOAD			1
#define PF_W			0x02

struct elf_header{
  BYTE		e_ident[EI_NIDENT];
  WORD		e_type;
  WORD		e_machine;
  DWORD		e_version;
  DWORD		e_entry;  /* Entry point */
  DWORD		e_phoff;
  DWORD		e_shoff;
  DWORD		e_flags;
  WORD		e_ehsize;
  WORD		e_phentsize;
  WORD		e_phnum;
  WORD		e_shentsize;
  WORD		e_shnum;
  WORD		e_shstrndx;
};

/* FIXME!!! */
typedef struct {
    DWORD p_type;
    DWORD p_offset;
    DWORD p_vaddr;
    DWORD p_paddr;
    DWORD p_filesz;
    DWORD p_memsz;
    DWORD p_flags;
    DWORD p_align;
} Elf32_Phdr;

struct elf_section_header {
  DWORD		sh_name;
  DWORD		sh_type;		/* Type of section */
  DWORD		sh_flags;		/* Miscellaneous section attributes */
  DWORD		sh_addr;		/* Section virtual addr at execution */
  DWORD		sh_offset;		/* Section file offset */
  DWORD		sh_size;		/* Size of section in bytes */
  DWORD		sh_link;		/* Index of another section */
  DWORD		sh_info;		/* Additional section information */
  DWORD		sh_addralign;	/* Section alignment */
  DWORD		sh_entsize;	/* Entry size if section holds table */
};

struct elf_symbol_info {
  DWORD		st_name;
  DWORD		st_value;
  DWORD		st_size;
  BYTE		st_info;
  BYTE		st_other;
  WORD		st_shndx;
};

struct elf_rel_info {
  DWORD		r_offset;
  DWORD		r_info;
};

struct elf_rela_info {
  DWORD		r_offset;
  DWORD		r_info;
  DWORD		r_addend;
};
