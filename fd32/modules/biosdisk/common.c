/* The FreeDOS-32 BIOSDisk Driver
 * a block device driver using the BIOS disk services.
 * Copyright (C) 2001-2005  Salvatore ISAJA
 *
 * This file "common.c" is part of the FreeDOS-32 BIOSDisk Driver (the Program).
 *
 * The Program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Program; see the file GPL.txt; if not, write to
 * the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/** \file
 * Operations common to Standard and IBM/MS Extended BIOSes
 * and to floppy and hard disk devices.
 */

#include "biosdisk.h"

#define NUM_RETRIES 5 /* Number of times to retry on error */
#define OP_READ     2 /* 'Read' INT 13h service number     */
#ifdef BIOSDISK_WRITE
 #define OP_WRITE   3 /* 'Write' INT 13h service number    */
#endif
//#define OP_VERIFY   4 /* 'Verify' INT 13h service number   */


/* Address packet to specify disk positions in extended INT 13h services */
typedef struct
{
	BYTE   size;       /* Size of this structure: 10h or 18h */
	BYTE   reserved;   /* 0 */
	WORD   num_blocks; /* Max 007Fh for Phoenix EDD */
	DWORD  buf_addr;   /* Seg:off pointer to transfer buffer */
	QWORD  start;      /* Starting absolute block number */
	#if 0
	QWORD  flat_addr;  /* (EDD-3.0, optional) 64-bit flat address of transfer
	                    * buffer; used if DWORD at 04h is FFFFh:FFFFh
	                    */
	#endif
}
__attribute__ ((packed)) AddressPacket;


/* Wrapper function for extended INT 13h 'read', 'write' and 'verify' services.
 * The 'lba' and 'num_sectors' values must be checked by the caller.
 * Returns the number of sector transferred on success, <0 on error.
 */
int biosdisk_ext_xfer(int operation, const Disk *d, DWORD lba, unsigned num_sectors)
{
	FD32_DECLARE_REGS(regs);
	AddressPacket ap;

	ap.size       = 16;
	ap.reserved   = 0;
	ap.num_blocks = (WORD) num_sectors;
	ap.buf_addr   = ((DWORD) d->tb_segment << 16) + (DWORD) d->tb_offset;
	ap.start      = lba + d->first_sector;
	fd32_memcpy_to_lowmem(d->ap_selector, 0, &ap, 16);

	AH(regs) = operation | 0x40; /* Can be 0x42=read, 0x43=write or 0x44=verify */
	DL(regs) = d->bios_number;
	DS(regs) = d->ap_segment;
	SI(regs) = d->ap_offset;
	fd32_int(0x13, regs);
	if (FLAGS(regs) & 0x1) return -1; //INT 13h error code in AH
	return num_sectors;
}


/* Wrapper function for standard INT 13h 'read', 'write' and 'verify' services.
 * The 'lba' and 'num_sectors' values must be checked by the caller.
 * Returns the number of sector transferred on success, <0 on error.
 */
int biosdisk_std_xfer(int operation, const Disk *d, DWORD lba, unsigned num_sectors)
{
	FD32_DECLARE_REGS(regs);

	/* Convert LBA (validated by the caller) to CHS. */
	unsigned c, h, s, temp;
	unsigned bytes_per_cyl = d->bios_h * d->bios_s;
	c = lba / bytes_per_cyl;
	temp = lba % bytes_per_cyl;
	h = temp / d->bios_s;
	s = temp % d->bios_s + 1;

	AH(regs) = operation; /* Can be 0x02=read, 0x03=write or 0x04=verify */
	AL(regs) = (BYTE) num_sectors; /* Must be nonzero */
	CH(regs) = (BYTE) c;
	CL(regs) = s | ((c & 0x300) >> 2);
	DH(regs) = h;
	DL(regs) = d->bios_number;
	ES(regs) = d->tb_segment;
	BX(regs) = d->tb_offset;
	fd32_int(0x13, regs);
	if (FLAGS(regs) & 0x1) return -1; //INT 13h error code in AH
	return num_sectors;
}


/* Resets the disk system, as required between access retries */
static int biosdisk_reset(const Disk *d)
{
	FD32_DECLARE_REGS(regs);
	AH(regs) = 0x00; /* Reset disk system */
	DL(regs) = d->bios_number;
	fd32_int(0x13, regs);
	if (FLAGS(regs) & 0x1) return -1; /* Error code in AH */
	return 0;
}


/* Reads sectors from the disk using the appropriate transfer function.
 * Returns the number of sector transferred on success, <0 on error.
 */
/* Implementation of BlockOperations::read() */
ssize_t biosdisk_read(void *handle, void *buffer, uint64_t start, size_t count, int flags)
{
	ssize_t res;
	unsigned k;
	const Disk *d = handle;
	if (start + count > d->total_blocks) count = d->total_blocks - start;
	res = count;
	while (count)
	{
		for (k = 0; k < NUM_RETRIES; k++)
		{
			int r = d->xfer(OP_READ, d, start, 1);
			if (r >= 0) break;
			biosdisk_reset(d);
		}
		if (k == NUM_RETRIES) return -1;
		fd32_memcpy_from_lowmem(buffer, d->tb_selector, 0, d->block_size);
		count--;
		start++;
		buffer += d->block_size;
	}
	return res;
}


#ifdef BIOSDISK_WRITE
/* Writes sectors to the disk using the appropriate transfer function.
 * Returns the number of sector transferred on success, <0 on error.
 */
/* Implementation of BlockOperations::write() */
ssize_t biosdisk_write(void *handle, const void *buffer, uint64_t start, size_t count, int flags)
{
	ssize_t res;
	unsigned k;
	const Disk *d = handle;
	if (start + count > d->total_blocks) count = d->total_blocks - start;
	res = count;
	while (count)
	{
		fd32_memcpy_to_lowmem(d->tb_selector, 0, (void *) buffer, d->block_size);
		for (k = 0; k < NUM_RETRIES; k++)
		{
			int r = d->xfer(OP_WRITE, d, start, 1);
			if (r >= 0) break;
			biosdisk_reset(d);
		}
		if (k == NUM_RETRIES) return -1;
		count--;
		start++;
		buffer += d->block_size;
	}
	return res;
}
#endif


/* Wrapper function for standard INT 13h 'detect disk change' service.
 * Returns 0 if disk has not been changed, a positive number if disk
 * has been changed or a negative number on error.
 */
int biosdisk_mediachange(const Disk *d)
{
	FD32_DECLARE_REGS(regs);
	AH(regs) = 0x16;   /* Floppy disk - Detect disk change */
	DL(regs) = d->bios_number;
	SI(regs) = 0x0000; /* to avoid crash on AT&T 6300 */
	fd32_int(0x13, regs);
	if (!(FLAGS(regs) & 0x1)) return 0; /* Carry clear is disk not changed */
	if (AH(regs) == 0x06) return 1; /* Disk changed */
	return -1; /* If AH is not 06h an error occurred */
}
