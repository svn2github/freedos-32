/*
// Program:  Format
//
// Modified By: Hanzac Chen, 2005
// For being compiled by DJGPP, compatible and portable
//
// Version:  0.91s
// (0.91b/c/d - DMA in track_address_fields fix - Eric Auer May 2003)
// (0.91e/f - more formats, better media type setting - Eric Auer)
// (0.91g/h/i - using BPB in more consistent way - Eric Auer Dec 2003)
// (0.91k ... - Eric Auer 2004)
// Written By:  Brian E. Reifsnyder
// Copyright:  2002-2004 under the terms of the GNU GPL, Version 2
// Module:  floppy.c
// Description:  Floppy disk specific functions.
*/

#define FLOP

#include <io.h>	/* write ... for user interface: stderr usage 0.91n */
#include <stdlib.h>
#include <string.h> /* strlen ... for user interface ... 0.91n */
#include <unistd.h>
#include <sys/farptr.h>

#include "floppy.h"
#include "format.h"
#include "btstrct.h"
#include "userint.h" /* Critical_* */
#include "driveio.h" /* huge_sector_buffer* */

/* This backup is needed because printf(), if redirected to A:file.txt */
/* can load the DDPT for A: while we are trying to format B:... Oops!  */
/* Only DDPT CONTENTS, not LOCATION of DDPT should change, though.     */
static DDPT Sddpt;		/* local backup copy of ddpt - added 0.91s -ea */

void Compute_Interleave_Factor(void);
void SaveDDPT(void);
void RestoreDDPT(void);


void SaveDDPT()		/* copy ddpt -> Sddpt */
{
	dosmemget(ddpt, sizeof(DDPT), &Sddpt);
	/* Adjusted for protected mode: not using movedata here because pointers are no longer far */
} /* SaveDDPT */


void RestoreDDPT()	/* copy Sddpt -> ddpt */
{
	dosmemput(&Sddpt, sizeof(DDPT), ddpt);
	/* Adjusted for protected mode: not using movedata here because pointers are no longer far */
} /* RestoreDDPT */


void Compute_Interleave_Factor()
{
	int index;
	int starting_sector;


	low_level.interleave_factor =
	    (param.media_type < HD) ? 1 : 3; /* normal: 1 extra large: 3 */
	if (param.media_type == (HD+1))
		low_level.interleave_factor = 2; /* DMF format has special interleave */

	index = 1;
	starting_sector = 0;
	low_level.interleave_index = 0;
	do
	{
		low_level.interleave_map[low_level.interleave_index] = index;
		low_level.interleave_index
		=low_level.interleave_index+low_level.interleave_factor;
		if ( low_level.interleave_index >=
		        parameter_block.bpb.sectors_per_cylinder )
		{
			starting_sector = starting_sector + 1;
			low_level.interleave_index = starting_sector;
		}

		index++;
	} while (index <= parameter_block.bpb.sectors_per_cylinder);

	#ifdef __FORMAT_DEBUG__
	printf("\n[DEBUG]  Initial Interleave Map:  \n          ");

	index = 0;
	do
	{
		printf("%2d ",low_level.interleave_map[index]);
		index++;
	} while (index <= (parameter_block.bpb.sectors_per_cylinder-1) );
	puts("");
	#endif
} /* Compute_Interleave_Factor */


void Compute_Sector_Skew()
{
	int carry;
	int index;
	int skew_counter;

	if (param.media_type < HD) /* standard format? then no skew! */
		return;

	skew_counter = 0;

	do
	{
		carry =
		    low_level.interleave_map[parameter_block.bpb.sectors_per_cylinder - 1];

		index = parameter_block.bpb.sectors_per_cylinder - 1;

		do
		{
			low_level.interleave_map[index] = low_level.interleave_map[index-1];

			index--;
		} while (index>0);

		low_level.interleave_map[0] = carry;

		skew_counter++;
	} while (skew_counter < 3); /* skew is always 3 for extra large formats */

	#ifdef __FORMAT_DEBUG__
		printf("\n[DEBUG]  Interleave Map After Sector Skew:  \n          ");

		index = 0;
		do
		{
			printf("%d ",low_level.interleave_map[index]);
			index++;
		} while (index <= (parameter_block.bpb.sectors_per_cylinder-1) );
		putchar('\n');
	#endif
} /* Compute_Sector_Skew */


/* this saves stack space and avoids other hassles, but you may not */
/* use DriveIO of single sectors unless you accept buffer overwrite */
/* now returns the bad-sector-count -ea */
int Format_Floppy_Cylinder(int cylinder,int head)
{
	int drive_number;
	int sector_index;
	int retry_count;
	int index;
	int secPerTrack;
	int badSecCount;

	unsigned int result;

	/* Set Up Track Address Fields */
	TAF track_address_fields;
	/* use this buffer to save stack -ea */
	/* new 0.91s: huge_sector_buffer, not sector_buffer, because  *** */
	/* some stupid BIOSes think that TAF would need 512*bytes DMA *** */
	/* (found this in an ancient Mark Zbikowski newsgroup posting) */

	/* DMA boundary avoided -ea */

	badSecCount = 0;
	secPerTrack = parameter_block.bpb.sectors_per_cylinder;

	if ( (cylinder == 0) && (head == 0) )	/* moved to here from uformat.c 0.91s */
	{
		/* Reset the floppy disk controller */
		drive_number = param.drive_number;
		regs.h.ah = 0x00;
		regs.h.dl = drive_number;
		RestoreDDPT();
		__dpmi_int(0x13, &regs); /* int 13.0 - rereads DDPT, resets controller */
	}

	index = 0;
	do
	{
		track_address_fields.cylinder = cylinder;
		track_address_fields.head = head;
		track_address_fields.sector = low_level.interleave_map[index];
		track_address_fields.size_code = 0x02; /* 128<<2: 512by */
		dosmemput(&track_address_fields, sizeof(TAF), huge_sector_buffer+sizeof(TAF)*index);
		index++;
	} while (index<secPerTrack);

	drive_number = param.drive_number;

	#ifdef __FORMAT_DEBUG__
	printf("[DEBUG]  Formatting: Cylinder: %2d  Head: %2d  Sectors: %2d\n",
		cylinder, head, secPerTrack);
	#endif

	/* Format the Track */
	result = 0;
	regs.h.ah = 0x05;
	regs.h.al = secPerTrack; /* feels better -ea */
	regs.h.ch = cylinder;
	regs.h.dh = head;
	regs.h.dl = drive_number;
	regs.x.es = FP_SEG(huge_sector_buffer);
	regs.x.bx = FP_OFF(huge_sector_buffer); /* Use linear address because the DS is zero */
	regs.x.flags &= ~CARRY_FLAG;
	RestoreDDPT();
	__dpmi_int(0x13, &regs);
	result = (regs.x.flags&CARRY_FLAG) ? (regs.h.ah) : 0;	/* flags check added 0.91s */

	if (result!=0) {
		printf("Format_Floppy_Cylinder( head=%d cylinder=%d ) sectors=%d [int 13.5]\n",
			head, cylinder, secPerTrack );
		Critical_Error_Handler(BIOS, result); /* this would abort, no? */
	}


	/* result2 = result; */

	if (param.verify==TRUE)
	{
		/* Verify the Track */
		/* According to RBIL, this uses cached CRC values, but it  */
		/* might use the ES:BX data buffer on 1985/before BIOSes!? */
		/* -> SHOULD we use READ to huge_sector_buffer instead???  */
		/* Warning: huge_sector_buffer is only 18 sectors for now. */
		result=0;
		regs.h.ah = 0x02; /* changed from VERIFY to READ */
		regs.h.al = secPerTrack;
		if ( regs.h.al > HUGE_SECTOR_BUFFER_SNUM )
		{
			regs.h.al = HUGE_SECTOR_BUFFER_SNUM;
			printf("Only checking first %d sectors per track\n", regs.h.al);
			/* could read 2 half-tracks or use pinpointing below here     */
			/* instead... however, only 2.88MB disks have > 18 sect/track */
		}
		regs.h.ch = cylinder;
		regs.h.cl = 1;                     /* Start with the first sector. */
		regs.h.dh = head;
		regs.h.dl = drive_number;
		regs.x.es = FP_SEG(huge_sector_buffer);
		regs.x.bx = FP_OFF(huge_sector_buffer); /* if using READ */
		regs.x.flags &= ~CARRY_FLAG;
		RestoreDDPT();
		__dpmi_int(0x13, &regs);
		result = (regs.x.flags&CARRY_FLAG) ? (regs.h.ah) : 0;	/* flags check added 0.91s */

		#ifdef __FORMAT_DEBUG__
		printf("[DEBUG]  Intr 0x13.2 verify error: %X. Checking details...\n", result);
		#endif

		if (result!=0)
		{

			retry_count  = 0;
			sector_index = 1;

			do
			{
retry:
				/* Verify the Track...sector by sector. */
				/* According to RBIL, this uses cached CRC values, but it  */
				/* might use the ES:BX data buffer on 1985/before BIOSes!? */
				/* -> We use READ to sector_buffer instead. More "normal". */
				result = 0;
				regs.h.ah = 0x02; /* changed from 4 VERIFY to 2 READ */
				regs.h.al = 1;
				regs.h.ch = cylinder;
				regs.h.cl = sector_index;
				regs.h.dh = head;
				regs.h.dl = drive_number;
				regs.x.es = FP_SEG(huge_sector_buffer);
				regs.x.bx = FP_OFF(huge_sector_buffer); /* if using READ */
				regs.x.flags &= ~CARRY_FLAG;
				RestoreDDPT();
				__dpmi_int(0x13, &regs);
				result = (regs.x.flags&CARRY_FLAG) ? (regs.h.ah) : 0; /* flags check added 0.91s */

				if (result!=0)
				{

					retry_count++;

					if (retry_count>=3)
					{
						/* Record this sector as bad. */
						bad_sector_map[bad_sector_map_pointer] =
						    ( (cylinder * (parameter_block.bpb.number_of_heads) )
						      * parameter_block.bpb.sectors_per_cylinder)
						    + (head * parameter_block.bpb.sectors_per_cylinder)
						    + sector_index;

						bad_sector_map_pointer++;
						drive_statistics.bad_sectors++;

						if ((cylinder == 0) && (head == 0) && (sector_index <= 7))
						{
							/* 7: boot sect + 2 fat sect + 4 root dir sect on 160k disk */
							printf("\nFormat failed, error in first sectors!\n");
							return secPerTrack; /* pretend whole track bad - 0.91n */
						}

						printf("Sector %4ld CHS=[%2d:%d:%2d] bad\n",
							bad_sector_map[(bad_sector_map_pointer-1)], cylinder, head, sector_index);

						retry_count = 0;
						badSecCount++;
					}
					else
					{

						regs.x.ax = 0;
						regs.h.dl = drive_number;
						RestoreDDPT();
						__dpmi_int(0x13, &regs); /* reset drive and retry */

						goto retry;   /* Yes, it's a goto.  :-)  Even goto has uses. */

					}
				}

				sector_index++;
			} while (sector_index<=parameter_block.bpb.sectors_per_cylinder);

		}
	}

	RestoreDDPT();
	return badSecCount;

} /* Format_Floppy_Cylinder */


/* void ddptPrinter(void); */
void ddptPrinter(void)	/* Dump DDPT (11 bytes, sometimes split into bits) */
{
	/* PROBLEM: if output is redirected to a file on another floppy drive, */
	/* then printing the DDPT will replace the DDPT by the one used for    */
	/* that other floppy drive. Funny problem. Workaround added 0.91s. -ea */
	if ((Sddpt.sector_size != 2) /* || (Sddpt.fill_char_xmat != 0xf6) */)
	{	/* in DOSEMU, fill char can actually be 0 */
		printf("[DEBUG]  [DDPT] Sector size: code 0x0%x  Fill char: 0x0%x\n",
		       Sddpt.sector_size, Sddpt.fill_char_xmat);
		return;	/* no use printing broken DDPTs in detail */
	}

	printf("[DEBUG]  [DDPT] Step Rate: %3d msec  Head Unload: %5d msec  DMA Flag Bit: %2d\n",
	       /* note that the default step rate 15*16ms is pretty long */
	       Sddpt.step_rate * 16,			/* 1st byte, LO4 [3 digits] */
	       /* this defaults to pretty long as well, 64 msec in DOSEmu */
	       (16 - Sddpt.head_unload_time) << 4,	/* 1st byte, HI4 [3 digits] */
	       /* default is 0 - always 0? */
	       Sddpt.dma_flag);				/* 2nd byte, LO1 */
	/* (head unload shown as %5d for nicer looks above "post rotate"...) */

	printf("[DEBUG]  [DDPT] Head Load: %3d msec  Post Rotate: %5d msec  Sector Size: %d\n",
	       /* default unknown, e.g. 60 msec */
	       Sddpt.head_load_time << 1,		/* 2nd byte, HI7 [3 digits]*/
	       /* default is 2.035 sec, time during which motor stays spinning after access */
	       Sddpt.post_rt_of_disk_motor * 55,	/* unit is timer ticks [5 digits] */
	       /* default is 512 */
	       (Sddpt.sector_size < 5) ? (128 << Sddpt.sector_size)	/* 128..2048 */
	       : -Sddpt.sector_size);			/* print nonsense as negative */
	/* (floppy can have 128..1024 only) */

	printf("[DEBUG]  [DDPT] SECTORS PER TRACK: %3d  (Gap: %3d  Data: %3d  Format Gap: %3d)\n",
	       /* default depends on drive type */
	       Sddpt.sectors_per_cylinder,		/* max useful value 63 */
	       /* better NOT MESS with the following three: */
	       Sddpt.gap3_length_rw,
	       Sddpt.dtl,
	       Sddpt.gap3_length_xmat);

	printf("[DEBUG]  [DDPT] Fill Char: 0x%2.2x  Head Settle: %d msec  Motor Start: %d msec\n",
	       /* default 246, in hex: 0xf6 */
	       Sddpt.fill_char_xmat,
	       /* default 25ms [max 3 digits] (MS tells: full-height 5.25in were faster) */
	       Sddpt.head_settle_time,
	       /* default BIOS 0.5 sec, DOS 0.25 sec [max 5 digits] */
	       Sddpt.run_up_time * 125);
} /* ddptPrinter */


void Set_Floppy_Media_Type()
{
	int drive_number = param.drive_number;
	int number_of_cylinders;
	int sectors_per_cylinder;
	int drive_type = 0;


	param.media_type = UNKNOWN;

	if ((drive_number & 0x80) != 0)
	{
		printf("Drive number > 127: This is no floppy! Aborting.\n");
		Exit(4,40);
	}

	regs.x.ax = 0;
	regs.h.dl = drive_number;
	__dpmi_int(0x13, &regs); /* reset drive */
	SaveDDPT();

	#ifdef __FORMAT_DEBUG__
	printf("[DEBUG]  Current Disk Drive Parameter Table Values at %4.4x:%4.4x:\n",
		FP_SEG(&ddpt), FP_OFF(&ddpt));
	ddptPrinter();	/* original ddpt, from init.c Setup_DDPT (int 1e) */
	#endif

	param.fat_type = FAT12; /* Set_Floppy_Media_Type() is always FAT12 */

	if (param.one==TRUE)
	{
		/* *** (this is only for 160k and 180k formats) *** */
		param.cylinders = 40;
		if ((param.sectors!=9) && (param.sectors!=8))
		{
			param.sectors = 9;
		}

		param.sides = 1;
		param.t = TRUE; /* trigger media type search */
	}
	else
	{
		param.sides = 2; /* the default (initialize value!) */
	}

	if (param.four==TRUE)
	{
		/* (also need special SETUP for 360k disk in 1200k drive) */
		param.sectors = 9;
		param.cylinders = 40;
		param.t = TRUE; /* trigger media type search */
	}

	if (param.eight==TRUE)
	{
		/* (this is only for 120k and 320k DOS 1.0 formats) */
		param.sectors = 8;
		param.cylinders = 40;
		param.t = TRUE; /* trigger media type search */
	}

	if (  (param.t==TRUE) /* user wanted a certain number of cylinders (tracks) */
	        || (param.f==TRUE) /* user wanted a certain size */
	   )
	{
		int index = 0; /* find matching media type from list */
		do
		{
			int cyls = drive_specs[index].total_sectors;
			cyls /= drive_specs[index].number_of_heads;
			cyls /= drive_specs[index].sectors_per_cylinder;

			if ( (  (param.f==TRUE)
			        && (param.size == (drive_specs[index].total_sectors >> 1))
			     ) ||
			        (  (param.t==TRUE)
			           && (param.cylinders == cyls)
			           && (param.sectors   == drive_specs[index].sectors_per_cylinder)
			           && (param.sides     == drive_specs[index].number_of_heads)
			        )
			   )
			{
				param.media_type = index;

				/* size is always found in the drive_specs, even if already given */
				param.size = drive_specs[index].total_sectors >> 1;

				if (/* *** param.f== *** */ TRUE) /* size given, geometry wanted */
				{
					param.cylinders = cyls;
					param.sectors   = drive_specs[index].sectors_per_cylinder;
					param.sides     = drive_specs[index].number_of_heads;
				}

				printf("Formatting to %ldk (Cyl=%ld Head=%ld Sec=%2ld)\n",
				       param.size, param.cylinders, param.sides, param.sectors);
				index = -10; /* break out of the loop */
			} /* end "if match" */
			else
			{
				index++;
				while (drive_specs[index].number_of_heads==0)
					index++; /* skip placeholder entries */
			} /* search on if no match */

		} while ( (index>=0) && (drive_specs[index].bytes_per_sector==512) );

		if (index>0)
		{
			if (param.f==TRUE) /* only size given */
			{
				printf("No media type found for %ldk format\n", param.size);
			}
			else /* geometry given */
			{
				param.size = (param.cylinders * param.sides * param.sectors) >> 1;
				printf("No media type found for %ldk format (Cyl=%ld Head=%ld Sec=%2ld)\n",
				       param.size, param.cylinders, param.sides, param.sectors);
			}
			Exit(4,41); /* cannot continue anyway, media type needed later! */
		}
	}

	drive_number = param.drive_number;

	/* IF the user gave ANY size indication, we already know which */
	/* media_type the user wants, so we check drive capabilities now! */


	if (TRUE) /* just limiting variable scope */
	{
		int drive_number  = param.drive_number;
		int preferredsize = 0;

		regs.h.ah = 0x08;
		regs.h.dl = drive_number;
		regs.x.di = 0;
		regs.x.flags &= ~CARRY_FLAG;
		RestoreDDPT();
		__dpmi_int(0x13, &regs); /* FUNC 8 - GET DRIVE TYPE */
		if ((regs.x.flags&CARRY_FLAG) && (regs.h.ah != 0))
			Exit(4,53);	/* should never happen... check added 0.91s */

		drive_type = regs.h.bl;
		/* we ignore the returned DDPT pointer ES:DI for now */
		preferredsize = 0; /* drive not installed */

		switch (drive_type)
		{
		case 1: preferredsize = 360;
			break;
		case 2: preferredsize = 1200;
			break;
		case 3: preferredsize = 720;
			break;
		case 4: preferredsize = 1440;
			break;
		case 5:
		case 6: preferredsize = 2880;
			break;
			/* Type 5 is originally for floppy tape drives */
			/* int 21.440d.860 would use: 0 360k, 1 1200k, 2 720k, 7 1440k, 5 harddisk */
		} /* switch */

		if (param.media_type==UNKNOWN) /* do auto-detection of size! */
		{
			int index = 0; /* find matching media type from list */
			param.size = preferredsize;
			do
			{
				int cyls = drive_specs[index].total_sectors;
				cyls /= drive_specs[index].number_of_heads;
				cyls /= drive_specs[index].sectors_per_cylinder;

				if (param.size == (drive_specs[index].total_sectors >> 1))
				{
					param.media_type = index;
					param.cylinders  = cyls;
					param.sectors    = drive_specs[index].sectors_per_cylinder;
					param.sides      = drive_specs[index].number_of_heads;

					printf("Using drive default: %ldk (Cyl=%ld Head=%ld Sec=%2ld)\n",
					       param.size, param.cylinders, param.sides, param.sectors);
					index = -10;
				}
				else
				{
					index++;
					while (drive_specs[index].number_of_heads==0)
						index++; /* skip placeholder entries */
				}

			} while ( (index>=0) && (drive_specs[index].bytes_per_sector==512) );

			if (index >= 0)
			{
				printf("Size %ldk undefined!??\n", param.size);
				Exit(4,42);	/* should not happen: no default geometry known */
			}
		}
		else
		{
			if (param.size > preferredsize) /* shorter messages 0.91q */
			{
				if (param.size > (preferredsize + (preferredsize >> 2)))
				{
					printf("Your want %ldk but drive standard is %dk. Too much. Aborting.\n",
					       param.size, preferredsize);
					Exit(4,43);
				}
				printf("OVERFORMAT: %ldk (drive standard is %dk). Good luck!\n",
				       param.size, preferredsize);
			}
		}
	} /* variable scope end */

	/* Copy BPB from list into our parameter_block BPB structure NOW. - 0.91i */
	/* *** Our global BPB in parameter_block is NOT VALID before this point! *** */
	memcpy(&parameter_block.bpb.bytes_per_sector /* target */,
	       &drive_specs[param.media_type].bytes_per_sector /* source */,
	       sizeof(STD_BPB)); /* was 37 bytes, why? Should be 36! */

	number_of_cylinders  = parameter_block.bpb.total_sectors;
	number_of_cylinders /= parameter_block.bpb.number_of_heads;
	number_of_cylinders /= parameter_block.bpb.sectors_per_cylinder;

	sectors_per_cylinder = parameter_block.bpb.sectors_per_cylinder;
	/* cannot configure drive for "number of heads" - only used in formatting */

	/* useful cases: 360k in 1200k, 1200k in 1200k, ... */
	/* formats >= 1440k are only set up by int 13.17, not here. */
	/* we already aborted if the format is ABOVE possible range */

	switch (drive_type)
	{
	case 1: /*  360k (int 21.440d.860 calls this type 0) */
		regs.h.al = 1; /* mode 1: 160, 180, 320, 360, 400k */
		break;
	case 2: /* 1200k (int 21.440d.860 calls this type 1) */
		regs.h.al = (param.size > 400) ? 3 : 2;
		/* mode 2: like 1, but in 1200k drive. mode 3: 1200k */
		/* mode 3 is probably only for 1200k, but we allow smaller */

		/* (bonus: mode 5 for 720k in 1200k drive) */
		/* from Ch. Hochstaetters FDFORMAT/88: mode 5 for 720k in *1200k*! */
		break;
	case 3: /*  720k (int 21.440d.860 calls this type 2) */
		regs.h.al = 4; /* mode 4: 720, 800k */ /* mode FOUR! */
		/* (bonus: mode 2 should allow 360k format in 720k drive) */
		if (param.four || param.one || param.eight)
		{
			printf("This is a 720k drive which does not support /1, /4 or /8\n");
			Exit(4,44);
		}
		if (param.size < 720)
		{
			printf("Minimum size for this drive type is 720k\n");
			Exit(4,45);
		}
		break;
	case 4: /* 1440k (int 21.440d.860 calls this type 7) */
		regs.h.al = 0xff; /* NO SETTING - 0.91M */
		/* standard mode!? (int 13.18 sets details) */
		/* *** 0.91M - no setting needed for 1.44M drives        *** */
		/* *** FreeDOS kernel just tries 13.18 FIRST, then 13.17 *** */
		/* *** if 13.18 gives error 0c (not supported) or        *** */
		/* *** 80 (no disk in drive), 13.17 is not even tried    *** */

		if (param.size < 1200) /* new 0.91c: 720k in 1440k... */
			regs.h.al = 0xff; /* NO SETTING - 0.91M */
		/* according to RBIL, only int 13.18 is relevant for 1.44M drive */
		/* Ch. Hochstaetters FDFORMAT/88 uses mode 5 here, not mode 4 !? */
		/*  However, int 13.17.5 returns error 1, invalid parameter...   */

sizecheck_3inch:
		if (param.four || param.one || param.eight)
		{
			printf("This is a 3.25 inch drive which does not support /1, /4 or /8\n");
			Exit(4,46);
		}
		if ((param.size < 720) || (param.size == 1200))
		{
			printf("Minimum size for this drive type is 720k. 1200k not allowed.\n");
			Exit(4,47);
		}
		break;
	case 5: /* 2880k */
	case 6: /* 2880k */
	default:
		regs.h.al = 0xff; /* no mode setting needed (?) */
		goto sizecheck_3inch;
		/* break; */
	}
	if (regs.h.al != 0xff)
	{
		int sizeclass = regs.h.al;

		#ifdef __FORMAT_DEBUG__
		printf("[DEBUG] Selecting int 13h ah=17h size class: %d\n", sizeclass);
		#endif

		do
		{
			regs.h.ah = 0x17; /* set disk mode, e.g "360k disk in 1200k drive */
			/* regs.x.cx = 0xffff; */ /* unused */
			regs.h.dl = drive_number;
			regs.x.flags &= ~CARRY_FLAG;
			RestoreDDPT();
			__dpmi_int(0x13, &regs); /* FUNC 17 - SET DRIVE MODE */
		} while ((regs.x.flags&CARRY_FLAG) && (regs.h.ah == 6));
		/* retry if error is only "disk changed" */

		if (regs.x.flags&CARRY_FLAG)
		{
			printf("Drive mode (size class %d) setting failed, error %2.2x (hex)\n",
			       sizeclass, regs.h.ah);
			if (regs.h.ah == 0x80)
			{
				printf("No disk in drive!\n");
				Exit(4,48);
			}
			else
				printf("Continuing anyway.\n");
		}
	}

	/* only more modern drives / BIOSes support this at all, probably. */
	/* we may not give non-standard geometries NOW - no BIOS support.  */
	RestoreDDPT();
	regs.x.di = 0xffff;		/* "NO NEW DDPT DATA" */
	if (drive_type < 4)		/* new 0.91s */
		goto skip_int13_18;	/* *** SKIP INT 13.18 for 5.25" and DD *** */


	/* --- hide oversized formats --- */
	regs.h.ah = 0x18; /* set geometry, apart from head count of course */
	regs.h.ch = (number_of_cylinders > 43) ? 79 : 39;

	regs.h.cl = sectors_per_cylinder;
	if (regs.h.cl >= 36)
	{
		regs.h.cl = 36; /* 2.88M or oversized -> 2.88M */
	}
	else
	{
		if (regs.h.cl >= 18)
		{
			regs.h.cl = 18; /* 1.44M or oversized -> 1.44M */
		}
		else
		{
			if (regs.h.cl >= 15)
			{
				regs.h.cl = 15; /* 1.2M or oversized -> 1.2M */
			}
			else
			{
				regs.h.cl = (regs.h.cl > 9) ? 9 : (regs.h.cl);
				/* 720k or 360k or oversized -> 720k or 360k */
			} /* end of the */
		} /* if chain which */
	} /* hides oversizes... */
	/* --- /hide oversized formats --- */

	regs.h.dl = drive_number;

	/* pre 0.91n: if ddpt secpercyl == cl AND cl in 9..15 range, skip this */
	/* also skip if drive_type below 4, since 0.91M */
	#ifdef __FORMAT_DEBUG__
	printf("[DEBUG]  Setting geometry with int 13h ah=18h: tracks=%d sectors=%d\n",
		regs.h.ch+1, sectors_per_cylinder);
	#endif

	RestoreDDPT();
	__dpmi_int(0x13, &regs); /* FUNC 18 (AT): SET GEOMETRY */
	/* 40x[1,2]x8 (?), 80x2x[18,36]: 120, 320, 1440, 2880k */

	/* replaced FLAGS check by AH check in 0.91s: RBIL "uses" no CF for 13.18! */
	/* 00 okay, 01 no function 13.18, 0C invalid geometry, 80 no disk in drive */

	/* pre 0.91s reached this even if int 13.18 skipped */
	if (regs.h.ah != 0) /* any int 13.18 troubles encountered */
	{

		if (regs.h.ah==0x80)
		{
			printf("No disk in drive (timeout)!\n");
			Exit(4,49);
		}

		if (regs.h.ah==1)
		{
			/* BIOS simply does not support 13.18, bad luck */
			#ifdef __FORMAT_DEBUG__
			puts("[DEBUG]  BIOS does not support int 13.18 geometry setting, ignored.");
			#endif
			regs.x.di = 0xffff;	/* "NO NEW DDPT DATA" */
		}
		else	/* ... if AH is 0x0ch, that is */
		{
			printf("Media type %ldk  (%d x %ld x %2d)  not supported by this drive!?\n", param.size, number_of_cylinders, param.sides, sectors_per_cylinder);
			printf("Geometry set (int 13.18) error (%2.2x). ", regs.h.ah);
			if ( (_osmajor < 7) || (sectors_per_cylinder > Sddpt.sectors_per_cylinder) )
			{		/* 0.91q: Win9x DOS box uses *63 geometry all the time */
				printf("Giving up.\n");	/* old DOS - or ddpt geo too small */
				Exit(4,50);
			}
			else		/* ability to ignore added 0.91q */
			{
				printf("Ignored.\n");
				regs.x.di = 0xffff;	/* "NO NEW DDPT DATA" */
			}
		}

	} /* end of int 13.18 trouble processing */

	/* The following part only happened if int 13.18 worked before 0.91q */
	/* (was right: otherwise no new DDPT, and wrong: always update geo!) */

	if (regs.x.di != 0xffff)		/* if new DDPT data... - new 0.91s */
	{
		/* ES:DI points to an array to which the int 1E vectors should point */
		regs.x.cx = regs.x.es; 	/* backup ES, new pointer CX:DI... */

		/* changed in 0.91b: memcpy but do not change pointer unless in BIOS */
		/* 0.91b .. 0.91r fetched int 1e vector again here - not necessary!? */

		if ((FP_SEG(&ddpt) < 0xa000) || ((FP_SEG(&ddpt) & 0x3ff) != 0))
		{
			/* found DDPT to reside in RAM - copying new data there */
			/* RestoreDDPT() no need? */
			dosmemget(MK_FP(regs.x.cx, regs.x.di), sizeof(DDPT), &ddpt);
			/* Adjusted for protected mode: Not using movedata here because pointers are no longer far */
			SaveDDPT();


			#ifdef __FORMAT_DEBUG__
			printf("[DEBUG] Updated INT 1E (DDPT) data at: %4.4x:%4.4x.\n",
				FP_SEG(&ddpt), FP_OFF(&ddpt));
			#endif
			/* actually we did NOT really update sectors_per_cylinder yet :-) */

		} /* DDPT in RAM */
		else
		{
			/*  found DDPT to reside in ROM - moving int 1e vector to new value   */
			/* fix 0.91s: must be CX:DI, int 13.18 returned ES:DI, but CX=ES now! */

			dosmemget(MK_FP(regs.x.cx, regs.x.di), sizeof(DDPT), &ddpt); /* WE should know the update, too */
			regs.h.ah = 0x25;		/* set int vector */
			regs.h.al = 0x1e;		/* ddpt vector */
			regs.x.dx = regs.x.di;	/* offset DX   */
			regs.x.ds = regs.x.cx;	/* segment DS  */
			__dpmi_int(0x21, &regs);

			printf("Using DDPT in ROM - only standard sizes possible.\n");

			#ifdef __FORMAT_DEBUG__
			printf("[DEBUG] New INT 1E (DDPT) vector: %4.4x:%4.4x.\n",
				FP_SEG(&ddpt), FP_OFF(&ddpt));
			#endif

		} /* DDPT in ROM */

	} /* int 13.17/.18 geometry setting did not work, so we changed the DDPT. */
	/* *** changed 0.91q: DDPT is always updated, even if int 13.18 ok. *** */


skip_int13_18:	/* *** end skipable int 13.18 stuff (jump added 0.91s) *** */
	RestoreDDPT();
	Sddpt.sectors_per_cylinder = sectors_per_cylinder;
	RestoreDDPT();
	/* update geometry even if the ddpt pointer or contents did not change. */

	if (Sddpt.sectors_per_cylinder != sectors_per_cylinder) /* DDPT is in ROM */
		printf("SECTORS PER TRACK stuck to %d, wanted %d. Continuing anyway.\n",
			Sddpt.sectors_per_cylinder, sectors_per_cylinder);


	if (param.media_type > HD) /* new in 0.91c, changed in 0.91g */
		/* "over-formats" always have to use interleave, the others not */
		/* Ch. Hochstaetter writes that this is to help gap size!? */
	{
		int gap_len;
		/* TODO: DDPT SEG is regs.x.es? */
		if ( (regs.x.es > 0xa000) && ((regs.x.es & 0x3ff) == 0) )
		{
			printf("DDPT tweaking impossible: DDPT in ROM\n");
			Exit(4,51);
		}

		switch (sectors_per_cylinder)
		{
			/* *** values from Ch. Hochstaetters FDFORMAT/88 1.8 *** */
			/* *** those values are for interleave 2, but we use 3. Problem??? *** */
		case 8:  gap_len = 0x58; break;  /* 160k and 320k */
		case 9:  gap_len = 0;    break;  /* (360k/720k: 0x50 / my BIOS: 0x2a) */
		case 10: gap_len = 0x2e; break;  /* 400k */
		case 15: gap_len = 0;    break;  /* (1200k: 0x54) */
		case 18: gap_len = 0;    break;  /* (1440k: 0x6c / my BIOS: 0x1b) */
		case 19:
		case 20: gap_len = 0x2a; break; /* (unused) */
		case 21: gap_len = 0x0c; break; /* > 1440k. Smaller gap than 30 or 40! */
			/* small gap only works with interleave */
		default: gap_len = 0;
		} /* case */

		if (gap_len == 0)
		{
			printf("No gap length known for %d sec/cyl. Good luck with BIOS value!\n",
				sectors_per_cylinder); /* (we will display BIOS value in DEBUG mode) */
			printf("TWEAK: Sectors per cylinder in DDPT set to %d\n",
				sectors_per_cylinder);
		}
		else
		{
			printf("TWEAK: Setting format gap length %d and %d sectors/cylinder!\n",
				gap_len, sectors_per_cylinder);
			RestoreDDPT();
			Sddpt.gap3_length_xmat = gap_len; /* (gap3_length_rw needs no change) */
			RestoreDDPT();
		}

	} /* TWEAK mode for over sized formats: gap size / sectors per track */


	/* changed 0.91h: total = "diskimage" size, avail = "free in data area" size. */
	drive_statistics.sect_total_disk_space =
	    (unsigned long)parameter_block.bpb.total_sectors;
	drive_statistics.sect_available_on_disk =
	    (unsigned long)parameter_block.bpb.total_sectors;

	drive_statistics.sect_available_on_disk -= parameter_block.bpb.reserved_sectors;
	drive_statistics.sect_available_on_disk -= parameter_block.bpb.number_of_fats
	        * (unsigned long)parameter_block.bpb.sectors_per_fat;
	drive_statistics.sect_available_on_disk -= 32UL
	        * (unsigned long)parameter_block.bpb.root_directory_entries
	        / parameter_block.bpb.bytes_per_sector;

	drive_statistics.sect_in_each_allocation_unit =
	    (unsigned long)parameter_block.bpb.sectors_per_cluster;

	drive_statistics.bad_sectors = 0; /* 0.91c */
	drive_statistics.bytes_per_sector =
	    (unsigned long)parameter_block.bpb.bytes_per_sector;

	Compute_Interleave_Factor();

	#ifdef __FORMAT_DEBUG__
	printf("\n[DEBUG]  Configured Disk Drive Parameter Table Values at %4.4x:%4.4x:\n",
		FP_SEG(&ddpt), FP_OFF(&ddpt));
	ddptPrinter();	/* right before the actual int 13.0 call... */
	#endif

	regs.x.ax = 0; /* reset floppy controller and make it use the current DDPT */
	regs.h.dl = drive_number;
	regs.x.flags &= ~CARRY_FLAG;
	RestoreDDPT();
	__dpmi_int(0x13, &regs); /* reset floppy controller */
	if ((regs.x.flags&CARRY_FLAG) && (regs.h.ah != 0)) {
		regs.x.ax = regs.h.ah;
		printf("Reset of floppy controller failed (code %x) - DDPT rejected?\n",
		       regs.x.ax);
	}

	/* *** Extra: check 0x40:[0x90 + BIOS drive number] value for sanity *** */
	/* *** bits are: BB D K xxxx, where xxxx = 7 "default", 4/5? "1200k" *** */
	/* *** 3 "360k / default 5.25inch", 4 (optional) "360k in 1200k" ... *** */
	/* *** BB = baud rate 00=500k (1.xM), 01=300k (360k), 10=250k (720k) *** */
	/* *** D = double stepping (40 tracks 360k in 1.2M drive)            *** */
	/* *** K = drive already configured for this disk, known media type  *** */
	/* *** E.g.: 0x14 (1.xM), 0x53/0x73 (360k/360k-in-1.2M), 0x97 (720k) *** */
	/* Idea from Ch. Hochstaetters FDFORMAT/88 1.8 (opt. even SETS this...). */
	#ifdef __FORMAT_DEBUG__
	char ch;
	unsigned char control = _farpeekb(0x40, (0x90 + drive_number));
	/* low nibble: usually 7, can be 4/5? 1200k 3 360k/default 5.25inch */
	/*   4 "360k in 1200k" ... can vary a lot. */
	printf("[DEBUG]  Controller setup %2hx: ", control); /* %hx: short hex */
	if (control & 0x20) printf("[doublestepping] ");
	if (control & 0x10) printf("[configured type %hu] ", (control & 0x0f));
	/* only if configured is true int 13.5 will work!? */
	control >>= 6; /* keep only baud rate bits */
	switch (control)
	{
		case 0:  printf(" 500 kbps (HD 1xx0k)\n"); break;
		case 1:  printf(" 300 kbps (DD  360k)\n"); break;
		case 2:  printf(" 250 kbps (DD  720k)\n"); break;
		default: printf("1000 kbps (ED 2880k)\n"); break;
	}

	write(isatty(1) ? 1 : 2, "-- press ENTER to format disk (ESCAPE to abort) --\n", 51);
	/* writes to stderr to stay visible if redirecting */
	do
	{
		regs.h.ah = 7;
		__dpmi_int(0x21, &regs);
		ch = regs.h.al;
	} while ((ch != 13) && (ch != 27));
	/* (avoids important information scrolling away) */
	if (ch != 13)
		Exit(3,52);
	#endif

	RestoreDDPT();
} /* Set_Floppy_Media_Type */
