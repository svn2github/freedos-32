/*
// Program:  Format
//
// Modified By: Hanzac Chen, 2005
// For being compiled by DJGPP, compatible and portable
//
// Version:  0.91s
// (0.90b/c/d - warnings fixed, buffers far, boot sector IO - EA 2003)
// (0.91b..j / k... - more fixes - Eric Auer 2003 / 2004)
// (0.91q Locking more Win9x compatible - EA)
// Written By:  Brian E. Reifsnyder
// Copyright:  2002-2004 under the terms of the GNU GPL, Version 2
// Module Name:  driveio.c
// Module Description:  Functions specific to accessing a disk.
*/

#define DIO

#include <string.h>
#include <unistd.h>

#include "format.h"
#include "driveio.h"
#include "userint.h" /* Critical_* */


/* Clear Huge Sector Buffer */
void Clear_Huge_Sector_Buffer(void)
{
	dos_memset(huge_sector_buffer, 0, 512*HUGE_SECTOR_BUFFER_SNUM);
//	memset(huge_sector_buffer_1, 0, sizeof(huge_sector_buffer_1));
}



/* Clear Sector Buffer */
void Clear_Sector_Buffer(void)
{
	dos_memset(sector_buffer, 0, 512);
//	memset(sector_buffer_1, 0, sizeof(sector_buffer_1));
}


/* exit program, after unlocking drives - introduced 0.91r  */
/* supports "normal" (MS) and "verbose" (FD) errorlevels    */
/* MS: 0 ok, 3 user aborted, 4 fatal error, 5 not confirmed */
void Exit(int mserror, int fderror)
{
	if ((mserror==3) || (mserror==4)) {	/* aborted by user / fatal error? */
		Lock_Unlock_Drive(0);		/* remove inner lock */
		Lock_Unlock_Drive(0);		/* remove outer lock */
		/* if there were less locks than unlocks, no problem: checked below. */
	}

	#ifdef __FORMAT_DEBUG__
	exit(fderror);	/* use the "verbose errorlevel" */
	#endif
	if (mserror==4)	/* "fatal error"? */
		printf(" [Code %d]\n", fderror);
	exit(mserror);	/* use the "normal errorlevel" */
} /* Exit */


/* 0.91l - Win9x / DOS 7+: LOCK (lock TRUE) / UNLOCK (lock FALSE) drive */
/* changed 0.91q: lock can be 2 for the nested second level 0 lock...   */
void Lock_Unlock_Drive(int lock)
{
	static int lock_nesting = 0;
	/* 0.91q: As recommended by http://msdn.microsoft.com "Level 0 Lock":   */
	/* get level 0 lock, use filesystem functions to prepare (if needed),   */
	/* get level 0 lock AGAIN, this time with permissions 4, use sector I/O */
	/* and IOCTL functions or int 13 to create filesystem, then release the */
	/* inner level 0 lock, use filesystem functions for cleanup, release    */
	/* the outer level 0 lock, done. You must NOT drop the locks or exit at */
	/* a moment when the filesystem is in inconsistent / nonFAT state, or   */
	/* Win9x will assume that the filesystem is still as before the lock... */

	/* drive existence check would be nice but not working yet - 0.91r */
	/* for now: LOCK as drive exist check lucklily DOES work in FAT32 kernels */
	/* (unformatted drives are reported as existing, just what we need...)    */
	/* LOCK and Get_Device_Parameters both return error 0x0f for completely   */
	/* non-existing drives / okay for unformatted... G.D.P. good for FATxx :) */
	/* ===> For now, we use G.D.P., LOCK and BootSecRead as drive and file-   */
	/* system detectors. Note: if no filesystem, B.S.R. *should* return 0x07, */
	/* unknown media, but in FreeDOS 2035 it is 1 / 0x0c (1x/32), yuck! <===  */
	/* The PROBLEM: IF DOS 6-, "invalid drive" detected only AFTER confirm... */

	/* *** 0.91l try to LOCK the drive (logical volume) *** */
	if (_osmajor < 7) /* dos.h: Major DOS version number. 7 is "Win9x" */
		return;		/* ignore the whole locking on older DOS versions */

	if ((lock_nesting==0) && (!lock)) {
		#ifdef __FORMAT_DEBUG__
		printf("[DEBUG]  Extraneous drive unlock attempt ignored.\n");
		#endif
		return;
	}

	#ifdef __FORMAT_DEBUG__
	printf("[DEBUG]  DOS 7+ detected, %sing drive%s\n",
		lock ? ( (lock>1) ? "FORMAT-LOCK" : "LOCK" ) : "UNLOCK", lock ? "" : " (by one level)");
	#endif
	/* (for UNLOCK, BH/DX are irrelevant) */

	regs.x.cx = lock ? 0x084a : 0x086a; /* (un)LOCK logical FAT1x volume */
	do
	{
	regs.x.ax = 0x440d; /* IOCTL */
	if ((!lock) && (param.fat_type == FAT32))
		regs.h.ch = 0x48; /* FAT32 */
	regs.h.bh = 0;	/* big fat lock, not one of the "levels" 1..3 */
	regs.h.bl = param.drive_number + 1; /* A: is 1 etc. */
	regs.x.dx = (lock>1) ? 4 : 0;	/* when getting SECOND level 0 lock */
	/* ... then select "format" permissions! */
	regs.x.flags &= ~CARRY_FLAG;	/* (2 is block new file mappings) */
	__dpmi_int(0x21, &regs); /* call int 0x21 DOS API */
	regs.h.ch += 0x40;    /* try FAT32 (etc.) mode next */
	if (!lock)
		regs.h.ch = 0x77; /* give up if earlier if unlocking */
	} while ( (regs.h.ch <= 0x48) && (regs.x.flags&CARRY_FLAG));

	if ((regs.x.flags&CARRY_FLAG) && lock) /* ignore unlock errors */
		{
			unsigned int lockerrno = regs.x.ax;

			if (lockerrno == 0x0f) /* invalid drive */
			{
				printf(" Invalid drive! Aborting.\n");
				Exit(4,29);
			}

			regs.x.ax = 0x3000; /* get full DOS version and OEM ID */
			__dpmi_int(0x21, &regs);
			if (regs.h.bh == 0xfd)
				printf(" FreeDOS log. drive lock error 0x%x ignored.\n", lockerrno);
			else
			{
				printf(" Could not lock logical drive (error 0x%x)! Aborting.\n", lockerrno);
				Exit(4,25);
			}
		} /* LOCK error */

	/* for int 13 writes you have to lock the physical volume, too */
	if ((param.drive_type == FLOPPY) && (lock!=1)) /* 0.91q: not for outer level */
	{
		regs.x.ax = 0x440d;  /* IOCTL */
		regs.x.cx = lock ? 0x084b : 0x086b; /* (un)LOCK physical FAT1x volume */
		if ((!lock) && (param.fat_type == FAT32)) regs.h.ch = 0x48; /* FAT32 */
		regs.h.bh = 0;       /* lock level (or use 3?) (DX / BL still set) */
		/* or should BL be the BIOS driver number now (MSDN)? RBIL: 01=a: */
		regs.x.flags &= ~CARRY_FLAG;
		__dpmi_int(0x21, &regs); /* call int 0x21 DOS API */
		if ((regs.x.flags&CARRY_FLAG) && lock) /* ignore unlock errors */
			{
				unsigned int lockerrno = regs.x.ax;

				regs.x.ax = 0x3000; /* get full DOS version and OEM ID */
				__dpmi_int(0x21, &regs);
				if (regs.h.bh == 0xfd)
					printf(" FreeDOS phys. drive lock error 0x%x ignored.\n", lockerrno);
				else
					printf(" Could not lock physical floppy drive for formatting (error 0x%x)!?\n",
					       lockerrno);
				/* could { ... exit(1); } here, but MSDN does not even */
				/* suggest getting a physical drive lock at all..!? */
			} /* LOCK error */
	} /* extra FLOPPY treatment */

	lock_nesting += (lock) ? 1 : -1;	/* maintain lock nesting count */
}   /* *** drive is now LOCKed if lock, else UNLOCKed *** */



/* changed 0.91k: if number of sectors is negative, do not abort on fail */
int Drive_IO(int command,unsigned long sector_number,int number_of_sectors)
{
	unsigned int return_code;
	int error_counter = 0;
	int allowfail = (number_of_sectors < 0);
	if (allowfail) number_of_sectors =  -number_of_sectors;
	if (!number_of_sectors)
	{
		printf("Drive_IO: zero sized read/write request!? Ignored.\n");
		return -1;
	}

retry:

	if (param.fat_type != FAT32)
	{
		return_code = TE_AbsReadWrite(param.drive_number,number_of_sectors,sector_number,
			(number_of_sectors==1) ? sector_buffer : huge_sector_buffer, command);
	} else {
		return_code = FAT32_AbsReadWrite(param.drive_number,number_of_sectors,sector_number,
			(number_of_sectors==1) ? sector_buffer : huge_sector_buffer, command);
		/* return code: lo is int 24 / DI code, hi is bitmask: */
		/* 1 bad command 2 bad address mark 3 (int 26 only) write protection */
		/* 4 sector not found 8 dma failure 10 crc error 20 controller fail  */
		/* 40 seek fail 80 timeout, no response */
	}

	if ( (return_code==0x0207 /* 0x0408 */) &&
	        ((param.drive_number & 0x80)==0) /* avoid fishy retry */
	   ) /* changed -ea */
	{
		/* As per RBIL, if 0x0408 is returned, retry with high bit of AL set. */
		/* MY copy of RBIL mentions 0x0207 for that case!? -ea */
		param.drive_number = param.drive_number + 0x80; /* set high bit. */
		error_counter++;
		#ifdef __FORMAT_DEBUG__
		putchar('+'); /* give some hint about the change -ea */
		#endif
		goto retry;
	}

	if ( (return_code!=0) && (error_counter<3) )
	{
		error_counter++;
		#ifdef __FORMAT_DEBUG__
		putchar('*'); /* give some problem indicator -ea */
		#endif
		goto retry;
	}

	if (return_code!=0)
	{
		if ( ((param.drive_number & 0x7f)>=2) ||
		        (command==WRITE) || (sector_number!=0L)  )
		{
			if (allowfail)
			{
				#ifdef __FORMAT_DEBUG__
				printf("* bad sector(s): %ld (code 0x%x) on %s *\n",
					sector_number, return_code, (command==WRITE) ? "WRITE" : "READ");
				#else
				putchar('#');
				#endif
			}
			else
			{
				/* Added more details -ea */
				printf("Drive_IO( command=%s sector=%ld count=%d ) [%s] [drive %c%c]\n",
					(command==WRITE) ? "WRITE" : "READ", sector_number, number_of_sectors,
					(param.fat_type==FAT32) ? "FAT32" : "FAT12/16",
					'A' + (param.drive_number & 0x7f),
					(param.drive_number & 0x80) ? '*' : ':' );
				Critical_Error_Handler(DOS, return_code);
			}
		}
		else
		{
			/* Only HARDDISK and WRITE and READ-beyond-BOOTSECTOR errors   */
			/* are CRITICAL, others are handled by the CALLER instead! -ea */
			printf("#");
		}
	}

	param.drive_number = param.drive_number & 0x7f; /* unset high bit. */
	return (return_code);
}



/* Do not confuse this with DOS 7.0+ drive locking */
void Enable_Disk_Access(void) /* DOS 4.0+ drive access flag / locking */
{
	unsigned char category_code;
	unsigned long error_code=0;

	category_code = (param.fat_type == FAT32) ? 0x48 : 0x08;

	/* Get the device parameters for the logical drive */

	regs.h.ah	= 0x44;						/* IOCTL Block Device Request */
	regs.h.al	= 0x0d;						/* IOCTL */
	regs.h.bl	= param.drive_number + 1;
	regs.h.ch	= category_code;			/* 0x08 if !FAT32, 0x48 if FAT32 */
	regs.h.cl	= 0x67;						/* Get Access Flags */
	regs.x.ds	= FP_SEG(transfer_buffer);
	regs.x.dx	= FP_OFF(transfer_buffer);
	/* TODO: Using linear address because the DS's base is 0x0? */

	__dpmi_int(0x21, &regs);
	/* Copy to the access_flags from transfer buffer */
	dosmemget(transfer_buffer, sizeof(AF), &access_flags);
	access_flags.special_function = 0;
	error_code = regs.h.al;

	if (regs.x.flags&CARRY_FLAG)
	{
		/* BO: if invalid function: try to format anyway maybe access
		 flags do not work this way in this DOS (e.g. DRDOS 7.03) */
		if (error_code == 0x1 || error_code == 0x16)
		{
			printf("No disk access flags used (IOCTL int 21.440d.x8.67)\n");
			return;
		}

		/* Add error trapping here */
		printf("\nCannot get disk access flags (error %02lx). Giving up.\n", error_code);
		Exit(4,26);
	}

	if (access_flags.disk_access==0) /* access not yet enabled? */
	{
		access_flags.disk_access = 1;
		dosmemput(&access_flags, sizeof(AF), transfer_buffer);

		regs.h.ah = 0x44;                     /* IOCTL Block Device Request */
		regs.h.al = 0x0d;                     /* IOCTL */
		regs.h.bl = param.drive_number + 1;
		regs.h.ch = category_code;            /* 0x08 if !FAT32, 0x48 if FAT32 */
		regs.h.cl = 0x47;                     /* Set access flags */
		regs.x.es = FP_SEG(transfer_buffer);
		regs.x.dx = FP_OFF(transfer_buffer);
		__dpmi_int(0x21, &regs);

		error_code = regs.h.al;
		/* Copy to the access_flags from transfer buffer */
		dosmemget(transfer_buffer, sizeof(AF), &access_flags);
		
		if (regs.x.flags&CARRY_FLAG)
		{
			/* Add error trapping here */
			printf("\nCannot enable disk access flags (error %02lx). Giving up.\n", error_code);
			Exit(4,27);
		}
	}

	#ifdef __FORMAT_DEBUG__
	printf("[DEBUG]  Enabled disk access flags.\n");
	#endif
}



/* FAT32_AbsReadWrite() is modified from TE_AbsReadWrite(). */
unsigned int FAT32_AbsReadWrite(char DosDrive, int count, ULONG sector,
									ULONG buffer, unsigned int ReadOrWrite)
{
	struct DRP {
		unsigned long  sectorNumber;
		unsigned short count;
		unsigned short offset;
		unsigned short segment;	/* transfer address */
	} diskReadPacket;

	struct {
		unsigned direction  : 1 ; /* low bit */
		unsigned reserved_1 : 12;
		unsigned write_type : 2 ;
		unsigned reserved_2 : 1 ;
	} mode_flags;

	diskReadPacket.sectorNumber	= sector;
	diskReadPacket.count		= count;
	diskReadPacket.segment		= FP_SEG(buffer);
	diskReadPacket.offset		= FP_OFF(buffer);
	/* Using transfer buffer */
	dosmemput(&diskReadPacket, sizeof(struct DRP), transfer_buffer);

	mode_flags.reserved_1 = 0;
	mode_flags.write_type = 0;
	mode_flags.direction  = (ReadOrWrite == READ) ? 0 : 1;
	mode_flags.reserved_2 = 0;

	DosDrive++;

	/* no inline asm for Turbo C 2.01! -ea */
	/* Turbo C also complains about packed bitfield structures -ea */
	regs.x.ax = 0x7305;
	regs.x.ds = FP_SEG(transfer_buffer);
	regs.x.bx = FP_OFF(transfer_buffer);
	regs.x.cx = 0xffff;
	regs.h.dl = DosDrive;
	regs.x.si = mode_flags.direction; /* | (mode_flags.write_type << 13); */
	__dpmi_int(0x21, &regs);
	return ((regs.x.flags&CARRY_FLAG) ? regs.x.ax : 0);
}

/* TE_* not usable while boot sector invalid, so... */
static unsigned int FloppyBootReadWrite(char DosDrive, int count, ULONG sector,
									ULONG buffer, unsigned int ReadOrWrite)
{
	int i = 0;
	int error = 0;

	if ((DosDrive > 1) || ((DosDrive & 0x80) != 0) || (sector != 0) || (count != 1))
		return 0x0808; /* sector not found (DOS) */

	do
	{
		if (ReadOrWrite == WRITE)
			regs.x.ax = 0x0301; /* write 1 sector */
		else
			regs.x.ax = 0x0201; /* read 1 sector */
		regs.x.cx = 1; /* 1st sector, 1th (0th) track */
		regs.h.dl = DosDrive; /* drive ... */
		regs.h.dh = 0; /* 1st (0th) side */
		/* Using dos memory buffer because of the protected mode */
		regs.x.es = FP_SEG(buffer);
		regs.x.bx = FP_OFF(buffer);
		regs.x.flags &= ~CARRY_FLAG;
		__dpmi_int(0x13, &regs);
		/* carry check added 0.91s */
		if ((regs.x.flags&CARRY_FLAG) && (regs.h.ah != 0)) {
			i++; /* error count */
			error = regs.h.ah;   /* BIOS error code */
			if (error == 6)
				i--; /* disk change does not count as failed attempt */
		} else {
			return 0; /* WORKED */
		}
	} while (i < 3); /* do ... while */ /* 3 attempts are enough */
	/* (you do not want to use floppy disk with weak boot sector) */

	return error; /* return ERROR code */
	/* fixed in 0.91b: theerror / AH, not AX is the code! */
}

/* TE_AbsReadWrite() is written by Tom Ehlert. */
unsigned int TE_AbsReadWrite(char DosDrive, int count, ULONG sector,
								ULONG buffer, unsigned int ReadOrWrite)
{
	struct DRP {
		unsigned long  sectorNumber;
		unsigned short count;
		unsigned short offset;
		unsigned short segment;	/* transfer address */
	} diskReadPacket;

	if ( (count==1) && (sector==0) && ((DosDrive & 0x7f) < 2) )
	{
		/* ONLY use lowlevel function if only a floppy boot sector is     */
		/* affected: maybe DOS simply does not know filesystem params yet */
		/* the lowlevel function also tells DOS about the new fs. params! */
		regs.x.ax = FloppyBootReadWrite(DosDrive & 0x7f, 1, 0, buffer, ReadOrWrite);
		/* (F.B.R.W. by -ea) */

		#ifdef __FORMAT_DEBUG__
		printf("[%2.2x]", regs.h.al);
		#endif

		switch (regs.h.al) { /* translate BIOS error code to DOS code */
			/* AL is the AH that int 13h returned... */
			case 0: return 0; /* no error */
			case 3: return 0x0300; /* write protected */
			case 0x80: return 0x0002; /* drive not ready */
			case 0x0c: return 0x0007; /* unknown media type */
			case 0x04: return 0x000b; /* read fault */
			default: break;
		} /* case */
		return 0x0808;
		/* everything else: "sector not found" */
		/* 02 - address mark not found, 07 - drive param error */
	}

	diskReadPacket.sectorNumber	= sector;
	diskReadPacket.count		= count;
	diskReadPacket.segment		= FP_SEG(buffer);
	diskReadPacket.offset		= FP_OFF(buffer);
	/* Using transfer buffer */
	dosmemput(&diskReadPacket, sizeof(struct DRP), transfer_buffer);

	regs.h.al = DosDrive; /* 0 is A:, 1 is B:, ... */
	/* in "int" 25/26, the high bit of AL is an extra flag */
	regs.x.ds = FP_SEG(transfer_buffer);
	regs.x.bx = FP_OFF(transfer_buffer);
	regs.x.cx = 0xffff;

	switch(ReadOrWrite)
	{
		case READ:  __dpmi_int(0x25, &regs); break;
		case WRITE: __dpmi_int(0x26, &regs); break;
		default:
			printf("DriveIO...ReadWrite: no mode %02x\n", ReadOrWrite);
			Exit(4,28);
	}

	return (regs.x.flags&CARRY_FLAG) ? regs.x.ax : 0;
}
