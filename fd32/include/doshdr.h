/* Sample boot code
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

struct dos_header {
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
  long int e_lfanew;	/* File address of new exe header	*/
};
