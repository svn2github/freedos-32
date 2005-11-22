/* The FreeDOS-32 BIOSDisk Driver
 * a block device driver using the BIOS disk services.
 * Copyright (C) 2001-2005  Salvatore ISAJA
 *
 * This file "request.c" is part of the FreeDOS-32 BIOSDisk Driver (the Program).
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
 * BIOSDisk driver request function.
 */

#include <dr-env.h>
#include <errno.h>
#include <devices.h>
#include <block/block.h>
#include "biosdisk.h"

/* Implementation of BlockOperations::open() */
static int biosdisk_open(void *handle)
{
	Disk *d = (Disk *) handle;
	if (d->open_count == 1)
		return -EBUSY;
	else
		d->open_count = 1;
	return 0;
}


/* Implementation of BlockOperations::revalidate() */
static int biosdisk_revalidate(void *handle)
{
	return 0;
}


/* Implementation of BlockOperations::close() */
static int biosdisk_close(void *handle)
{
	Disk *d = (Disk *) handle;
	d->open_count = 0;
	return 0;
}


/* Implementation of BlockOperations::get_device_info() */
static int biosdisk_get_device_info(void *handle, BlockDeviceInfo *buf)
{
	Disk *d = (Disk *) handle;
	buf->flags = d->type;
#ifdef BIOSDISK_WRITE
	buf->flags |= BLOCK_DEVICE_INFO_WRITABLE;
#endif
	/* The following assumes that a single FDC is installed */
	buf->multiboot_id = d->multiboot_id;
	return 0;
}


/* Implementation of BlockOperations::get_medium_info() */
static int biosdisk_get_medium_info(void *handle, BlockMediumInfo *buf)
{
	Disk *d = (Disk *) handle;

	if(d->open_count == 0)
		return -EBUSY;
	buf->blocks_count = (uint64_t)d->total_blocks;
	buf->block_bytes = d->block_size;
	return 0;
}


/* Implementation of BlockOperations::sync() */
static int biosdisk_sync(void *handle)
{
	return 0; /* No write-back caching at present */
}


/* Implementation of BlockOperations::test_unit_ready() */
/* TODO: Is it correct? */
static int biosdisk_test_unit_ready(void *handle)
{
	Disk *d = handle;
	if (!(d->priv_flags & REMOVABLE)) return -ENOTSUP;
	if (d->priv_flags & CHANGELINE) return biosdisk_mediachange(d);
	return 0;
}

static BlockOperations bops =
{
	.request	= biosdisk_request,
	.open		= biosdisk_open,
	.revalidate	= biosdisk_revalidate,
	.close		= biosdisk_close,
	.get_device_info = biosdisk_get_device_info,
	.get_medium_info = biosdisk_get_medium_info,
	.read		= biosdisk_read,
	.write		= biosdisk_write,
	.sync		= biosdisk_sync,
	.test_unit_ready = biosdisk_test_unit_ready
};

static unsigned refcount = 0;

int biosdisk_request(int function, ...)
{
	int res = -ENOTSUP;
	va_list ap;
	va_start(ap, function);
	switch (function)
	{
		case REQ_GET_OPERATIONS:
		{
			int type = va_arg(ap, int);
			void **operations = va_arg(ap, void **);
			if (type == BLOCK_OPERATIONS_TYPE)
				if (operations)
				{
					*operations = (void *) &bops;
					refcount++;
					res = 0;
				}
			break;
		}
		case REQ_GET_REFERENCES:
			res = refcount;
			break;
		case REQ_RELEASE:
			res = -EINVAL;
			if (refcount) res = --refcount;
			break;
		default:
			res = -ENOTSUP;
			break;
	}
	va_end(ap);
	return res;
}
