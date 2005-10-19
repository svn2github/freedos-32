/* The FreeDOS-32 Floppy Driver
 * Copyright (C) 2003-2005  Salvatore ISAJA
 *
 * This file "blockio.c" is part of the FreeDOS-32 Floppy Driver (the Program).
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
#include "blockio.h"
#include <errno.h>


#if FLOPPY_CONFIG_DEBUG
  #define LOG_PRINTF(x) fd32_log_printf x
#else
  #define LOG_PRINTF(x)
#endif


/* Bits for Floppy::buffers::flags */
enum
{
    BUF_VALID = 1 << 0, /* Buffered data is valid   */
    BUF_BAD   = 1 << 1  /* Cylinder has bad sectors */
};


/* Converts an absolute sector number (LBA) to CHS using disk geometry.
 * Returns 0 on success, or a negative value meaning invalid sector number.
 */
static inline int lba_to_chs(const Floppy *floppy, unsigned lba, Chs *chs)
{
	unsigned temp;
	const FloppyFormat *fmt = floppy->fdd->fmt;
	if (lba >= fmt->size) return -1;
	chs->c = lba / floppy->cylinder_size;
	temp   = lba % floppy->cylinder_size;
	chs->h = temp / fmt->sec_per_trk;
	chs->s = temp % fmt->sec_per_trk + 1;
	return 0;
}


/* Implementation of BlockOperations::open() */
int floppy_open(void *handle)
{
	Floppy *floppy = (Floppy *) handle;
	int res;
	if (floppy->locked) return -EBUSY;
	#if FLOPPY_CONFIG_CACHE
	floppy->buffers[0].flags = 0;
	#endif
	res = fdc_log_disk(floppy->fdd);
	if (res < 0) return res;
	floppy->cylinder_size = floppy->fdd->fmt->sec_per_trk * floppy->fdd->fmt->heads;
	floppy->locked = true;
	return 0;
}


/* Implementation of BlockOperations::revalidate() */
int floppy_revalidate(void *handle)
{
	Floppy *floppy = (Floppy *) handle;
	int res;
	if (!floppy->locked) return -EINVAL;
	res = fdc_log_disk(floppy->fdd);
	if (res < 0) return res;
	return 0;
}


/* Implementation of BlockOperations::close() */
int floppy_close(void *handle)
{
	Floppy *floppy = (Floppy *) handle;
	if (!floppy->locked) return -EINVAL;
	#if FLOPPY_CONFIG_CACHE
	floppy->buffers[0].flags = 0;
	#endif
	floppy->locked = false;
	return 0;
}


/* Implementation of BlockOperations::get_device_info() */
int floppy_get_device_info(void *handle, BlockDeviceInfo *buf)
{
	Floppy *floppy = (Floppy *) handle;
	buf->flags = BLOCK_DEVICE_INFO_TFLOPPY
	#if FLOPPY_CONFIG_WRITE
	           | BLOCK_DEVICE_INFO_WRITABLE
	#endif
	           | BLOCK_DEVICE_INFO_REMOVABLE;
	/* 360 KiB drives have not a disk change line */
	if (floppy->fdd->dp->cmos_type > 1)
		buf->flags |= BLOCK_DEVICE_INFO_MEDIACHG;
	/* The following assumes that a single FDC is installed */
	buf->multiboot_id = 0x00FFFFFF | (floppy->fdd->number << 24);
	return 0;
}


/* Implementation of BlockOperations::get_medium_info() */
int floppy_get_medium_info(void *handle, BlockMediumInfo *buf)
{
	Floppy *floppy = (Floppy *) handle;
	if (!floppy->locked) return -EINVAL;
	buf->blocks_count = floppy->fdd->fmt->size;
	buf->block_bytes  = 512; /* XXX: Other sector sizes? */
	return 0;
}


/* Implementation of BlockOperations::read() */
ssize_t floppy_read(void *handle, void *buffer, uint64_t start, size_t count, int flags)
{
	size_t num_read = 0;
	Floppy *floppy = (Floppy *) handle;
	int res;
	Chs chs;
	#if FLOPPY_CONFIG_CACHE
	if (!(flags & BLOCK_RW_NOCACHE))
		while (num_read < count)
		{
			if (!((floppy->buffers[0].flags & BUF_VALID)
			   && (start >= floppy->buffers[0].start)
			   && (start <  floppy->buffers[0].start + floppy->cylinder_size)))
			{
				/* Cache miss. Try to read a whole cylinder */
				if (lba_to_chs(floppy, start, &chs) < 0) return -ENXIO;
				res = fdc_read_cylinder(floppy->fdd, chs.c, floppy->buffers[0].data);
				if (res == FDC_ATTENTION)
					return -BLOCK_ERROR(BLOCK_SENSE_ATTENTION, 0);
				floppy->buffers[0].flags = BUF_VALID;
				floppy->buffers[0].start = chs.c * floppy->cylinder_size;
				if (res != FDC_OK) floppy->buffers[0].flags |= BUF_BAD;
			}
			if (!(floppy->buffers[0].flags & BUF_BAD))
			{
				/* Read as many contiguous sectors as possible from the cache */
				size_t partial = floppy->cylinder_size;
				if (fdc_disk_changed(floppy->fdd))
					return -BLOCK_ERROR(BLOCK_SENSE_ATTENTION, 0);
				if (num_read + partial > count)
					partial = count - num_read;
				memcpy(buffer, &floppy->buffers[0].data[512 * (start - floppy->buffers[0].start)], 512 * partial);
				num_read += partial;
				start += partial;
				buffer += 512 * partial;
			}
			else
			{
				/* The cache contains bad sectors, fall back on single sector reads */
				if (lba_to_chs(floppy, start, &chs) < 0) return -ENXIO;
				res = fdc_read(floppy->fdd, &chs, buffer, 1);
				if (res < 0) return res;
				num_read++;
				start++;
				buffer += 512;
			}
		}
	else
	#endif /* #if FLOPPY_CONFIG_CACHE */
		while (num_read < count)
		{
			/* Cache disabled, fall back on single sector reads */
			if (lba_to_chs(floppy, start, &chs) < 0) return -ENXIO;
			res = fdc_read(floppy->fdd, &chs, buffer, 1);
			if (res < 0) return res;
			num_read++;
			start++;
			buffer += 512;
		}
	return (ssize_t) num_read;
}


/* Implementation of BlockOperations::write() */
ssize_t floppy_write(void *handle, const void *buffer, uint64_t start, size_t count, int flags)
{
	#if FLOPPY_CONFIG_WRITE
	size_t num_written = 0;
	Floppy *floppy = (Floppy *) handle;
	Chs chs;
	int res;
	while (num_written < count)
	{
		#if FLOPPY_CONFIG_CACHE
		/* If cached, write through */
		if (!(flags & BLOCK_RW_NOCACHE)
		 && (floppy->buffers[0].flags & BUF_VALID)
		 && (start >= floppy->buffers[0].start)
		 && (start <  floppy->buffers[0].start + floppy->cylinder_size))
			memcpy(&floppy->buffers[0].data[512 * (start - floppy->buffers[0].start)], buffer, 512);
		#endif
		if (lba_to_chs(floppy, start, &chs) < 0) return -ENXIO;
		LOG_PRINTF(("[FLOPPY] write_sector: C=%u, H=%u, S=%u\n", chs.c, chs.h, chs.s));
		res = fdc_write(floppy->fdd, &chs, buffer, 1);
		if (res < 0) return res;
		num_written++;
		start++;
		buffer += 512;
	}
	return num_written;
	#else /* not FAT_CONFIG_WRITE */
	return -ENOTSUP;
	#endif
}


/* Implementation of BlockOperations::sync() */
int floppy_sync(void *handle)
{
	return 0; /* No write-back caching at present */
}


/* Implementation of BlockOperations::test_unit_ready() */
int floppy_test_unit_ready(void *handle)
{
	int res = 0;
	if (fdc_disk_changed(((Floppy *) handle)->fdd))
		res = -BLOCK_ERROR(BLOCK_SENSE_ATTENTION, 0);
	return res;
}
