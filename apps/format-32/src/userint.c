/*
// Program:  Format
//
// Modified By: Hanzac Chen, 2005
// For being compiled by DJGPP, compatible and portable
//
// Version:  0.91r
// (0.90b/c/d - better error messages - Eric Auer - May 2003)
// (0.91b..g - Eric Auer 2003 / 0.91k ... Eric Auer 2004)
// Written By:  Brian E. Reifsnyder
// Copyright:  2002-2004 under the terms of the GNU GPL, Version 2
// Module Name:  userint.c
// Module Description:  User interfacing functions.
*/

/* #include <stdlib.h> */ /* no // comments for Turbo C! -ea */


#define USERINT

#include <io.h>		/* write (to stderr) */
#include <dos.h>
#include <conio.h>
#include <ctype.h>	/* toupper */
#include <string.h>	/* strlen  */
#include <unistd.h>

#include "format.h"
#include "driveio.h"
#include "userint.h"


void ASCII_CD_Number(unsigned long number)
{
	int comma_counter;
	int index;
	int right_index;
	int shift_counter;
	int shift_register;
	int shift_temp_register;
	int start_shift;
	unsigned char dsep, ksep; /* decimal- and kilo-separator */

	/* we assume that this is DOS 2.11+: offset 7 1000s-sep 9 decsep    */
	/* also in this buffer: date format, 12<->24 clock, currency stuff, */
	/* date sep (. or / or -), time sep (:) and lots of other things    */

	ksep = ','; /* or . */
	dsep = '.'; /* or , */

	memset((void *)&ascii_cd_number[0], 0, sizeof(ascii_cd_number));

	itoa(number, ascii_cd_number, 10);

	/* Add Commas */
	index = 13;
	right_index = 13;
	start_shift = FALSE;

	do
	{
		if (ascii_cd_number[index]>0) start_shift = TRUE;

		if (start_shift==TRUE)
		{
			ascii_cd_number[right_index] = ascii_cd_number[index];
			ascii_cd_number[index] = 0;
			right_index--;
		}

		index--;
	} while (index>=0);

	comma_counter = 0;
	index = 13;
	do
	{
		comma_counter++;

		if (ascii_cd_number[index]==0)
		{
			comma_counter = 5;
			ascii_cd_number[index] = ' ';
		}

		if (comma_counter==4)
		{
			shift_counter = index-1;
			shift_register = ascii_cd_number[index];
			ascii_cd_number[index] =  ksep; /* 1000s separator */
			do
			{
				shift_temp_register = ascii_cd_number[shift_counter];
				ascii_cd_number[shift_counter] = shift_register;
				shift_register = shift_temp_register;

				shift_counter--;
			} while (shift_counter>=0);

			comma_counter = 0;
		}

		index--;
	} while (index>=0);

	ascii_cd_number[14] = 0;

	/* cheat: add decimal separator after end of ascii_cd_number string */
	ascii_cd_number[15] =  dsep;

}


void Ask_User_To_Insert_Disk()
{
	printf(" Insert new diskette for drive %c:\n",param.drive_letter[0]);
	write(isatty(1) ? 1 : 2,
	      " Press ENTER when the right disk is in drive...\n", 48);
	/* write to STDERR for the case of STDOUT being redirected */

	/* Wait for a key */

	regs.h.ah = 0x08;
	__dpmi_int(0x21, &regs);

	putchar('\n');
}


void Confirm_Hard_Drive_Formatting(int format) /* 0 unformat, 1 format */
{
	if (!format)
	{
		puts("UNFORMAT will revert your root directory and FAT to a");
		puts("previously recorded state. This can seriously mess up things!");
	}
	printf("\n WARNING: ALL DATA ON %s DISK\n",
	       (param.drive_type==HARD) ? "NON-REMOVABLE" : "FLOPPY");
	printf(" DRIVE %c: %s LOST! PLEASE CONFIRM!\n",
	       param.drive_letter[0],
	       format ? "WILL BE" : "MIGHT GET" );

	write(isatty(1) ? 1 : 2, " Proceed with ", 14);
	if (!format) write(isatty(1) ? 1 : 2, "un", 2);
	write(isatty(1) ? 1 : 2, "format (YES/NO)? ", 17);
	/* write to STDERR for the case of STDOUT being redirected */

	/* Get keypress and echo */

	regs.h.al = toupper(getch());
	if ( regs.h.al != 'Y' )
	{
		putchar('\n');
		Exit(5,10);	/* no Y confirm */
	}

	regs.h.al = toupper(getch());
	if ( regs.h.al != 'E' )
	{
		printf("\nYou have to type the whole word YES to confirm.");
		Exit(5,11); /* no YE confirm */
	}

	regs.h.al = toupper(getch());
	if ( regs.h.al != 'S' )
	{
		printf("\nYou have to type the whole word YES to confirm.");
		Exit(5,12); /* no YES confirm */
	}

	regs.h.al = toupper(getch()); /* (usually the ENTER after YES - accept anything here) */
	putchar('\n'); /* enter is only \r, so we still need the \n ... */
}


int Ask_Label(char * str)
{
	int label_len;

	write(isatty(1) ? 1 : 2, "Please enter volume label (max. 11 chars): ", 43);
	/* write to STDERR for the case of STDOUT being redirected */

	label_len = 0;
	str[0] = '\0';
	do {
		regs.h.al = toupper(getch());
		putchar(regs.h.al);

		if (regs.h.al == 3)	/* ctrl-c */
			Exit(3,3);	/* aborted by user */

		if ((regs.h.al > 'Z') ||
		        ((regs.h.al < ' ') && (regs.h.al != 8) && (regs.h.al != 13)) ||
		        ((label_len >= 11) && (regs.h.al != 13)) )
		{
			printf("\010\007\n%s",str);		/* beep! */
			continue;	/* buglet: invalid-key is like enter if first key */
		}

		switch (regs.h.al)
		{
		case 8:  if (label_len) label_len--;	/* backspace */
			printf(" \010");			/* really wipe */
			break;
		case 13: label_len = -label_len;	/* enter */
			break;
		default:
			str[label_len] = regs.h.al;	/* key */
			label_len++;
			str[label_len] = '\0';		/* terminating zero */
		} /* switch */
	} while (label_len > 0);

	putchar('\n');
	return (-label_len);				/* length of user input */
}


void Critical_Error_Handler(int source, unsigned int error_code)
{
	unsigned int error_code_high = (error_code & 0xff00) >> 8;
	unsigned int error_code_low  = error_code & 0x00ff;

	if (source==BIOS) {
		error_code_high = error_code_low;
		error_code_low  = 0x00;
	}
	printf("\n Critical error encountered while using %s disk driver",
	       (source==BIOS) ? "INT 13" : "DOS");

	/* Display Status Message. */

	if (source==BIOS) {
		printf("\n INT 13 status (hex): %2.2x", error_code_high);

		printf("\n   Bits: ");
		/* changed to allow bit fields -ea */
		if (error_code_high == 0x00) printf("none ");
		if (error_code_high & 0x01) printf("bad command ");
		if (error_code_high & 0x02) printf("bad address mark ");
		if (error_code_high & 0x04) printf("requested sector not found ");
		if (error_code_high & 0x08) printf("DMA troubles ");
		if (error_code_high & 0x10) printf("data error (bad CRC) ");
		if (error_code_high & 0x20) printf("controller failed ");
		if (error_code_high & 0x40) printf("seek operation failed ");
		if (error_code_high & 0x80) printf("timeout / no response ");

		printf("\n   Description: "); /* *** marks popular errors */
		switch (error_code_high) {
		case 0x01: printf("invalid function or parameter"); break;
		case 0x02: printf("address mark not found"); break; /* *** */
		case 0x03: printf("DISK WRITE-PROTECTED"); break; /* *** */
		case 0x04: printf("read error / sector not found"); break; /* *** */
		case 0x05: printf("harddisk reset failed"); break;
		case 0x06: printf("FLOPPY CHANGED"); break; /* *** */
		case 0x07: printf("drive parameter error"); break;
		case 0x08: printf("DMA overrun"); break;
		case 0x09: printf("DMA across 64k boundary attempted"); break; /* *** */
		case 0x0a: printf("bad sector on harddisk"); break;
		case 0x0b: printf("bad track on harddisk"); break;
		case 0x0c: printf("invalid media / track out of range"); break; /* *** */
		case 0x0d: printf("illegal sectors / track (PS/2)"); break;
		case 0x0e: printf("harddisk control data address mark detected"); break;
		case 0x0f: printf("harddisk DMA arbitration level overflow"); break;
		case 0x10: printf("READ ERROR (CRC/ECC wrong)"); break; /* *** */
		case 0x11: printf("read error corrected by CRC/ECC"); break;
		case 0x20: printf("controller failure"); break;
		case 0x31: printf("no media in drive"); break; /* (int 13 extensions) */
		case 0x32: printf("invalid CMOS drive type"); break; /* (Compaq) */
		case 0x40: printf("Seek failed"); break; /* *** */
		case 0x80: printf("timeout (drive not ready)"); break; /* *** */
		case 0x0aa: printf("timeout (harddisk not ready)"); break;
		case 0x0b0: printf("volume not locked in drive"); break; /* int 13 ext */
		case 0x0b1: printf("volume locked in drive"); break; /* int 13 ext */
		case 0x0b2: printf("volume not removable"); break; /* int 13 ext */
		case 0x0b3: printf("volume in use"); break; /* int 13 ext */
		case 0x0b4: printf("volume lock count overflow"); break; /* int 13 ext */
		case 0x0b5: printf("eject failed"); break; /* int 13 ext */
		case 0x0b6: printf("volume is read protected"); break; /* int 13 ext */
		case 0x0bb: printf("general harddisk error"); break;
		case 0x0cc: printf("harddisk WRITE ERROR"); break;
		case 0x0e0: printf("harddisk status register error"); break;
		case 0x0ff: printf("harddisk sense operation failed"); break;
		default: printf("(unknown error code)"); break;
		} /* switch */
		/* BIOS */
	} else { /* DOS */
		printf("\n DOS driver error (hex): %2.2x", error_code_low);

		printf("\n   Description: ");
		switch (error_code_low) {
		case 0x00: printf("write-protection violation attempted / none"); break;
		case 0x01: printf("unknown unit for driver"); break;
		case 0x02: printf("drive not ready"); break;
		case 0x03: printf("unknown command given to driver"); break;
		case 0x04: printf("data error (bad CRC)"); break;
		case 0x05: printf("bad device driver request structure length"); break;
		case 0x06: printf("seek error"); break;
		case 0x07: printf("unknown media type"); break;
		case 0x08: printf("sector not found"); break;
		case 0x09: printf("printer out of paper"); break;
		case 0x0a: printf("write fault"); break;
		case 0x0b: printf("read fault"); break;
		case 0x0c: printf("general failure"); break;
		case 0x0d: printf("sharing violation"); break;
		case 0x0e: printf("lock violation"); break;
		case 0x0f: printf("invalid disk change"); break;
		case 0x10: printf("FCB unavailable"); break;
		case 0x11: printf("sharing buffer overflow"); break;
		case 0x12: printf("code page mismatch"); break;
		case 0x13: printf("out of input"); break;
		case 0x14: printf("insufficient disk space"); break;
		default: printf("(unknown error code)"); break;
		} /* switch */
		error_code_high = error_code_low;	/* for return value */
	} /* DOS */

	/* *** We should probably allow an IGNORE or RETRY in *some* cases! *** */

	printf("\n Program terminated.\n");

	Exit(4, (128 | error_code_high) );	/* critical error handler */
}


/* 0.91k - avoid overflow if FAT32 */
void Display_Drive_Statistics()
{

#define DDS_unitstring ((param.fat_type == FAT32) ? "kbytes" : "bytes")
#define DDS_factor(x) ((param.fat_type == FAT32) ? ((x)>>1) : ((x) * \
	drive_statistics.bytes_per_sector))
	/* a cheat: ASCII_CD_Number puts the decimal point at index 15... */
#define DDS_dp ((param.fat_type == FAT32) ? ascii_cd_number[15] : ' ')
#define DDS_half(x) ((param.fat_type == FAT32) ? (((x) & 1) ? "5" : "0") : "")

	if ((drive_statistics.bytes_per_sector != 512) && (param.fat_type == FAT32))
		puts("Sector size is not 512 bytes - kbyte values will be wrong.");

	/* changed in 0.91h */
	/* avail = data area size (w/o root dir), total = "diskimage" size */
	drive_statistics.allocation_units_available_on_disk
	= drive_statistics.sect_available_on_disk
	  / drive_statistics.sect_in_each_allocation_unit;

	drive_statistics.sect_available_on_disk -=
	    ( drive_statistics.allocation_units_with_bad_sectors
	      * drive_statistics.sect_in_each_allocation_unit );

	ASCII_CD_Number(DDS_factor(drive_statistics.sect_total_disk_space));
	printf("\n%13s%c%s %s total disk space (disk size)\n",
	       ascii_cd_number, DDS_dp, DDS_half(drive_statistics.sect_total_disk_space),
	       DDS_unitstring);

	if (drive_statistics.bad_sectors > 0) /* changed 0.91c */
	{
		ASCII_CD_Number(DDS_factor(drive_statistics.bad_sectors));
		printf("%13s%c%s %s in bad sectors\n",
		       ascii_cd_number, DDS_dp, DDS_half(drive_statistics.bad_sectors),
		       DDS_unitstring);
		ASCII_CD_Number(DDS_factor(drive_statistics.sect_total_disk_space -
		                           drive_statistics.sect_available_on_disk));
		printf("%13s%c%s %s in clusters with bad sectors\n",
		       ascii_cd_number, DDS_dp, DDS_half(drive_statistics.sect_total_disk_space -
		                                         drive_statistics.sect_available_on_disk), DDS_unitstring);
	}

	ASCII_CD_Number(DDS_factor(drive_statistics.sect_available_on_disk));
	printf("%13s%c%s %s available on disk (free clusters)\n",
	       ascii_cd_number, DDS_dp, DDS_half(drive_statistics.sect_available_on_disk),
	       DDS_unitstring);

	putchar('\n');

	ASCII_CD_Number(DDS_factor(drive_statistics.sect_in_each_allocation_unit));
	printf("%13s%c%s %s in each allocation unit.\n",
	       ascii_cd_number, DDS_dp, DDS_half(drive_statistics.sect_in_each_allocation_unit),
	       DDS_unitstring);

	ASCII_CD_Number(drive_statistics.allocation_units_available_on_disk);
	printf("%13s%s allocation units on disk.\n", ascii_cd_number,
	       (param.fat_type == FAT32) ? "  " : "");

	if (drive_statistics.allocation_units_with_bad_sectors > 0) /* 0.91c */
	{
		ASCII_CD_Number(drive_statistics.allocation_units_with_bad_sectors);
		printf("%13s%s of the allocation units marked as bad\n", ascii_cd_number,
		       (param.fat_type == FAT32) ? "  " : "");
	}

	printf("\n Volume Serial Number is %04X-%04X\n",
	       drive_statistics.serial_number_high, drive_statistics.serial_number_low);
}


void Display_Invalid_Combination()
{
	puts("\n Invalid combination of options... please read help. Aborting.");
	Exit(4,2);
}


void Key_For_Next_Page(void);

void Key_For_Next_Page()
{
	if (!isatty(1))
		return; /* redirected? then do not wait. */
	/* interesting: redirection to MORESYS (>MORE$) still is a TTY   */
	/* redirection to a file is not a TTY, so waiting is avoided :-) */

	printf("-- press enter to see the next page or ESC to abort  --");

	/* Get keypress */
	regs.h.ah = 0x07;
	__dpmi_int(0x21, &regs);
	if (regs.h.al == 27)
	{
		puts("\nAborted at user request.");
		Exit(3,13);
	}

	puts("\n");

} /* Key_For_Next_Page() */


/* Help Routine (removed legacy stuff in 0.91c, but it is still parsed) */
/* There should be an help file which explains everything in detail,    */
/* including the (hardly ever used but still supported) legacy options! */
/* 0.91n - if detailed is non-zero, display multi-page help screen. */
void Display_Help_Screen(int detailed)
{
	printf("FreeDOS %6s 32-bit Version %s\n",NAME,VERSION);
	puts("Written by Brian E. Reifsnyder (and several contributors).");
	puts("Copyright 1999 - 2004 under the terms of the GNU GPL, Version 2.\n");

	if (detailed)
		puts("Syntax (see documentation for more details background information):\n");
	else
		puts("Syntax (see documentation or use -Z:longhelp for more options):\n");

#if LEGACY_HELP /* with legacy stuff */
	puts("FORMAT drive: [-V[:label]] [-Q] [-U] [-F:size] [-B | -S] [-D]");
	puts("FORMAT drive: [-V[:label]] [-Q] [-U] [-T:tracks -N:sectors] [-B | -S] [-D]");
	puts("FORMAT drive: [-V[:label]] [-Q] [-U] [-4] [-B | -S] [-D]");
	puts("FORMAT drive: [-Q] [-U] [-1] [-4] [-8] [-B | -S] [-D]\n");
#else /* new - without legacy stuff */
	puts("FORMAT drive: [-V[:label]] [-Q] [-U] [-F:size] [-S] [-D]");
	puts("FORMAT drive: [-V[:label]] [-Q] [-U] [-T:tracks -N:sectors] [-S] [-D]");
	/* the /4 option is a legacy shorthand for size selection: 360k in 1.2M drive */
	/* (drive type detection and "double stepping" setting are automatic on ATs.) */
	puts("FORMAT drive: [-V[:label]] [-Q] [-U] [-4] [-S] [-D]\n");
#endif

	puts(" -V:label   Specifies a volume label for the disk, stores date and time of it.");
	puts(" -S         Calls SYS to make the disk bootable and to add system files.");

#if LEGACY_HELP /* legacy, DOS 1.x (/B cannot be combined with /S) */
	puts(" -B         Kept for compatibility, formerly reserved space for the boot files.");
#endif
	puts(" -D         Be very verbose and show debugging output. For bug reports.");

	puts(" -Q         Quick formats the disk. If not combined with -U, can be UNFORMATed");
	puts("            and preserves bad cluster marks (-Q -U does not).");
	/* preserving the bad cluster list is new in 0.91k */
	puts(" -U         Unconditionally formats the disk. Lowlevel format if floppy disk.");

	puts(" -F:size    Specifies the size of the floppy disk to format. Normal sizes are:");
	puts("            360, 720, 1200, 1440, or 2880 (unit: kiloBytes). -F:0 shows a list.");
	puts(" -4         Formats a 360k floppy disk in a 1.2 MB floppy drive.");
	puts(" -T:tracks  Specifies the number of tracks on a floppy disk.");
	puts(" -N:sectors Specifies the number of sectors on a floppy disk.");

#if LEGACY_HELP /* legacy, DOS 1.x */
	puts(" -1         Formats a single side of a floppy disk (160k - 180k).");
	puts(" -8         Formats a 5.25\" disk with 8 sectors per track (160k / 320k).");
#endif

	if (!detailed) return; /* stop here for normal, short, help screen */

	putchar('\n'); /* we got enough space for that */
	Key_For_Next_Page();

	puts("This FORMAT (32-bit version) is made for the http://www.freedos.org/project and http://freedos-32.sourceforge.net.");
	puts("  See http://www.gnu.org/ for information about GNU GPL license.");
	puts("Made in 1999-2003 by Brian E. Reifsnyder <reifsnyderb@mindspring.com>");
	puts("  Maintainer of 0.90 / 0.91 series: Eric Auer <eric@coli.uni-sb.de>");
	puts("Contributors: Jan Verhoeven, John Price, James Clark, Tom Ehlert,");
	puts("  Bart Oldeman, Jim Hall and others. Not to forget all the testers!\n");

	puts("Switches and additional features explained:");
	puts("-B (reserve space for sys) is dummy and cannot be combined with -S (sys)");
	puts("-V:label is not for 160k/320k disks. The label stores the format date/time.\n");

	puts("Size specifications only work for floppy disks. You can use");
	puts("either -F:size (in kilobytes, size 0 displays a list of allowed sizes)");
	puts("or     -T:tracks -N:sectors_per_track");
	puts("or any combination of -1 (one-sided, 160k/180k),");
	puts("                      -8 (8 sectors per track, 160k/320k, DOS 1.x)");
	puts("                  and -4 (format 160-360k disk in 1200k drive)\n");
	puts("To suppress the harddisk format confirmation prompt, use    -Z:seriously");
	puts("To save only unformat (mirror) data without formatting, use -Z:mirror");
	puts("To UNFORMAT a disk for which fresh mirror data exists, use  -Z:unformat\n");
	/* we got enough space for that */
	Key_For_Next_Page();

	puts("Modes for FLOPPY are: Tries to use quick safe format. Use lowlevel format");
	puts("  only when needed. Quick safe format saves mirror data for unformat.");
	puts("Modes for HARDDISK are: Tries to use quick safe format. Use quick full");
	puts("  format only when needed. Quick full format only resets the filesystem");
	puts("If you want to force lowlevel format (floppy) or want to have the whole");
	puts("  disk surface scanned and all contents wiped (harddisk), use -U.");
	puts("  FORMAT -Q -U is quick full format (no lowlevel format / scan / wipe!)");
	puts("  FORMAT -Q is quick save format (save mirror data if possible)");
	puts("    the mirror data will always overwrite the end of the data area!");
	puts("  FORMAT autoselects a mode (see above) if you select neither -Q nor -U\n");

	puts("Supported FAT types are: FAT12, FAT16, FAT32, all with mirror / unformat.");
	puts("Supported floppy sizes are: 160k 180k 320k 360k and 1200k for 5.25inch");
	puts("  and 720k and 1440k (2880k never tested so far) for 3.5inch drives.");
	puts("Supported overformats are: 400k 800k 1680k (and 3660k) with more sectors");
	puts("  and 1494k (instead of 1200k) and 1743k (and 3486k) with more tracks, too.");
	puts("  More tracks will not work on all drives, use at your own risk.");
	puts("  Warning: older DOS versions can only use overformats with a driver.");
	puts("  720k in 1440k is known to be problematic. 360k in 1200k 'depends'...\n");

	puts("For FAT32 formatting, you can use the -A switch to force 4k alignment.");
	puts("WARNING: Running FORMAT for FAT32 under Win98 seems to create broken format!");

} /* Display_Help_Screen */


void Display_Percentage_Formatted(unsigned long percentage)
{
	if (isatty(1 /* stdout */))
		printf("%3ld percent completed.\r", percentage); /* on screen */
	else
		printf("%3ld percent completed.\n", percentage); /* for redirect */
	/* \r re-positions cursor back to the beginning of the line */
}


void IllegalArg(char *option, char *argptr)
{
	printf("Parameter value not allowed - %s%s\n", option, argptr);
	Exit(4,14);
}

