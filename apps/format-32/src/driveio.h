/*
// Program:  Format
//
// Modified By: Hanzac Chen, 2005
// For being compiled by DJGPP, compatible and portable
//
// Version:  0.91r
// (0.90b/c - DMA boundary avoidance by Eric Auer - May 2003)
// (0.91r: added Exit() - Eric 2004)
// Written By:  Brian E. Reifsnyder
// Copyright:  2002 under the terms of the GNU GPL, Version 2
*/

#ifndef DRIVEIO_H
#define DRIVEIO_H


#ifdef DIO
	#define DIOEXTERN   /* */
#else
	#define DIOEXTERN   extern
#endif


#define HUGE_SECTOR_BUFFER_SNUM	21


typedef unsigned long ULONG;
typedef unsigned long  __uint32;
typedef unsigned short __uint16;

/* only uformat.c uses huge_sector_buffer at all (for fill with 0xf6), */
/* so huge_sector_buffer does NOT have to be one track big!            */

DIOEXTERN ULONG sector_buffer;		/* Emulated long pointer */
DIOEXTERN ULONG transfer_buffer;	/* Just linear address */
DIOEXTERN ULONG huge_sector_buffer;


/* The following lines were taken from partsize.c, written by Tom Ehlert. */

DIOEXTERN void Clear_Huge_Sector_Buffer(void);
DIOEXTERN void Clear_Sector_Buffer(void);

DIOEXTERN void Exit(int mserror, int fderror);	/* by Eric Auer for 0.91r */
DIOEXTERN void Lock_Unlock_Drive(int lock);	/* by Eric Auer for 0.91l */

DIOEXTERN int Drive_IO(int command,unsigned long sector_number,int number_of_sectors);
DIOEXTERN void Enable_Disk_Access(void);

DIOEXTERN unsigned int FAT32_AbsReadWrite(char DosDrive, int count, ULONG sector, ULONG buffer, unsigned ReadOrWrite);
DIOEXTERN unsigned int TE_AbsReadWrite(char DosDrive, int count, ULONG sector, ULONG buffer, unsigned ReadOrWrite);

#endif
