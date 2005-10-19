/* The FreeDOS-32 Floppy Driver
 * Copyright (C) 2003-2005  Salvatore ISAJA
 *
 * This file "blockio.h" is part of the FreeDOS-32 Floppy Driver (the Program).
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
#ifndef __FD32FLOPPY_BLOCKIO_H
#define __FD32FLOPPY_BLOCKIO_H

#include "fdc.h"
#include <block/block.h>

#define FLOPPY_CONFIG_WRITE 1
#define FLOPPY_CONFIG_CACHE 1
#define FLOPPY_CONFIG_DEBUG 1
#define MAX_BUFFERS 1 /* Number of cacheable cylinders */

#ifndef __cplusplus
typedef enum { false = 0, true = !false } bool;
#endif


/* The Floppy structure is used to add buffered access. */
typedef struct Floppy
{
    Fdd   *fdd;           /* Floppy drive in the low-level driver */
    bool   locked;        /* True if exclusive lock is in place */
    size_t cylinder_size; /* Size in sectors of a cylinder buffer */
    struct
    {
        BYTE    *data;  /* Sector data (buffer_size sectors) */
        unsigned start; /* The first block (LBA) stored in data */
        unsigned flags; /* See BUF_* enums in floppy.c */
    }
    buffers[MAX_BUFFERS]; /* Cylinder-based sector cache */
}
Floppy;


int     floppy_open           (void *handle);
int     floppy_revalidate     (void *handle);
int     floppy_close          (void *handle);
int     floppy_get_device_info(void *handle, BlockDeviceInfo *buf);
int     floppy_get_medium_info(void *handle, BlockMediumInfo *buf);
ssize_t floppy_read           (void *handle, void *buffer, uint64_t start, size_t count, int flags);
ssize_t floppy_write          (void *handle, const void *buffer, uint64_t start, size_t count, int flags);
int     floppy_sync           (void *handle);
int     floppy_test_unit_ready(void *handle);


#endif /* #ifndef __FD32FLOPPY_BLOCKIO_H */
