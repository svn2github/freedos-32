/*
// Program:  Format
//
// Modified By: Hanzac Chen, 2005
// For being compiled by DJGPP, compatible and portable
//
// Written By:  Brian E. Reifsnyder
// Version:  0.91s
// (0.90b ... 0.90f and 0.91b ... 0.91j are updates by Eric Auer 2003)
// (0.91k ... are by Eric Auer 2004)
// Copyright:  2002-2004 under the terms of the GNU GPL, Version 2.
// Module Name:  FORMAT.H
// Module Description:  Main header file for format.
*/

#ifndef FORMAT_H
#define FORMAT_H 1

/*
/////////////////////////////////////////////////////////////////////////////
//  SPECIAL
/////////////////////////////////////////////////////////////////////////////
*/

#define NAME "Format"
#define VERSION "0.91s" /* the only update 0.91o -> 0.91s in this file... */


#ifdef MAIN
#define EXTERN /**/
#else
#define EXTERN extern
#endif

#include <dos.h>
#include <dpmi.h>
#include <bios.h>
#include <stdio.h>	/* printf */
#include <stdlib.h>	/* exit */
#include <memory.h>	/* memset memcpy */

#include "btstrct.h"	/* BPB and FAT32_BPB etc. */

/*
/////////////////////////////////////////////////////////////////////////////
//  DEFINES
/////////////////////////////////////////////////////////////////////////////
*/


#define MK_FP( seg, ofs )		(  ((unsigned long)(seg)<<4)+(unsigned short)(ofs) )
#define FP_SEG( fp )			(  (unsigned short)((unsigned long)(fp)>>4) )
#define FP_OFF( fp )			(  (unsigned short)(fp)&0x000F )
#define CARRY_FLAG				(  0x01 )

#define dos_memset( lmem, val, size )	do{ unsigned int i, real_val = val; for (i = 0; i < size; i++) dosmemput(&real_val, 1, (unsigned long)lmem+i); }while(0)

#define TRUE	1
#define FALSE	0

#define HARD	0
#define FLOPPY	1

#define FAT12	1
#define FAT16	2
#define FAT32	3

#define MAX_BAD_SECTORS 1024

#define READ       0x25
#define WRITE      0x26

#define UNKNOWN 99

#define BIOS 1
#define DOS  2

/*
/////////////////////////////////////////////////////////////////////////////
//  GLOBAL VARIABLES
/////////////////////////////////////////////////////////////////////////////
*/

EXTERN char ascii_cd_number[16]; /* for 1,234,567 style number display */
EXTERN char partition_label[16]; /* the volume label */

/* Buffers */
EXTERN unsigned long bad_sector_map[(MAX_BAD_SECTORS+1)];
EXTERN unsigned int bad_sector_map_pointer;

/* EXTERN unsigned char fat12_fat[6145]; recordbc.c */

EXTERN __dpmi_regs regs;

/*
/////////////////////////////////////////////////////////////////////////////
//  GLOBAL STRUCTURES
/////////////////////////////////////////////////////////////////////////////
*/

typedef struct Access_Flags
{
	unsigned char special_function;      /* Set to 0.                        */
	unsigned char disk_access;           /* Set to non-zero to allow access. */
} AF;

typedef struct Drive_Statistics
{
	unsigned long sect_total_disk_space; /* all use sect, not bytes: 0.91k */
	unsigned long bad_sectors;
	unsigned long sect_available_on_disk;

	unsigned long sect_in_each_allocation_unit;
	unsigned long allocation_units_available_on_disk;
	unsigned long allocation_units_with_bad_sectors; /* 0.91c */
	unsigned long bytes_per_sector; /* 0.91k */

	unsigned short serial_number_low;
	unsigned short serial_number_high;
} DS;

typedef struct Formatting_Parameters
{
	char drive_letter[2];
	char volume_label[12];

	char existing_format;           /* TRUE or FALSE */

	short drive_type;                 /* FLOPPY or HARD */
	short drive_number;
	short fat_type;
	short media_type;                 /* FD#### or HD */

	short force_yes;
	short verify;
	/* unsigned long first_data_sector; - removed 0.91i, was inconsistent */

	short v;
	short q;
	short u;
	short f;
	short b;
	short s;
	short t;
	short n;
	short one;
	short four;
	short eight;

	unsigned long size;
	unsigned long cylinders;
	unsigned long sectors;
	unsigned long sides; /* for /1 processing -ea */

} FP;

typedef struct DDPT_Structure /* see floppy.c for understanding this */
{
	unsigned short step_rate : 4 ;       /* 0, HI4 */
	unsigned short head_unload_time : 4; /* 0, LO4 */

	unsigned short head_load_time : 7;   /* 1, HI7 */
	unsigned short dma_flag : 1;         /* 1, LO1 */

	unsigned short post_rt_of_disk_motor : 8; /* 2 */
	unsigned short sector_size : 8;           /* 3 */
	unsigned short sectors_per_cylinder : 8;  /* 4 */
	unsigned short gap3_length_rw : 8;   /* !!!  5 */
	unsigned short dtl : 8;              /* !!!  6 */
	unsigned short gap3_length_xmat : 8; /* !!!  7 */
	unsigned short fill_char_xmat : 8;        /* 8 */
	unsigned short head_settle_time : 8;      /* 9 */
	unsigned short run_up_time : 8;           /* a */
} DDPT;

#define FBYTE unsigned char
#define FWORD unsigned short

typedef struct Track_Address_Field_Structure /* 4 bytes */
{
	FBYTE cylinder;
	FBYTE head;
	FBYTE sector;
	FBYTE size_code; /* 2 for 512 bytes */
} TAF;

typedef struct Low_Level_Format_Values_Structure
{
	short interleave_factor;
	short interleave_index;
	short interleave_map[64];
} LLFVS;

typedef struct Parameter_Block_Structure
{
	FBYTE query_flags;
	FBYTE device_type;
	FWORD device_attributes;
	FWORD number_of_cylinders;
	FBYTE media_type;

	/* BPB Follows */
	STD_BPB  bpb;	/* see btstrct.h */
	/* first is bytes_per_sector
	 * total_sectors is 0 if > 32 MB
	 * sectors_per_fat is 0 for FAT32
	 * DOS 3+ uses hidden_sectors_high
	 * large_sector_count_{low,high} used in DOS 4+ if > 32 MB
	 *
	 * DOS 4 actually uses a word at 1f for number of cylinders
	 * and a byte after that for the type, then a word for flags
	 * DOS 5+ no longer has those fields, though
	 */

	/* FAT32 BPB Extensions Follow */
	FAT32_BPB xbpb; /* see btstrct.h */
	/* starting at offset 19h... FAT size is max 16 MBy, but using the
	 * sectors_per_fat_{low,high} is used to distinguish FAT16 <-> FAT32.
	 * word ext_flags at 1dh: bit 7 donotmirrorfats 3..0 activefat
	 * file_sys_info_sec and backup_boot_sec_num can be -1
	 * Ending of parameter block */

	FBYTE reserved_2[32];  /* 0x3c: reserved for int 21.440d.4860 */

	/* int 21.440d.??40 (set device parameters) has a table after BPB or in  */
	/* FAT32 case after reserved_2: word sectors per track (max 63), and for */
	/* each sector a word "number" and a word "size" to define track layout. */
} PB;


EXTERN AF access_flags; /* removed far keyword -ea */
EXTERN DS drive_statistics;
EXTERN FP param;
EXTERN PB parameter_block;
EXTERN unsigned long ddpt;
EXTERN LLFVS low_level;

#endif
