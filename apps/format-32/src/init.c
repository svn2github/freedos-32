/*
// Program:  Format
//
// Modified By: Hanzac Chen, 2005
// For being compiled by DJGPP, compatible and portable
//
// Version:  0.91k
// (0.90b/c/d - DMA friendly buffer movement - Eric Auer - May 2003)
// (0.91k ... Eric Auer 2004)
// Written By:  Brian E. Reifsnyder
// Copyright:  2002-2004 under the terms of the GNU GPL, Version 2
// Module Name:  init.c
// Module Description:  Program initialization functions.
*/

#include <go32.h>
#include <stdlib.h>

#include "format.h"
#include "driveio.h" /* sector buffers */


void Initialization(void)
{
	int dos_selector = 0;
	int bad_sector_map_pointer = 0;

	/* Using transfer buffer and set as long pointer */
	transfer_buffer = __tb+0x200;
	sector_buffer = __tb+0x400;
	huge_sector_buffer = MK_FP(__dpmi_allocate_dos_memory(512*HUGE_SECTOR_BUFFER_SNUM/16, &dos_selector), 0);
	param.drive_letter[0]=NULL;
	param.volume_label[0]=NULL;

	param.drive_type=0; /* not NULL; */
	param.drive_number=0; /* not NULL; */
	param.fat_type=0; /* not NULL; */
	param.media_type=UNKNOWN;

	param.force_yes=FALSE;
	param.verify=TRUE;

	param.v=FALSE;
	param.q=FALSE;
	param.u=FALSE;
	param.f=FALSE;
	param.b=FALSE;
	param.s=FALSE;
	param.t=FALSE;
	param.n=FALSE;
	param.one=FALSE;
	param.four=FALSE;
	param.eight=FALSE;

	param.size=UNKNOWN;
	param.cylinders=0;
	param.sectors=0;

	drive_statistics.bad_sectors = 0;
	drive_statistics.bytes_per_sector = 512;

	/* Clear bad_sector_map[]. */
	do
	{
		bad_sector_map[bad_sector_map_pointer]=0;
		bad_sector_map_pointer++;
	} while (bad_sector_map_pointer<MAX_BAD_SECTORS);

	/* Initialize segment regs */
	regs.x.ds = regs.x.es = FP_SEG(__tb);

	/* Get the location of the DDPT */
	regs.h.ah =0x35;
	regs.h.al =0x1e;
	__dpmi_int(0x21, &regs);
	ddpt = MK_FP(regs.x.es, regs.x.bx);
#ifdef __FORMAT_DEBUG__
	printf("[DEBUG] SB: %lx HUGE SB: %lx ES: %x BX: %x\n", sector_buffer, huge_sector_buffer, regs.x.es, regs.x.bx);
#endif
}
