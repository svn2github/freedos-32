/**************************************************************************
 * FreeDOS32 Floppy Driver                                                *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2003, Salvatore Isaja                                    *
 *                                                                        *
 * This is "fdc.c" - Portable code for Floppy Disk Controller support     *
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
 *                                                                        *
 * ACKNLOWLEDGEMENT                                                       *
 * This file is derived from fdc.c, floppy controller handler functions   *
 * Copyright (C) 1998  Fabian Nunez                                       *
 * You can download the original library from Cottontail OS Development   *
 * Library <http://www.0xfi.com/oslib/topx.html>, the file is FDC.ZIP     *
 * The author can be reached by email at: fabian@cs.uct.ac.za             *
 * or by airmail at: Fabian Nunez                                         *
 *                   10 Eastbrooke                                        *
 *                   Highstead Road                                       *
 *                   Rondebosch 7700                                      *
 *                   South Africa                                         *
 * Floppy formats and drive parameters table are from the Linux driver    *
 *  linux/kernel/floppy.c                                                 *
 *  Copyright (C) 1991, 1992  Linus Torvalds                              *
 *  Copyright (C) 1993, 1994  Alain Knaff                                 *
 **************************************************************************/

#include "fdc.h"


#define MAXDRIVES 2 /* Newer controllers don't support 4 drives, just 2 */
#define SAFESEEK  5 /* Track to seek before issuing a recalibrate       */
//#define HAS2FDCS
//#define __DEBUG__

#ifdef __DEBUG__
  #define LOG_PRINTF(x) fd32_log_printf x
#else
  #define LOG_PRINTF(x)
#endif


/* This is called when fdc_logdisk is able to read sectors from the new disk. */
/* It fine-tunes format probing by reading informations in the boot sector.   */
extern int floppy_bootsector(Fdd *fdd, const FloppyFormat *formats, unsigned num_formats);


/* Floppy Drive Controller IO ports */
enum
{
    FDC_BPRI = 0x3F0, /* Base port of the primary controller   */
    FDC_BSEC = 0x370, /* Base port of the secondary controller */
    FDC_DOR  = 0x002, /* RW: Digital Output Register           */
    FDC_MSR  = 0x004, /* R : Main Status Register              */
    FDC_DRS  = 0x004, /* W : Data Rate Select Register         */
    FDC_DATA = 0x005, /* RW: Data Register                     */
    FDC_DIR  = 0x007, /* R : Digital Input Register            */
    FDC_CCR  = 0x007  /* W : Configuration Control Register    */
};


/* Command bytes (these are NEC765 commands + options such as MFM, etc) */
enum
{
    CMD_SPECIFY = 0x03, /* Specify drive timings   */
    CMD_WRITE   = 0xC5, /* Write data (+ MT,MFM)   */
    CMD_READ    = 0xE6, /* Read data (+ MT,MFM,SK) */
    CMD_SENSEI  = 0x08, /* Sense interrupt status  */
    //CMD_FORMAT  = 0x4D, /* Format track (+ MFM)    */
    CMD_READID  = 0x4A, /* Read sector Id (+ MFM)  */
    CMD_RECAL   = 0x07, /* Recalibrate             */
    CMD_SEEK    = 0x0F, /* Seek track              */
    CMD_VERSION = 0x10  /* Get FDC version         */
};


/* Bits for Fdd.flags */
enum
{
    DF_CHANGED = 1 << 0, /* Disk has been changed during the last command */
    DF_SPINUP  = 1 << 1, /* Motor spinup time elapsed, ready to transfer  */
    DF_SPINDN  = 1 << 2  /* Motor spindown time started                   */
};


/* Status structure for a Floppy Disk Controller */
typedef struct Fdc
{
    unsigned base_port;        /* Base port for this controller: XXX0h      */
    BYTE     result[7];        /* Stores the result bytes returned          */
    BYTE     result_size;      /* Number of result bytes returned           */
    BYTE     sr0;              /* Status Register 0 after a sense interrupt */
    BYTE     dor;              /* Reflects the Digital Output Register      */
    Fdd      drive[MAXDRIVES]; /* Drives connected to this controller       */
}
Fdc;


/* All Floppy Drive Controllers issue an IRQ6 to report command completion. */
/* The irq_signaled variable let us know when IRQ6 happened.                */
/* Since it is a global variable, this makes the driver not reentrant, thus */
/* we use a simple busy flag to protect the critical sections (all... :-)   */
/* For the same reason, we can use just a single, shared, DMA buffer.       */
//static IrqSave      old_irq6;         /* Saves the old IRQ6 handler   */
/* TODO: At present we have no means to save the old IRQ6 handler */
static volatile int busy         = 0; /* Set if the driver is busy    */
static volatile int irq_signaled = 0; /* Set if IRQ has been signaled */
static LOWMEM_ADDR  dma_sel;          /* Selector/address of DMA buf  */
static DWORD        dma_addr;         /* Physical address of DMA buf  */
static Fdc          pri_fdc;          /* Status of the primary FDC    */
#ifdef HAS2FDCS
static Fdc          sec_fdc;          /* Status of the secondary FDC  */
#endif


/* Geometry and other format specifications for floppy disks */
static const FloppyFormat floppy_formats[32] =
{
  /*  SIZE SPT HD TRK STR GAP3  RATE SRHUT GAP3F  NAME          NR DESCRIPTION   */
    {    0,  0, 0,  0, 0, 0x00, 0x00, 0x00, 0x00, NULL    }, /*  0 no testing    */
    {  720,  9, 2, 40, 0, 0x2A, 0x02, 0xDF, 0x50, "d360"  }, /*  1 360KB PC      */
    { 2400, 15, 2, 80, 0, 0x1B, 0x00, 0xDF, 0x54, "h1200" }, /*  2 1.2MB AT      */
    {  720,  9, 1, 80, 0, 0x2A, 0x02, 0xDF, 0x50, "D360"  }, /*  3 360KB SS 3.5" */
    { 1440,  9, 2, 80, 0, 0x2A, 0x02, 0xDF, 0x50, "D720"  }, /*  4 720KB 3.5"    */
    {  720,  9, 2, 40, 1, 0x23, 0x01, 0xDF, 0x50, "h360"  }, /*  5 360KB AT      */
    { 1440,  9, 2, 80, 0, 0x23, 0x01, 0xDF, 0x50, "h720"  }, /*  6 720KB AT      */
//    { 2880, 18, 2, 80, 0, 0x1B, 0x00, 0xDF, 0x6C, "H1440" }, /*  7 1.44MB 3.5"   */
    { 2880, 18, 2, 80, 0, 0x1B, 0x00, 0xCF, 0x6C, "H1440" }, /*  7 1.44MB 3.5"   */
    { 5760, 36, 2, 80, 0, 0x1B, 0x43, 0xAF, 0x54, "E2880" }, /*  8 2.88MB 3.5"   */
    { 6240, 39, 2, 80, 0, 0x1B, 0x43, 0xAF, 0x28, "E3120" }, /*  9 3.12MB 3.5"   */

    { 2880, 18, 2, 80, 0, 0x25, 0x00, 0xDF, 0x02, "h1440" }, /* 10 1.44MB 5.25"  */
    { 3360, 21, 2, 80, 0, 0x1C, 0x00, 0xCF, 0x0C, "H1680" }, /* 11 1.68MB 3.5"   */
    {  820, 10, 2, 41, 1, 0x25, 0x01, 0xDF, 0x2E, "h410"  }, /* 12 410KB 5.25"   */
    { 1640, 10, 2, 82, 0, 0x25, 0x02, 0xDF, 0x2E, "H820"  }, /* 13 820KB 3.5"    */
    { 2952, 18, 2, 82, 0, 0x25, 0x00, 0xDF, 0x02, "h1476" }, /* 14 1.48MB 5.25"  */
    { 3444, 21, 2, 82, 0, 0x25, 0x00, 0xDF, 0x0C, "H1722" }, /* 15 1.72MB 3.5"   */
    {  840, 10, 2, 42, 1, 0x25, 0x01, 0xDF, 0x2E, "h420"  }, /* 16 420KB 5.25"   */
    { 1660, 10, 2, 83, 0, 0x25, 0x02, 0xDF, 0x2E, "H830"  }, /* 17 830KB 3.5"    */
    { 2988, 18, 2, 83, 0, 0x25, 0x00, 0xDF, 0x02, "h1494" }, /* 18 1.49MB 5.25"  */
    { 3486, 21, 2, 83, 0, 0x25, 0x00, 0xDF, 0x0C, "H1743" }, /* 19 1.74MB 3.5"   */

    { 1760, 11, 2, 80, 0, 0x1C, 0x09, 0xCF, 0x00, "h880"  }, /* 20 880KB 5.25"   */
    { 2080, 13, 2, 80, 0, 0x1C, 0x01, 0xCF, 0x00, "D1040" }, /* 21 1.04MB 3.5"   */
    { 2240, 14, 2, 80, 0, 0x1C, 0x19, 0xCF, 0x00, "D1120" }, /* 22 1.12MB 3.5"   */
    { 3200, 20, 2, 80, 0, 0x1C, 0x20, 0xCF, 0x2C, "h1600" }, /* 23 1.6MB 5.25"   */
    { 3520, 22, 2, 80, 0, 0x1C, 0x08, 0xCF, 0x2e, "H1760" }, /* 24 1.76MB 3.5"   */
    { 3840, 24, 2, 80, 0, 0x1C, 0x20, 0xCF, 0x00, "H1920" }, /* 25 1.92MB 3.5"   */
    { 6400, 40, 2, 80, 0, 0x25, 0x5B, 0xCF, 0x00, "E3200" }, /* 26 3.20MB 3.5"   */
    { 7040, 44, 2, 80, 0, 0x25, 0x5B, 0xCF, 0x00, "E3520" }, /* 27 3.52MB 3.5"   */
    { 7680, 48, 2, 80, 0, 0x25, 0x63, 0xCF, 0x00, "E3840" }, /* 28 3.84MB 3.5"   */

    { 3680, 23, 2, 80, 0, 0x1C, 0x10, 0xCF, 0x00, "H1840" }, /* 29 1.84MB 3.5"   */
    { 1600, 10, 2, 80, 0, 0x25, 0x02, 0xDF, 0x2E, "D800"  }, /* 30 800KB 3.5"    */
    { 3200, 20, 2, 80, 0, 0x1C, 0x00, 0xCF, 0x2C, "H1600" }  /* 31 1.6MB 3.5"    */
};


/* Parameters to manage a floppy disk drive.                                */
/* Head load time is 16 ms for all drives except 2880 KiB, that have 15 ms. */
static const DriveParams default_drive_params[] =
{
   /* T HLT  SPUP  SPDN SEL  INTT  AUTODETECT FORMATS        NAT  NAME                     */
    { 0,  0, 1000, 3000, 20, 3000, { 7, 4, 8, 2, 1, 5, 3,10 }, 0, "unknown"                },
    { 1,  4, 1000, 3000, 20, 3000, { 1, 0, 0, 0, 0, 0, 0, 0 }, 1, "5.25\" DD, 360 KiB"     },
    { 2,  8,  400, 3000, 20, 3000, { 2, 5, 6,23,10,20,12, 0 }, 2, "5.25\" HD, 1200 KiB"    },
    { 3,  4, 1000, 3000, 20, 3000, { 4,22,21,30, 3, 0, 0, 0 }, 4, "3.5\" DD, 720 KiB"      },
//    { 4,  1,  400, 3000, 20, 1500, { 7, 4,25,22,31,21,29,11 }, 7, "3.5\" HD, 1440 KiB"     },
    { 4,  8,  400, 3000, 20, 1500, { 7, 4,25,22,31,21,29,11 }, 7, "3.5\" HD, 1440 KiB"     },
    { 5, 15,  400, 3000, 20, 3000, { 7, 8, 4,25,28,22,31,21 }, 8, "3.5\" ED, 2880 KiB AMI" },
    { 6, 15,  400, 3000, 20, 3000, { 7, 8, 4,25,28,22,31,21 }, 8, "3.5\" ED, 2880 KiB"     }
};


/* Constants for DMA transfers */
enum
{
    DMA_CHANNEL = 2,    /* Number of the DMA channel for floppy transfer */
    DMA_PAGE    = 0x81, /* Page register of that DMA channel             */
    DMA_OFFSET  = 0x04, /* Offset register of that DMA channel           */
    DMA_LENGTH  = 0x05  /* Length register of that DMA channel           */
};


/* Sets up a DMA trasfer between a device and memory.              */
/* If 'read' is TRUE, then transfer will be from memory to device, */
/* else from the device to memory.                                 */
static void dma_xfer(DWORD physaddr, unsigned length, int read)
{
    DWORD page, offset;

    /* Calculate DMA page and offset */
    page    = physaddr >> 16;
    offset  = physaddr & 0xffff;
    length -= 1; /* with DMA, if you want k bytes, you ask for k - 1 */

    fd32_cli();                                          /* Disable IRQs            */
    fd32_outb(0x0A, DMA_CHANNEL | 4);                    /* Set channel mask bit    */
    fd32_outb(0x0C, 0);                                  /* Clear flip flop         */
    fd32_outb(0x0B, (read ? 0x48 : 0x44) + DMA_CHANNEL); /* Mode (write+single+r/w) */
    fd32_outb(DMA_PAGE, page);                           /* Page                    */
    fd32_outb(DMA_OFFSET, offset & 0xFF);                /* Offset: low byte        */
    fd32_outb(DMA_OFFSET, offset >> 8);                  /* Offset: high byte       */
    fd32_outb(DMA_LENGTH, length & 0xFF);                /* Length: low byte        */
    fd32_outb(DMA_LENGTH, length >> 8);                  /* Length: high byte       */
    fd32_outb(0x0A, DMA_CHANNEL);                        /* Clear DMA mask bit      */
    fd32_sti();                                          /* enable IRQs             */
}


/* Sends a byte to the controller FIFO. */
/* Returns FDC_TIMEOUT on timeout.      */
static int sendbyte(unsigned base_port, unsigned byte)
{
    volatile unsigned msr;
    unsigned          tmo;

    for (tmo = 0; tmo < 128; tmo++)
    {
        msr = fd32_inb(base_port + FDC_MSR);
        if ((msr & 0xC0) == 0x80)
        {
            fd32_outb(base_port + FDC_DATA, byte);
            return FDC_OK;
        }
        fd32_inb(0x80); /* delay */
    }
    return FDC_TIMEOUT; /* write timeout */
}


/* Gets a byte from the controller FIFO. */
/* Returns FDC_TIMEOUT on timeout.       */
static int getbyte(unsigned base_port)
{
    volatile unsigned msr;
    unsigned          tmo;

    for (tmo = 0; tmo < 128; tmo++)
    {
        msr = fd32_inb(base_port + FDC_MSR);
        if ((msr & 0xD0) == 0xD0) return fd32_inb(base_port + FDC_DATA);
        fd32_inb(0x80); /* delay */
    }
    return FDC_TIMEOUT; /* read timeout */
}


/* Callback function called on IRQ6 timeout */
static void irq_timeout_cb(void *params)
{
    *((int *) params) = 1; /* Signal timeout */
}


/* Waits for FDC command to complete. Returns FDC_TIMEOUT on time out. */
static int wait_fdc(Fdd *fdd)
{
    /* Wait for IRQ6 handler to signal command finished */
    volatile int irq_timeout = 0;
    int irq_timeout_event = fd32_event_post(fdd->dp->int_tmout * 1000, irq_timeout_cb, (void *) &irq_timeout);
    /* TODO: Check for FD32_EVENT_NULL */
    WFC(!irq_signaled && !irq_timeout);
    fd32_event_delete(irq_timeout_event);

    /* Read in command result bytes while controller is busy */
    fdd->fdc->result_size = 0;
    while ((fdd->fdc->result_size < 7) && (fd32_inb(fdd->fdc->base_port + FDC_MSR) & 0x10))
        fdd->fdc->result[fdd->fdc->result_size++] = getbyte(fdd->fdc->base_port);

    irq_signaled = 0;
    if (irq_timeout)
    {
        LOG_PRINTF(("[FDC] Timeout!\n"));
        return FDC_TIMEOUT;
    }
    #if 0
    {
        unsigned i;
        LOG_PRINTF(("Result bytes: "));
        for (i = 0; i < fdd->fdc->result_size; i++)
            LOG_PRINTF(("%u(%02xh) ", fdd->fdc->result[i], fdd->fdc->result[i]));
        LOG_PRINTF(("\n"));
    }
    #endif
    return FDC_OK;
}


/* Returns nonzero if the motor of the specified drive is on */
static inline int is_motor_on(Fdc *fdc, unsigned drive)
{
    return fdc->dor & (1 << (drive + 4));
}


/* Turns the motor off after the spindown time */
static void motor_off_cb(void *params)
{
    Fdd *fdd = (Fdd *) params;
    if (is_motor_on(fdd->fdc, fdd->number))
    {
        fdd->fdc->dor &= ~(1 << (fdd->number + 4));
        fd32_outb(fdd->fdc->base_port + FDC_DOR, fdd->fdc->dor);
        fdd->flags &= ~(DF_SPINUP | DF_SPINDN);
    }
}


/* Signals that motor spinup time elapsed */
static void motor_spin_cb(void *fdd)
{
    ((Fdd *) fdd)->flags |= DF_SPINUP;
}


/* Turns the motor on and selects the drive */
static void motor_on(Fdd *fdd)
{
    if (fdd->flags & DF_SPINDN) fd32_event_delete(fdd->spin_down);
    fdd->flags   &= ~DF_SPINDN;
    if (!is_motor_on(fdd->fdc, fdd->number))
    {
        if (!(fdd->flags & DF_SPINUP))
            /* TODO: Check for FD32_EVENT_NULL */
            if (fd32_event_post(fdd->dp->spin_up * 1000, motor_spin_cb, fdd) < 0)
                LOG_PRINTF(("Motor on: Out of events!\n"));
        fdd->fdc->dor |= (1 << (fdd->number + 4));
        /* Select drive */
        fdd->fdc->dor = (fdd->fdc->dor & 0xFC) | fdd->number;
        fd32_outb(fdd->fdc->base_port + FDC_DOR, fdd->fdc->dor);
    }
}


/* Schedule motor off setting up the spindown timer */
static void motor_down(Fdd *fdd)
{
    if (is_motor_on(fdd->fdc, fdd->number))
    {
        if (fdd->flags & DF_SPINDN) return;
        fdd->spin_down = fd32_event_post(fdd->dp->spin_down * 1000, motor_off_cb, fdd);
        if (fdd->spin_down == -1) LOG_PRINTF(("[FDC] motor_down: out of events!\n"));
        fdd->flags |= DF_SPINDN;
    }
}


/* This is the IRQ6 handler. irq_signaled is global! */
static void irq6(int n)
{
    irq_signaled = 1;  /* Signal operation finished */
    fd32_master_eoi(); /* Send EOI the PIC          */
}


/* Returns nonzero if there was a disk change */
int fdc_disk_changed(Fdd *fdd)
{
    unsigned changeline;
    motor_on(fdd);
    changeline = fd32_inb(fdd->fdc->base_port + FDC_DIR) & 0x80;
    motor_down(fdd);
    return changeline || (fdd->flags & DF_CHANGED);
}


/* Recalibrates a drive (seek head to track 0).                     */
/* Since the RECALIBRATE command sends up to 79 pulses to the head, */
/* this function issues as many RECALIBRATE as needed.              */
/* The drive is supposed to be selected (motor on).                 */
static void recalibrate(Fdd *fdd)
{
    unsigned k;
    LOG_PRINTF(("Recalibrate\n"));
    for (k = 0; k < 13; k++)
    {
        sendbyte(fdd->fdc->base_port, CMD_RECAL);
        sendbyte(fdd->fdc->base_port, fdd->number);
        wait_fdc(fdd);
        /* Send a "sense interrupt status" command */
        sendbyte(fdd->fdc->base_port, CMD_SENSEI);
        fdd->fdc->sr0 = getbyte(fdd->fdc->base_port);
        fdd->track    = getbyte(fdd->fdc->base_port);
        if (!(fdd->fdc->sr0 & 0x10)) break; /* Exit if Unit Check is not set */
    }
    LOG_PRINTF(("Calibration result on drive %u: SR0=%02X, Track=%u\n",
                fdd->number, fdd->fdc->sr0, fdd->track));
}


/* Seeks a drive to the specified track.            */
/* The drive is supposed to be selected (motor on). */
int fdc_seek(Fdd *fdd, unsigned track)
{
    if (fdd->track == track) return FDC_OK; /* Already there */
    if (track >= fdd->fmt->tracks) return FDC_ERROR; /* Invalid track */
    sendbyte(fdd->fdc->base_port, CMD_SEEK);
    sendbyte(fdd->fdc->base_port, fdd->number);
    sendbyte(fdd->fdc->base_port, track);
    if (wait_fdc(fdd)) return FDC_TIMEOUT; /* Timeout */
    /* Send a "sense interrupt status" command */
    sendbyte(fdd->fdc->base_port, CMD_SENSEI);
    fdd->fdc->sr0 = getbyte(fdd->fdc->base_port);
    fdd->track    = getbyte(fdd->fdc->base_port);
    /* Check that seek worked */
    if ((fdd->fdc->sr0 != 0x20 + fdd->number) || (fdd->track != track))
    {
        LOG_PRINTF(("Seek error on drive %u: SR0=%02X, Track=%u Expected=%u\n",
                    fdd->number, fdd->fdc->sr0, fdd->track, track));
        return FDC_ERROR;
    }
    LOG_PRINTF(("Seek result on drive %u: SR0=%02X, Track=%u\n", fdd->number,
                fdd->fdc->sr0, fdd->track));
    return FDC_OK;
}


/* Gets the position of the heads by reading the next sector identifier. */
/* Fills chs with the current head position.                             */
/* The drive is supposed to be selected (motor on).                      */
static int readid(Fdd *fdd, Chs *chs)
{
    int res = 0;
    sendbyte(fdd->fdc->base_port, CMD_READID);
    sendbyte(fdd->fdc->base_port, fdd->number);
    if (wait_fdc(fdd)) res = FDC_TIMEOUT;
    if (res < 0) return res;
    if ((fdd->fdc->result[0] & 0xC0) != 0) return FDC_ERROR;
    chs->c = fdd->fdc->result[3];
    chs->h = fdd->fdc->result[4];
    chs->s = fdd->fdc->result[5];
    return FDC_OK;
}


/* Program data rate and specify drive timings using the SPECIFY command */
static void specify(const Fdd *fdd)
{
    fd32_outb(fdd->fdc->base_port + FDC_CCR, fdd->fmt->rate);
    sendbyte(fdd->fdc->base_port, CMD_SPECIFY);
    sendbyte(fdd->fdc->base_port, fdd->fmt->sr_hut);
    sendbyte(fdd->fdc->base_port, fdd->dp->hlt << 1); /* Always DMA */
}


/* Resets the FDC to a known state */
static void reset_fdc(Fdc *fdc)
{
    unsigned      k;
    volatile int  irq_timeout = 0;
    int irq_timeout_event;

    fd32_outb(fdc->base_port + FDC_DOR, 0);    /* Stop the motor and disable IRQ/DMA  */
    fd32_outb(fdc->base_port + FDC_DOR, 0x0C); /* Re-enable IRQ/DMA and release reset */
    fdc->dor = 0x0C;
    /* Resetting triggered an interrupt - handle it */
    irq_timeout_event = fd32_event_post(default_drive_params[0].int_tmout * 1000, irq_timeout_cb, (void *) &irq_timeout);
    /* TODO: Check for FD32_EVENT_NULL */
    WFC(!irq_signaled && !irq_timeout);
    fd32_event_delete(irq_timeout_event);
    irq_signaled = 0;
    if (irq_timeout)
        LOG_PRINTF(("Timed out while waiting for FDC after reset\n"));
    /* FDC specs say to sense interrupt status four times */
    for (k = 0; k < 4; k++)
    {
        /* Send a "sense interrupt status" command */
        sendbyte(fdc->base_port, CMD_SENSEI);
        fdc->sr0            = getbyte(fdc->base_port);
        fdc->drive[k].track = getbyte(fdc->base_port);
    }
}

    
/* Get a drive to a known state */
static void reset_drive(Fdd *fdd)
{
    if (fdd->dp->cmos_type == 0) return;
    if (fdd->flags & DF_SPINDN) fd32_event_delete(fdd->spin_down);
    fdd->flags   = 0;
    fdd->track   = 0;
    specify(fdd);
    motor_on(fdd);
    fdc_seek(fdd, SAFESEEK);
    recalibrate(fdd);
    motor_down(fdd);
    fdd->flags |= DF_CHANGED; /* So that fdc_log_disk can be issued */
}


/* Transfer sectors between the disk and the DMA buffer. */
/* The drive is supposed to be selected (motor on).      */
static int fdc_xfer(Fdd *fdd, const Chs *chs, DWORD dma_addr, unsigned num_sectors, FdcTransfer op)
{
    /* Wait for motor spin quickly enough */
    WFC(!(fdd->flags & DF_SPINUP));

    /* Send read/write command */
    if (op == FDC_READ)
    {
        dma_xfer(dma_addr, 512 * num_sectors, 0); /* Read */
        sendbyte(fdd->fdc->base_port, CMD_READ);
    }
    else
    {
        dma_xfer(dma_addr, 512 * num_sectors, 1); /* Write */
        sendbyte(fdd->fdc->base_port, CMD_WRITE);
    }
    sendbyte(fdd->fdc->base_port, (chs->h << 2) | fdd->number);
    sendbyte(fdd->fdc->base_port, chs->c);
    sendbyte(fdd->fdc->base_port, chs->h);
    sendbyte(fdd->fdc->base_port, chs->s);
    sendbyte(fdd->fdc->base_port, 2); /* 512 bytes per sector */
    sendbyte(fdd->fdc->base_port, fdd->fmt->sec_per_trk);
    sendbyte(fdd->fdc->base_port, fdd->fmt->gap3);
    sendbyte(fdd->fdc->base_port, 0xFF); /* DTL (bytes to transfer) = unused */

    /* Wait for completion and produce exit code */
    if (wait_fdc(fdd))
    {
        reset_fdc(fdd->fdc);
        reset_drive(fdd);
        return FDC_TIMEOUT;
    }
    if ((fdd->fdc->result[0] & 0xC0) == 0) return FDC_OK;
    return FDC_ERROR;
}


/* Reads sectors from the floppy, up to the end of the cylinder.         */
/* If buffer is NULL, the read data is discarded (may be used to probe). */
int fdc_read(Fdd *fdd, const Chs *chs, BYTE *buffer, unsigned num_sectors)
{
    unsigned tries;
    int      res = FDC_ERROR;

    if (fdd->dp->cmos_type == 0) return FDC_ERROR; /* Drive not available */
    /* TODO: Place a timeout for Busy check */
    while (busy); /* Wait while the floppy driver is already busy: BUSY WAIT! */
    busy = 1;
    motor_on(fdd);
    specify(fdd);
    for (tries = 0; tries < 3; tries++)
    {
        /* Move head to right track */
        if (fdc_seek(fdd, chs->c) == FDC_OK)
        {
            /* If changeline is active, no disk is in drive */
            if (fd32_inb(fdd->fdc->base_port + FDC_DIR) & 0x80)
            {
                LOG_PRINTF(("[FDC] fdc_read: no disk in drive\n"));
                res = FDC_NODISK;
                break;
            }
            res = fdc_xfer(fdd, chs, dma_addr, num_sectors, FDC_READ);
            if (res == FDC_OK) break;
        }
        else res = FDC_ERROR; /* Seek error */
        if (tries != 2) recalibrate(fdd); /* Error, try again... */
    }
    /* Copy data from the DMA buffer into the caller's buffer */
    if ((res == FDC_OK) && buffer)
        fd32_memcpy_from_lowmem(buffer, dma_sel, 0, 512 * num_sectors);
    motor_down(fdd);
    busy = 0;
    return res;
}


/* Writes sectors to the floppy, up to the end of the cylinder. */
int fdc_write(Fdd *fdd, const Chs *chs, const BYTE *buffer, unsigned num_sectors)
{
    unsigned tries;
    int      res = FDC_ERROR;

    if (fdd->dp->cmos_type == 0) return FDC_ERROR; /* Drive not available */
    /* TODO: Place a timeout for Busy check */
    while (busy); /* Wait while the floppy driver is already busy: BUSY WAIT! */
    busy = 1;
    motor_on(fdd);
    specify(fdd);
    fd32_memcpy_to_lowmem(dma_sel, 0, buffer, 512 * num_sectors);
    for (tries = 0; tries < 3; tries++)
    {
        /* Move head to right track */
        if (fdc_seek(fdd, chs->c) == FDC_OK)
        {
            /* If changeline is active, no disk is in drive */
            if (fd32_inb(fdd->fdc->base_port + FDC_DIR) & 0x80)
            {
                LOG_PRINTF(("[FDC] fdc_write: no disk in drive\n"));
                res = FDC_NODISK;
                break;
            }
            res = fdc_xfer(fdd, chs, dma_addr, num_sectors, FDC_WRITE);
            if (res == FDC_OK) break;
        }
        else res = FDC_ERROR; /* Seek error */
        if (tries != 2) recalibrate(fdd); /* Error, try again... */
    }
    motor_down(fdd);
    busy = 0;
    return res;
}


/* Transfers data between a whole cylinder (both tracks of disk sides) and */
/* and the DMA buffer, as fast as possible.                                */
/* We get the current head position with READID, so we can start to        */
/* transfer sectors from the first sector passing below the head. When the */
/* disk completes a revolution, we finish transferring the first sectors.  */
/* If buffer is NULL, the read data is discarded (may be used to probe).   */
static int fdc_xfer_cylinder(Fdd *fdd, unsigned cyl, FdcTransfer op)
{
    unsigned tries;               /* Count how many retries              */
    Chs      chs = { cyl, 0, 1 }; /* Passed to fdc_xfer to read sectors  */
    Chs      cur;                 /* Current position returned by READID */
    unsigned sec_in_cyl;          /* Zero-based sector we start to read  */
    int      res = FDC_ERROR;     /* This function's result...           */

    motor_on(fdd);
    specify(fdd);
    for (tries = 0; tries < 3; tries++)
    {
        /* Move head to right track */
        if (fdc_seek(fdd, cyl) == FDC_OK)
        {
            /* If changeline is active, no disk is in drive */
            if (fd32_inb(fdd->fdc->base_port + FDC_DIR) & 0x80)
            {
                LOG_PRINTF(("[FDC] fdc_Xfer_cylinder: no disk in drive\n"));
                res = FDC_NODISK;
                break;
            }
            readid(fdd, &cur);
            LOG_PRINTF(("[FDC] fdc_xfer_cylinder: readid=%u,%u,%u\n", cur.c, cur.h, cur.s));
            /* Transfer from the current sector of head 0 to the end of cylinder */
            cur.s++; /* The sector we read the Id is gone. Advance a bit */
            sec_in_cyl = cur.s - 1;
            if (cur.s > fdd->fmt->sec_per_trk)
            {
                cur.h++;
                cur.s -= fdd->fmt->sec_per_trk;
            }
            chs.h = cur.h;
            chs.s = cur.s;
            LOG_PRINTF(("[FDC] fdc_xfer_cylinder: first round:%u sectors from %u\n",
                        fdd->fmt->sec_per_trk * 2 - sec_in_cyl, sec_in_cyl));
            res = fdc_xfer(fdd, &chs, dma_addr + 512 * sec_in_cyl,
                           fdd->fmt->sec_per_trk * 2 - sec_in_cyl, op);
            /* Read the first sectors, that we missed in the previous revolution */
            if ((res == FDC_OK) && (sec_in_cyl > 0))
            {
                chs.h = 0;
                chs.s = 1;
                LOG_PRINTF(("[FDC] fdc_read_cylinder: second round:%u sectors from 0\n", sec_in_cyl));
                res = fdc_xfer(fdd, &chs, dma_addr, sec_in_cyl, op);
            }
            if (res == FDC_OK) break;
        }
        else res = FDC_ERROR; /* Seek error */
        if (tries != 2) recalibrate(fdd); /* Error, try again... */
    }
    motor_down(fdd);
    return res;
}


/* Reads a whole cylinder (both tracks of disk sides) as fast as possible. */
/* See fdc_xfer_cylinder for comments.                                     */
/* If buffer is NULL, the read data is discarded (may be used to probe).   */
int fdc_read_cylinder(Fdd *fdd, unsigned cyl, BYTE *buffer)
{
    int res;
    if (fdd->dp->cmos_type == 0) return FDC_ERROR; /* Drive not available */
    /* TODO: Place a timeout for Busy check */
    while (busy); /* Wait while the floppy driver is already busy: BUSY WAIT! */
    busy = 1;
    res = fdc_xfer_cylinder(fdd, cyl, FDC_READ);
    if ((res == FDC_OK) && buffer)
        fd32_memcpy_from_lowmem(buffer, dma_sel, 0, 512 * fdd->fmt->sec_per_trk * 2);
    busy = 0;
    return res;
}


/* Writes a whole cylinder (both tracks of disk sides) as fast as possible. */
/* See fdc_read_cylinder for comments.                                      */
int fdc_write_cylinder(Fdd *fdd, unsigned cyl, const BYTE *buffer)
{
    int res;
    if (fdd->dp->cmos_type == 0) return FDC_ERROR; /* Drive not available */
    /* TODO: Place a timeout for Busy check */
    while (busy); /* Wait while the floppy driver is already busy: BUSY WAIT! */
    busy = 1;
    fd32_memcpy_to_lowmem(dma_sel, 0, buffer, 512 * fdd->fmt->sec_per_trk * 2);
    res = fdc_xfer_cylinder(fdd, cyl, FDC_WRITE);
    busy = 0;
    return res;
}


/* Ckecks for the specified disk format, by reading the last sector of the */
/* last track of the last side of the specified format.                    */
/* Returns zero if format is supported, or a negative value if not.        */
static int probe_format(Fdd *fdd, unsigned format)
{
    Chs chs = { 0, 0, 0 };
    int res;

    fdd->fmt = &floppy_formats[format];
    chs.s = fdd->fmt->sec_per_trk;
    res = fdc_read(fdd, &chs, NULL, 1);
    if (res < 0) return res;
    LOG_PRINTF(("%s format detected\n", fdd->fmt->name));
    return FDC_OK;
}


/* Checks disk geometry. Call this after any disk change. */
/* Returns 0 on success.                                  */
int fdc_log_disk(Fdd *fdd)
{
    unsigned k;
    if (fdd->dp->cmos_type == 0) return FDC_ERROR; /* Drive not available */
    fdd->fmt = &floppy_formats[fdd->dp->native_fmt];
    reset_fdc(fdd->fdc);
    reset_drive(fdd);
    /* If changeline is active, no disk is in drive */
    motor_on(fdd);
    if (fd32_inb(fdd->fdc->base_port + FDC_DIR) & 0x80)
    {
        LOG_PRINTF(("[FDC] fdc_log_disk: no disk in drive\n"));
        motor_down(fdd);
        return FDC_NODISK;
    }
    fdd->flags &= ~DF_CHANGED;
    for (k = 0; k < 8; k++)
        if (probe_format(fdd, fdd->dp->detect[k]) == 0)
        {
            /* Finetune the probed format with boot sector informations */
            int res = floppy_bootsector(fdd, floppy_formats, 32);
            if (res >= 0) fdd->fmt = &floppy_formats[res];
            return FDC_OK;
        }
    motor_down(fdd);
    fdd->flags |= DF_CHANGED; /* Must log the disk again the next time */
    return FDC_ERROR; /* If we arrive here, the autodetection failed */
}


/* Initializes a drive to a known state */
static int setup_drive(Fdc *fdc, unsigned drive, unsigned cmos_type)
{
    fdc->drive[drive].fdc       = fdc;
    fdc->drive[drive].dp        = &default_drive_params[0];
    /* No need to initialize fdc->drive[drive].fmt */
    fdc->drive[drive].number    = drive;
    fdc->drive[drive].flags     = 0;
    fdc->drive[drive].track     = 0;
    fdc->drive[drive].spin_down = -1;
    if ((cmos_type > 0) && (cmos_type <= 6))
    {
        fdc->drive[drive].dp        = &default_drive_params[cmos_type];
        fdc->drive[drive].flags     = DF_CHANGED;
    }
    return FDC_OK;
}


/* Initializes the low-level driver */
int fdc_setup(FdcSetupCallback *setup_cb)
{
    unsigned k;
    unsigned cmos_drive0;
    unsigned cmos_drive1;
    int      res;
    WORD     dma_seg, dma_off;

    /* Setup IRQ and DMA */
    /* TODO: Provide a way to save the old IRQ6 handler */
    fd32_irq_bind(6, irq6);
    LOG_PRINTF(("IRQ6 handler installed\n"));
    /* TODO: Find a decent size for the DMA buffer */
    dma_sel = fd32_dmamem_get(512 * 18 * 2, &dma_seg, &dma_off);
    if (dma_sel == 0) return FDC_ERROR; /* Unable to allocate DMA buffer */
    dma_addr = ((DWORD) dma_seg << 4) + (DWORD) dma_off;
    LOG_PRINTF(("DMA buffer allocated at physical address %08lxh\n", dma_addr));

    /* Read floppy drives types from CMOS memory (up to two drives). */
    /* They are supposed to belong to the primary FDC.               */
    fd32_outb(0x70, 0x10);
    k = fd32_inb(0x71);
    cmos_drive0 = k >> 4;
    cmos_drive1 = k & 0x0F;
    setup_drive(&pri_fdc, 0, cmos_drive0);
    setup_drive(&pri_fdc, 1, cmos_drive1);

    /* Reset primary controller */
    pri_fdc.base_port = FDC_BPRI;
    reset_fdc(&pri_fdc);
    sendbyte(pri_fdc.base_port, CMD_VERSION);
    res = getbyte(pri_fdc.base_port);
    LOG_PRINTF(("Byte got from CMD_VERSION: %08x\n", res));
    switch (res)
    {
        case 0x80: LOG_PRINTF(("NEC765 FDC found on base port %04Xh\n", pri_fdc.base_port)); break;
        case 0x90: LOG_PRINTF(("Enhanced FDC found on base port %04Xh\n", pri_fdc.base_port)); break;
        default  : LOG_PRINTF(("FDC not found on base port %04Xh\n", pri_fdc.base_port));
    }
    for (res = FDC_OK, k = 0; k < MAXDRIVES; k++)
        if (setup_cb(&pri_fdc.drive[k]) < 0) res = -1;

    #ifdef HAS2FDCS
    setup_drive(&sec_fdc, 0, 0);
    setup_drive(&sec_fdc, 1, 0);
    sec_fdc.base_port = FDC_BSEC;
    reset_fdc(&sec_fdc);
    for (k = 0; k < MAXDRIVES; k++)
        if (setup_cb(&sec_fdc.drive[k]) < 0) res = -1;
    #endif
    return res;
}


/* Frees low-level driver resources */
void fdc_dispose(void)
{
    /* TODO: How to restore old handler? */
//    fd32_irq_bind(6, &old_irq6);
//    LOG_PRINTF(("IRQ6 handler uninstalled\n"));
    /* TODO: Find a decent size for the DMA buffer */
    fd32_dmamem_free(dma_sel, 512 * 18 * 2);
    LOG_PRINTF(("DMA buffer freed\n"));
    fd32_outb(pri_fdc.base_port + FDC_DOR, 0x0C); /* Stop motor forcefully */
}
