/* Dynalynk
 * Portable dynamic linker for object files
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#ifndef __COFF_HDR__
#define __COFF_HDR__

struct external_filehdr {
  WORD f_magic;		/* magic number			*/
  WORD f_nscns;		/* number of sections		*/
  DWORD f_timdat;	/* time & date stamp		*/
  DWORD f_symptr;	/* file pointer to symtab	*/
  DWORD f_nsyms;	/* number of symtab entries	*/
  WORD f_opthdr;	/* sizeof(optional hdr)		*/
  WORD f_flags;		/* flags			*/
};


/* Bits for f_flags:
 *	F_RELFLG	relocation info stripped from file
 *	F_EXEC		file is executable (no unresolved external references)
 *	F_LNNO		line numbers stripped from file
 *	F_LSYMS		local symbols stripped from file
 *	F_AR32WR	file has byte ordering of an AR32WR machine (e.g. vax)
 */

#define F_RELFLG	(0x0001)
#define F_EXEC		(0x0002)
#define F_LNNO		(0x0004)
#define F_LSYMS		(0x0008)


#define	I386MAGIC	0x14c
#define I386AIXMAGIC	0x175
#define I386BADMAG(x) (((x).f_magic!=I386MAGIC) && (x).f_magic!=I386AIXMAGIC)

struct aout_hdr 
{
  WORD 	magic;		/* type of file				*/
  WORD	vstamp;		/* version stamp			*/
  DWORD	tsize;		/* text size in bytes, padded to FW bdry*/
  DWORD	dsize;		/* initialized data "  "		*/
  DWORD	bsize;		/* uninitialized data "   "		*/
  DWORD	entry;		/* entry pt.				*/
  DWORD text_start;	/* base of text used for this file */
  DWORD data_start;	/* base of data used for this file */
};

struct gnu_aout {
  DWORD info;
  DWORD tsize;
  DWORD dsize;
  DWORD bsize;
  DWORD symsize;
  DWORD entry;
  DWORD txrel;
  DWORD dtrel;
};

#define OMAGIC          0404    /* object files, eg as output */
#define ZMAGIC          0413    /* demand load format, eg normal ld output */
#define STMAGIC		0401	/* target shlib */
#define SHMAGIC		0443	/* host   shlib */

struct external_scnhdr {
  char s_name[8];       /* section name			*/
  DWORD s_paddr;	/* physical address, aliased s_nlib */
  DWORD s_vaddr;	/* virtual address		*/
  DWORD s_size;		/* section size			*/
  DWORD s_scnptr;	/* file ptr to raw data for section */
  DWORD s_relptr;	/* file ptr to relocation	*/
  DWORD s_lnnoptr;	/* file ptr to line numbers	*/
  WORD s_nreloc;	/* number of relocation entries	*/
  WORD s_nlnno;		/* number of line number entries*/
  DWORD s_flags;	/* flags			*/
};

/*
 * names of "special" sections
 */
#define _TEXT	".text"
#define _DATA	".data"
#define _BSS	".bss"
#define _COMMENT ".comment"
#define _LIB ".lib"

/*
 * s_flags "type"
 */
#define SECT_NOLOAD	 (0x0002)
#define SECT_TEXT	 (0x0020)	/* section contains text only */
#define SECT_INIT_DATA	 (0x0040)	/* section contains data only */
#define SECT_UNINIT_DATA (0x0080)	/* section contains bss only */
#define SECT_DISCARDABLE (0x02000000)

struct coff_reloc_info {
  DWORD r_vaddr __attribute__((packed));
  DWORD r_symndx __attribute__((packed));
  WORD r_type;
};

struct coff_symbol_info {
  union {
    char e_name[8];
    struct {
      DWORD e_zeroes __attribute__((packed));
      DWORD e_offset __attribute__((packed));
    } e;
  } e;
  DWORD e_value __attribute__((packed));
  short int e_scnum;
  WORD e_type;
  BYTE e_sclass;
  BYTE e_numaux;
};

#endif
