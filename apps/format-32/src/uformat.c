/*
// Program:  Format
//
// Modified By: Hanzac Chen, 2005
// For being compiled by DJGPP, compatible and portable
//
// Version:  0.91r
// (0.90b/c/d fixing compiler warnings - Eric Auer 2003)
// (0.91b..i all kinds of fixes and clean-ups - Eric Auer 2003)
// (0.91k... Eric Auer 2004 (no updates in o/p/q))
// Written By:  Brian E. Reifsnyder
// Copyright:  2002-2004 under the terms of the GNU GPL, Version 2
// Module Name:  uformat.c
// Module Description:  Unconditional Format Functions
*/

#define UFORMAT

#include <conio.h> /* kbhit, getch, which use int 21.7 and 21.b or similar */
#include <string.h>

#include "format.h"
#include "uformat.h"
#include "floppy.h"
#include "driveio.h"
#include "userint.h"

void Unconditional_Floppy_Format(void);
void Unconditional_Hard_Disk_Format(void);
void Compute_Sector_Skew(void);

/* Unconditionally Format the Drive */
void Unconditional_Format()
{
	bad_sector_map_pointer=0;

	if (param.drive_type==FLOPPY) {
		Unconditional_Floppy_Format();
	} else {
		Unconditional_Hard_Disk_Format();
	}
}

void Unconditional_Floppy_Format()
{
	int index = 0;
	int buggy = 0;
	unsigned long percentage;
	int drive_number;

	int cylinders = parameter_block.bpb.total_sectors;
	cylinders /= parameter_block.bpb.number_of_heads;
	cylinders /= parameter_block.bpb.sectors_per_cylinder;

	/* Reset the floppy disk controller */
	drive_number = param.drive_number;
	regs.h.ah = 0x00;
	regs.h.dl = drive_number;
	__dpmi_int(0x13, &regs); /* int 13.0 - rereads DDPT, resets controller */

	/* Set huge_sector_buffer to all 0xf6's. */
	dos_memset(huge_sector_buffer, 0xf6, 512*HUGE_SECTOR_BUFFER_SNUM);
	dos_memset(huge_sector_buffer, 0xf6, 512*HUGE_SECTOR_BUFFER_SNUM);
	/* both! 0.91d */
	do
	{
		/* is it correct that ONE SIDED means "head 0 only" ? */
		buggy += Format_Floppy_Cylinder(index, 0);
		if ( (parameter_block.bpb.number_of_heads==2) &&
		        ((buggy==0) || (index!=0)) ) /* bugs only allowed in tracks > 0 */
			buggy += Format_Floppy_Cylinder(index, 1);

		if ((index==0) && (buggy>0)) /* -ea */
		{
			printf("Format error in track 0, giving up.\n");
			Exit(4,35);
		}

		percentage = (100*index) / cylinders;

		Display_Percentage_Formatted(percentage);

		Compute_Sector_Skew();

		index++;
	} while (index < cylinders);
	/* must be < not <= (formatted one track too much in 0.91 ! */

	Display_Percentage_Formatted(100);

	if (buggy>0)
		printf("\nFound %d bad sectors during formatting.\n", buggy);
	else
		putchar('\n');
}


/* Changed 0.91k: processing I/O errors properly in surface scan */
void Unconditional_Hard_Disk_Format()
{
	/* int error_code; */
	int number_of_sectors;
	char correct_sector[512]; /* new in 0.91g, faster */

	unsigned long percentage_old = 999;

	unsigned index = 0;

	unsigned long last_bad_sector;
	unsigned long sector_number;
	unsigned long percentage;

	unsigned long max_logical_sector = parameter_block.bpb.total_sectors; /* assume 16bit first */

	unsigned long badsec1 = 0; /* -ea */
	unsigned long badsec2 = 0; /* -ea */

	if (max_logical_sector == 0) { /* if 16bit value is 0, use 32bit value */
		/* typo fixed in here 0.91g+ ... had superfluous IF here ... */
		max_logical_sector = parameter_block.bpb.large_sector_count_high;
		max_logical_sector <<= 16;
		max_logical_sector |= parameter_block.bpb.large_sector_count_low;
	}

	number_of_sectors = HUGE_SECTOR_BUFFER_SNUM; /* -ea */
	if (number_of_sectors < 1) {
		printf("huge_sector_buffer not available???\n");
		number_of_sectors = 1; /* use small sector_buffer */
	}

	bad_sector_map_pointer = 0;
	last_bad_sector = 0xffffffffUL;
	sector_number = 1; /* start after boot sector */

	for (index = 0; index < MAX_BAD_SECTORS; index++)
		bad_sector_map[index] = 0;

	memset(&correct_sector[0], 0xf6, 512); /* a nice and shiny empty sector */

	/* Clear and check for bad sectors (maximum of 1 buffer full at a time) */
	printf(" Zapping / checking %lu sectors\n", max_logical_sector); /* -ea */
	if (max_logical_sector > 0x11000UL) /* 0.91g+: 34 MB. Enough for all FATs. */
	{
		printf("To skip the rest of the surface scan after the first 34 MB\n");
		printf("AT OWN RISK, press ESC (only checked at 'percent boundaries').\n");
	}

	do
	{
		int surface_error;

		surface_error = 0;

		/* clear small buffer (used if number_of_sectors == 1) */
		dosmemput(correct_sector, 512, sector_buffer);

		/* also clear big buffer (used in all other cases) */
		for (index=0; index < number_of_sectors; index++) {
			dosmemput(correct_sector, 512, huge_sector_buffer+(index<<9));
		}

		if ((sector_number + number_of_sectors) > max_logical_sector) {
			int howmany; /* clip last chunk -ea */
			howmany = (int)(max_logical_sector - sector_number);
			if ( howmany > number_of_sectors ) howmany = number_of_sectors;
			while ( (number_of_sectors > 1) && (howmany < 2) )
			{ /* make sure that huge_sector_buffer is used!  */
				sector_number--; /* the trick is to overlap... */
				howmany++;       /* ...with the previous chunk */
			}
			surface_error |= Drive_IO(WRITE, sector_number, -howmany);
			surface_error |= Drive_IO(READ,  sector_number, -howmany);
		} else {
			surface_error |= Drive_IO(WRITE, sector_number, -number_of_sectors);
			surface_error |= Drive_IO(READ,  sector_number, -number_of_sectors);
		}

		/* Check for bad sectors by comparing the results of the sector read. */
		/* Changed in 0.91g to use memcmp, not checking every byte manually.  */
		index = 0;
		do
		{
			int cmpresult = 0;
			if (surface_error == 0) {
				/* Copy from dos memory and then compare */
				unsigned char near_sector_buffer[512];
				if (number_of_sectors == 1) {
					dosmemget(sector_buffer, 512, near_sector_buffer);
					cmpresult = memcmp(near_sector_buffer, correct_sector, 512);
				} else {
					dosmemget(huge_sector_buffer+(index<<9), 512, near_sector_buffer);
					cmpresult = memcmp(near_sector_buffer, correct_sector, 512);
				}
			} /* else: I/O error reported, assume ALL sectors were wrong. */
			if ( cmpresult || surface_error )
			{

				if ( ( sector_number + index ) < 5 ) /* new 0.91d */
				{
					printf("One of the first 5 sectors is broken. FORMAT not possible.\n");
					Exit(4,36);
				}

				if ( last_bad_sector != ( sector_number + index ) )
				{
					bad_sector_map[bad_sector_map_pointer]
					= sector_number + index;

					bad_sector_map_pointer++; /* fixed in 0.91c */
					if (bad_sector_map_pointer >= MAX_BAD_SECTORS) /* new in 0.91c */
					{
						printf("Too many bad sectors! FAT bad sector map will be incomplete!\n");
						bad_sector_map_pointer--;
					} /* too many bad sectors */

					badsec1++; /* -ea */
				} /* new bad sector */

			} /* any bad sector */

			index++;
		} while (index < number_of_sectors ); /* one buffer full */
		/* size of buffer was assumed to be 32*512 (wrong!) 0.91d */

		percentage = (max_logical_sector < 65536UL)      ?
		             ( (100 * sector_number) / max_logical_sector ) :
		             ( sector_number / (max_logical_sector/100UL) ) ;
		/* improved in 0.91g+ */

		if (percentage!=percentage_old)
		{
			percentage_old = percentage;
			Display_Percentage_Formatted(percentage);
			if (badsec1 != 0)
			{ /* added better output -ea */
				printf("\n [errors found]\n");
				badsec2 += badsec1;
				badsec1 = 0;
			}
			if ( (sector_number > 0x11000UL) && (percentage != 0) &&
			        kbhit() && (getch() == 27) ) /* 0.91g+ */
			{
				printf("\nESC pressed - skipping surface scan!\n");
				sector_number = max_logical_sector;
			}
		}

		sector_number += number_of_sectors; /* 32; fixed in 0.91g+ */

	} while (sector_number<max_logical_sector);

	Display_Percentage_Formatted(100);

	badsec2 += badsec1;
	if (badsec2>0) {
		printf("\n %lu errors found.\n", badsec2);
	} else {
		printf("\n No errors found.\n");
	}

}
