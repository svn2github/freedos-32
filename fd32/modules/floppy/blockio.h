/**************************************************************************
 * FreeDOS32 Floppy Driver                                                *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2003, Salvatore Isaja                                    *
 *                                                                        *
 * This is "blockio.h" - Header for high level floppy handling.           *
 *                                                                        *
 *                                                                        *
 * This file is part of the FreeDOS32 Floppy Driver.                      *
 *                                                                        *
 * The FreeDOS32 Floppy Driver is free software; you can redistribute     *
 * it and/or modify it under the terms of the GNU General Public License  *
 * as published by the Free Software Foundation; either version 2 of the  *
 * License, or (at your option) any later version.                        *
 *                                                                        *
 * The FreeDOS32 Floppy Driver is distributed in the hope that it will    *
 * be useful, but WITHOUT ANY WARRANTY; without even the implied warranty *
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with the FreeDOS32 Floppy Driver; see the file COPYING.txt;      *
 * if not, write to the Free Software Foundation, Inc.,                   *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA                *
 **************************************************************************/
#ifndef __FD32FLOPPY_BLOCKIO_H
#define __FD32FLOPPY_BLOCKIO_H

#include "fdc.h"


#define FLOPPY_WRITE
#define MAX_BUFFERS 1 /* Number of cacheable cylinders */


/* The Floppy structure is used to add buffered access. */
typedef struct Floppy
{
    Fdd     *fdd;       /* Floppy drive in the low-level driver */
    struct              /* Caches a cylinder of the disk        */
    {
        BYTE    *data;  /* Track data (fdd->fmt->sec_per_trk blocks * 2) */
        unsigned start; /* The first block (LBA) stored in data          */
        unsigned flags; /* See FBF_* enums in floppy.c                   */
    }
    buffers[MAX_BUFFERS];
}
Floppy;


int floppy_read (Floppy *f, unsigned start, unsigned count, void *buffer);
#ifdef FLOPPY_WRITE
int floppy_write(Floppy *f, unsigned start, unsigned count, const void *buffer);
#endif


#endif /* #ifndef __FD32FLOPPY_BLOCKIO_H */

