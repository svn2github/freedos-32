/**************************************************************************
 * FreeDOS32 Floppy Driver                                                *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2003, Salvatore Isaja                                    *
 *                                                                        *
 * This is "fdc.h" - Header file for portable code for FDC support        *
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

#ifndef __FD32FLOPPY_FDC_H
#define __FD32FLOPPY_FDC_H

#include <dr-env.h>


/* Error codes for FDC routines */
enum
{
    FDC_OK      =  0, /* successful completion */
    FDC_ERROR   = -1, /* abnormal termination  */
    FDC_NODISK  = -2, /* no disk in drive      */
    FDC_TIMEOUT = -3, /* timed out             */
};


/* Transfer opcode for the fdc_rw function */
typedef enum
{
    FDC_WRITE = 0,
    FDC_READ  = 1
}
FdcTransfer;


/* A Cylinder/Head/Sector address */
typedef struct
{
    unsigned c, h, s;
}
Chs;


/* The format of a floppy disk. */
typedef struct
{
    unsigned    size;        /* Total number of sectors on disk    */
    unsigned    sec_per_trk; /* Number of sectors per track        */
    unsigned    heads;       /* Number of heads (sides)            */
    unsigned    tracks;      /* Number of tracks                   */
    unsigned    strech;      /* 1 if tracks are to be doubled      */
    BYTE        gap3;        /* GAP3 to use for reading/writing    */
    BYTE        rate;        /* Sector size and data transfer rate */
    BYTE        sr_hut;      /* Step Rate and Head Unload Time     */
    BYTE        gap3fmt;     /* GAP3 to use for formatting         */
    const char *name;        /* Disk format name                   */
}
FloppyFormat;


/* Parameters used to handle a floppy drive. */
typedef struct
{
    unsigned    cmos_type;  /* Drive type read from CMOS            */
    unsigned    hlt;        /* Head Load Time (in SPECIFY format)   */
    unsigned    spin_up;    /* Time needed for spinning up [ms]     */
    unsigned    spin_down;  /* Time needed for spinning down [ms]   */
    unsigned    sel_delay;  /* Select delay [ms]                    */
    unsigned    int_tmout;  /* Interrupt timeout [ms]               */
    unsigned    detect[8];  /* Autodetect formats                   */
    unsigned    native_fmt; /* Native disk format for drive         */
    const char *name;       /* Name displayed for the drive type    */
}
DriveParams;


/* Status structure for a Floppy Disk Drive */
typedef struct
{
    struct Fdc          *fdc;       /* Controller of this drive (see fdc.c)  */
    const  DriveParams  *dp;        /* Parameters for this drive             */
    const  FloppyFormat *fmt;       /* Current format of the floppy          */
    unsigned             number;    /* Drive number per FDC [0..MAXDRIVES-1] */
    volatile unsigned    flags;     /* See the DF_* enums in "fdc.c"         */
    unsigned             track;     /* Current floppy track sought           */
    int                  spin_down; /* Event handle for motor spindown       */
}
Fdd;


/* Callback function for FDC initialization.            */
/* A function that is called as each drive is detected. */
typedef int FdcSetupCallback(Fdd *fdd);


/* Function prototypes */
int  fdc_seek          (Fdd *fdd, unsigned track);
int  fdc_disk_changed  (Fdd *fdd);
int  fdc_log_disk      (Fdd *fdd);
int  fdc_read          (Fdd *fdd, const Chs *chs, BYTE *buffer, unsigned num_sectors);
int  fdc_read_cylinder (Fdd *fdd, unsigned cyl, BYTE *buffer);
int  fdc_write         (Fdd *fdd, const Chs *chs, const BYTE *buffer, unsigned num_sectors);
int  fdc_write_cylinder(Fdd *fdd, unsigned cyl, const BYTE *buffer);
int  fdc_setup         (FdcSetupCallback *setup_cb);
void fdc_dispose       (void);


#endif /* #ifndef __FD32FLOPPY_FDC_H */

