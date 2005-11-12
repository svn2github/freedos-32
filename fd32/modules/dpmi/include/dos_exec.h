#ifndef __DOS_EXEC_H__
#define __DOS_EXEC_H__

#include <ll/i386/hw-data.h>

typedef struct dos_header {
  WORD e_magic;		/* Magic number 			*/
  WORD e_cblp;		/* Bytes on last page of file		*/
  WORD e_cp;		/* Pages in file (size of the file in blocks)*/
  WORD e_crlc;		/* Number of Relocations		*/
  WORD e_cparhdr;	/* Size of header in paragraphs		*/
  WORD e_minalloc;	/* Minimum extra paragraphs needed	*/
  WORD e_maxalloc;	/* Maximum extra paragraphs needed	*/
  WORD e_ss;		/* Initial (relative) SS value		*/
  WORD e_sp;		/* Initial SP value			*/
  WORD e_csum;		/* Checksum				*/
  WORD e_ip;		/* Initial IP value			*/
  WORD e_cs;		/* Initial (relative) CS value		*/
  WORD e_lfarlc;	/* File address of relocation table	*/
  WORD e_ovno;		/* Overlay number			*/

  /* DJ Ends HERE!!! */

  WORD e_res[4];	/* Reserved words 			*/
  WORD e_oemid;		/* OEM identifier (for e_oeminfo) 	*/
  WORD e_oeminfo;	/* OEM information; e_oemid specific	*/
  WORD e_res2[10];	/* Reserved words			*/
  DWORD e_lfanew;	/* File address of new exe header	*/
} __attribute__ ((packed)) tDOSHD;

typedef struct dos_reloc {
  unsigned short offset;
  unsigned short segment;
} __attribute__ ((packed)) tDOSRLC;


typedef struct psp {
  WORD ps_exit;                 /* 00 CP/M-like exit point: int 20 */
  WORD ps_size;                 /* 02 segment of first byte beyond */
                                /*    memory allocated to program  */
  BYTE ps_fill1;                /* 04 single char fill             */

  /* CP/M-like entry point */
  BYTE ps_farcall;              /* 05  far call opcode   */
  void (*ps_reentry)(void);   /* 06  re-entry point    */
  DWORD  ps_isv22,              /* 0a  terminate address */
         ps_isv23,              /* 0e  break address     */
         ps_isv24;              /* 12 critical error address */
  WORD ps_parent;               /* 16 parent psp segment           */
  BYTE ps_files[20];            /* 18 file table - 0xff is unused  */
  WORD ps_environ;              /* 2c environment paragraph        */
  BYTE *ps_stack;               /* 2e user stack pointer - int 21  */
  WORD ps_maxfiles;             /* 32 maximum open files           */
  BYTE *ps_filetab;             /* 34 open file table pointer      */
  void *ps_prevpsp;             /* 38 previous psp pointer         */
  BYTE ps_fill2[20];            /* 3c */
  BYTE ps_unix[3];              /* 50 unix style call - 0xcd 0x21 0xcb */
  BYTE ps_fill3[3];             /* 53 */
  /* FD/32 items */
  void *dta;                    /* 56 FD32 use allocated from ps_fill3, under DOS this pointer is stored in the SDA */
  WORD stubinfo_sel;
  BYTE def_fcb_1[16];
  BYTE def_fcb_2[20];
  /* Offset 80h (128): this is also the start of the default DTA */
  BYTE command_line_len;
  BYTE command_line[127];
} __attribute__ ((packed)) tPsp;

int dos_exec(char *filename, DWORD env_segment, char *args,
		DWORD fcb1, DWORD fcb2, WORD *return_val);

#define RUN_RING		0
#define MAX_OPEN_FILES	0x30
#define DOS_VM86_EXEC		0
#define DOS_DIRECT_EXEC		1 /* Support COFF-GO32 only */
#define DOS_WRAPPER_EXEC	2
int dos_exec_switch(int option); /* Return TRUE(1) or FALSE(0) */
extern void (*dos_exec_mode16)(void); /* 16bit DPMI mode-switch */

#endif
