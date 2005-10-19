/* The FreeDOS-32 Floppy Driver
 * Copyright (C) 2003-2005  Salvatore ISAJA
 *
 * This is "fd32.c" - Driver code meaningful only for FreeDOS-32
 * This file is part of the FreeDOS-32 Floppy Driver (the Program).
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
#include <ll/i386/hw-data.h>
#include <ll/i386/error.h>
#include <errno.h>
#include <devices.h>
#include "blockio.h"


/* The following are defined as static arrays to reduce memory fragmentation */
#define MAX_FLOPPIES 4
static const char *dev_names[MAX_FLOPPIES] = { "fd0", "fd1", "fd2", "fd3" };
static Floppy      floppies[MAX_FLOPPIES];
static unsigned    refcount = 0;


static BlockOperations bops;


static int floppy_request(int function, ...)
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


static BlockOperations bops =
{
	.request         = floppy_request,
	.open            = floppy_open,
	.revalidate      = floppy_revalidate,
	.close           = floppy_close,
	.get_device_info = floppy_get_device_info,
	.get_medium_info = floppy_get_medium_info,
	.read            = floppy_read,
	.write           = floppy_write,
	.sync            = floppy_sync,
	.test_unit_ready = floppy_test_unit_ready
};


/* Registers the detected floppy device to the FD32 kernel.             */
/* This callback function is called every time a new drive is detected. */
/* Returns 0 on success, or a negative error code on failure.           */
/* TODO: Find a decent size for the track buffer. */
static int drive_detected(Fdd *fdd)
{
    static unsigned drive_num = 0;
    int             res;

    if (drive_num < MAX_FLOPPIES)
    {
        floppies[drive_num].fdd = fdd;
        floppies[drive_num].buffers[0].data  = (BYTE *) fd32_kmem_get(512 * 18 * 2);
        floppies[drive_num].buffers[0].start = (unsigned) -1;
        floppies[drive_num].buffers[0].flags = 0;
        res = block_register(dev_names[drive_num], floppy_request, (void *) &floppies[drive_num]);
        //res = fd32_dev_register(floppy_request, (void *) &floppies[drive_num], dev_names[drive_num]);
        drive_num++;
        if (res < 0) return res;
        return 0;
    }
    return -1; /* Too many floppy devices... Quite impossible to happen! */
}


/* Floppy Driver entry point. This is the function invoked by DynaLink. */
void floppy_init(void)
{
    if (fdc_setup(drive_detected) < 0)
    {
        message("[FLOPPY] Error while initializing floppy support\n");
        return;
    }
    message("[FLOPPY] Floppy Driver installed\n");
}
