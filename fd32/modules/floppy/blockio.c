/**************************************************************************
 * FreeDOS32 Floppy Driver                                                *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2003,2005, Salvatore Isaja                               *
 *                                                                        *
 * This is "blockio.c" - High level floppy handling and buffered access.  *
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
#include "blockio.h"


#define BUFFERED_READ
//#define __DEBUG__

#ifdef __DEBUG__
  #define LOG_PRINTF(x) fd32_log_printf x
#else
  #define LOG_PRINTF(x)
#endif


/* Bits for Floppy.buffers.flags */
enum
{
    FBF_VALID = 1 << 0, /* Buffered data is valid   */
    FBF_BAD   = 1 << 1  /* Cylinder has bad sectors */
};


/* Converts an absolute sector number (LBA) to CHS using disk geometry.     */
/* Returns 0 on success, or a negative value meaning invalid sector number. */
static inline int lba_to_chs(unsigned lba, Chs *chs, const Fdd *drive)
{
    unsigned temp;
    unsigned cyl_bytes = drive->fmt->heads * drive->fmt->sec_per_trk;

    if (lba >= drive->fmt->tracks * cyl_bytes) return -1;
    chs->c = lba / cyl_bytes;
    temp   = lba % cyl_bytes;
    chs->h = temp / drive->fmt->sec_per_trk;
    chs->s = temp % drive->fmt->sec_per_trk + 1;
    return 0;
}


#ifdef BUFFERED_READ
static int read_sector(Floppy *f, unsigned block, void *buffer)
{
    int res;
    Chs chs;

    /* Use the buffered cylinder when possible */
    if ((f->buffers[0].flags & FBF_VALID)
     && (block >= f->buffers[0].start)
     && (block < f->buffers[0].start + f->fdd->fmt->sec_per_trk * 2))
    {
        if (f->buffers[0].flags & FBF_BAD)
        {
            if (lba_to_chs(block, &chs, f->fdd)) return -1;
            return fdc_read(f->fdd, &chs, buffer, 1);
        }
        memcpy(buffer, &f->buffers[0].data[512 * (block - f->buffers[0].start)], 512);
        return FDC_OK;
    }
    /* If we arrive here, the block is not buffered. */
    /* First try to read a whole cylinder.           */
    if (lba_to_chs(block, &chs, f->fdd)) return -1;
    f->buffers[0].flags = FBF_VALID;
    f->buffers[0].start = chs.c * f->fdd->fmt->heads * f->fdd->fmt->sec_per_trk;
    res = fdc_read_cylinder(f->fdd, chs.c, f->buffers[0].data);
    if (res == FDC_OK)
    {
        memcpy(buffer, &f->buffers[0].data[512 * (block - f->buffers[0].start)], 512);
        return FDC_OK;
    }
    /* If cylinder read fails, mark cylinder buffer as BAD and fall back to
     * single sector read. Mark as VALID as well, so that we will not try to
     * read the whole cylinder again the next time.
     */
    LOG_PRINTF(("[FLOPPY] read_sector: error on track %u, fall back to single sector read!\n", chs.c));
    f->buffers[0].flags |= FBF_BAD;
    return fdc_read(f->fdd, &chs, buffer, 1);
}
#else
static int read_sector(Floppy *f, unsigned block, void *buffer)
{
    int res;
    Chs chs;

    if (lba_to_chs(block, &chs, f->fdd)) return -1;
    LOG_PRINTF(("[FLOPPY] read_sector: C=%u, H=%u, S=%u\n", chs.c, chs.h, chs.s));
    res = fdc_read(f->fdd, &chs, buffer, 1);
    if (res < 0) LOG_PRINTF(("[FLOPPY] read_sector: read error!\n"));
    return res;
}
#endif


/* Reads sectors from a disk into a user buffer.         */
/* Returns 0 on success, or a negative value on failure. */
int floppy_read(Floppy *f, unsigned start, unsigned count, void *buffer)
{
    int      res;
    unsigned k;
    if (fdc_disk_changed(f->fdd))
    {
        LOG_PRINTF(("[FLOPPY] floppy_read: changeline active\n"));
        #ifdef BUFFERED_READ
        f->buffers[0].flags = 0;
        #endif
        res = fdc_log_disk(f->fdd);
        if (res < 0) return res;
    }
    for (k = 0; k < count; k++, start++, buffer += 512)
    {
        res = read_sector(f, start, buffer);
        if (res < 0) return res;
    }
    return k;
}


#ifdef FLOPPY_WRITE
static int write_sector(Floppy *f, unsigned block, const void *buffer)
{
    int res;
    Chs chs;

    #ifdef BUFFERED_READ
    /* If the cylinder is buffered, invalidate the cache */
    if ((block >= f->buffers[0].start) && (block < f->buffers[0].start + f->fdd->fmt->sec_per_trk * 2))
        f->buffers[0].flags = 0;
    #endif
    if (lba_to_chs(block, &chs, f->fdd)) return -1;
    LOG_PRINTF(("[FLOPPY] write_sector: C=%u, H=%u, S=%u\n", chs.c, chs.h, chs.s));
    res = fdc_write(f->fdd, &chs, buffer, 1);
    if (res < 0) LOG_PRINTF(("[FLOPPY] write_sector: write error!\n"));
    return res;
}


/* Writes sectors from a user buffer into disk.          */
/* Returns 0 on success, or a negative value on failure. */
int floppy_write(Floppy *f, unsigned start, unsigned count, const void *buffer)
{
    int      res;
    unsigned k;
    if (fdc_disk_changed(f->fdd))
    {
        LOG_PRINTF(("[FLOPPY] floppy_write: changeline active\n"));
        #ifdef BUFFERED_READ
        f->buffers[0].flags = 0;
        #endif
        res = fdc_log_disk(f->fdd);
        if (res < 0) return res;
    }
    for (k = 0; k < count; k++, start++, buffer += 512)
    {
        res = write_sector(f, start, buffer);
        if (res < 0) return res;
    }
    return k;
}
#endif /* #ifdef FLOPPY_WRITE */
