/* Sample boot code
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#ifndef __WRAP_H__
#define __WRAP_H__

#include <ll/i386/hw-data.h>
#include "kernel.h"

#define VERSION "0.1"
#define RUN_RING 0

extern WORD user_cs, user_ds;

/* from dpmi/src/dos_exec.c, using dynamic linking */
WORD stubinfo_init(DWORD base, DWORD initial_size, DWORD mem_handle, char *filename, char *args);
void restore_psp(void);

/* similar with fd32/kernel/exec.c */
int my_exec_process(struct kern_funcs *p, int file, struct read_funcs *parser, char *filename, char *args);

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
  WORD e_oemid;	        /* OEM identifier (for e_oeminfo) 	*/
  WORD e_oeminfo;	/* OEM information; e_oemid specific	*/
  WORD e_res2[10];	/* Reserved words			*/
  DWORD e_lfanew;	/* File address of new exe header	*/
} __attribute__ ((packed)) tDOSHD;

/* NOTE: Move the structure here, Correct? */
struct stubinfo {
  char magic[16];
  DWORD size;
  DWORD minstack;
  DWORD memory_handle;
  DWORD initial_size;
  WORD minkeep;
  WORD ds_selector;
  WORD ds_segment;
  WORD psp_selector;
  WORD cs_selector;
  WORD env_size;
  char basename[8];
  char argv0[16];
  char dpmi_server[16];
  /* FD/32 items */
  DWORD dosbuf_handler;
};

#endif
