/**************************************************************************
 * FreeDOS32 Floppy Driver                                                *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2003, Salvatore Isaja                                    *
 *                                                                        *
 * This is "fd32.c" - Driver code meaningful only for FD32                *
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
 * along with the FreeDOS32 Floppy Driver; see the file COPYING.txt;       *
 * if not, write to the Free Software Foundation, Inc.,                   *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA                *
 **************************************************************************/
#include <ll/i386/hw-data.h>
#include <ll/i386/error.h>
#include <errno.h>
#include <devices.h>
#include "blockio.h"


/* The following are defined as static arrays to reduce memory fragmentation */
#define MAX_FLOPPIES 4
static const char *dev_names[MAX_FLOPPIES] = { "fd0", "fd1", "fd2", "fd3" };
static Floppy      floppies[MAX_FLOPPIES];


/* FD32 driver request function */
int floppy_request(DWORD function, void *params)
{
    switch (function)
    {
        case FD32_MEDIACHANGE:
        {
            fd32_mediachange_t *x = (fd32_mediachange_t *) params;
            if (x->Size < sizeof(fd32_mediachange_t)) return -EINVAL;
            return (fdc_disk_changed(((Floppy *) x->DeviceId)->fdd) != 0);
        }
        case FD32_BLOCKREAD:
        {
            fd32_blockread_t *x = (fd32_blockread_t *) params;
            if (x->Size < sizeof(fd32_blockread_t)) return -EINVAL;
            return floppy_read((Floppy *) x->DeviceId, x->Start, x->NumBlocks, x->Buffer);
        }
        #ifdef FLOPPY_WRITE
        case FD32_BLOCKWRITE:
        {
            fd32_blockwrite_t *x = (fd32_blockwrite_t *) params;
            if (x->Size < sizeof(fd32_blockwrite_t)) return -EINVAL;
            return floppy_write((Floppy *) x->DeviceId, x->Start, x->NumBlocks, x->Buffer);
        }
        #endif
        case FD32_BLOCKINFO:
        {
            Fdd              *fdd;
            fd32_blockinfo_t *x = (fd32_blockinfo_t *) params;
            if (x->Size < sizeof(fd32_blockinfo_t)) return -EINVAL;
            fdd = ((Floppy *) x->DeviceId)->fdd;
            x->BlockSize   = 512; /* TODO: Make this selectable */
            x->TotalBlocks = fdd->fmt->size;
            x->Type        = FD32_BIFLO;
            /* The following supposes a single FDC is installed */
            x->MultiBootId = 0x00FFFFFF | (fdd->number << 24);
            return 0;
        }
    }
    return -ENOTSUP;
}


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
        res = fd32_dev_register(floppy_request, (void *) &floppies[drive_num], dev_names[drive_num]);
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
