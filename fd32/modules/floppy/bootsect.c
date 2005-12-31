/**************************************************************************
 * FreeDOS32 Floppy Driver                                                *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2003, Salvatore Isaja                                    *
 *                                                                        *
 * This is "bootsect.c" - Floppy format autodetection finetuning          *
 *                        An idea borrowed from the Linux fdutils docs    *
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

#include "fdc.h"


#define __DEBUG__
#ifdef __DEBUG__
  #define LOG_PRINTF(x) fd32_log_printf x
#else
  #define LOG_PRINTF(x)
#endif


/* FAT Boot Sector and BIOS Parameter Block.                         */
/* Starting from sector offset 0, these are only the field needed to */
/* detect the file system is FAT and to read the disk geometry.      */
typedef struct
{
    BYTE  jmp_boot[3];   /* not used here */
    BYTE  oem_name[8];   /* not used here */
    WORD  byts_per_sec;
    BYTE  sec_per_clus;
    WORD  resvd_sec_cnt; /* not used here */
    BYTE  num_fats;      /* not used here */
    WORD  root_ent_cnt;  /* not used here */
    WORD  tot_sec16;
    BYTE  media;
    WORD  fat_sz16;
    WORD  sec_per_trk;
    WORD  num_heads;
    DWORD hidd_sec;      /* not used here */
    DWORD tot_sec32;
}
__attribute__ ((packed)) FatBpb;


/* Given a buffer containing the first sector of a block device, checks  */
/* if it contains a valid FAT BIOS Parameter Block. Only the fields      */
/* common to FAT12/FAT16 and FAT32 BPBs are checked.                     */
/* If the file system is FAT, the not negative index of the detected     */
/* format is returned. Otherwise returns a negative error.               */
static int is_fat(BYTE *sec_buf, const FloppyFormat *formats, unsigned num_formats)
{
    unsigned k;
    DWORD    tot_sec;
    FatBpb  *bpb = (FatBpb *) sec_buf;

    /* Check for the 0xAA55 signature at offset 510 of the boot sector */
    if (*((WORD *) &sec_buf[510]) != 0xAA55)
    {
        LOG_PRINTF(("[FLOPPY] is_fat: Boot sector signature 0xAA55 not found\n"));
        return -1;
    }
    /* Check volume size */
    if (bpb->tot_sec16 == 0)
    {
        if (bpb->tot_sec32 == 0)
        {
            LOG_PRINTF(("[FLOPPY] is_fat: Both BPB_TotSec16 and BPB_TotSec32 are zero\n"));
            return -1;
        }
        tot_sec = bpb->tot_sec32;
    }
    else
    {
        if (bpb->tot_sec32 != 0)
        {
            LOG_PRINTF(("[FLOPPY] is_fat: Both BPB_TotSec16 and BPB_TotSec32 are nonzero\n"));
            return -1;
        }
        tot_sec = bpb->tot_sec16;
    }
    /* BPB_BytsPerSec can be 512, 1024, 2048 or 4096 */
    switch (bpb->byts_per_sec)
    {
        case 512: case 1024: case 2048: case 4096: break;
        default:
            LOG_PRINTF(("[FLOPPY] is_fat: Invalid BPB_BytsPerSec: %u\n", bpb->byts_per_sec));
            return -1;
    }
    /* BPB_SecPerCluster can be 1, 2, 4, 8, 16, 32, 64, 128 */
    switch (bpb->sec_per_clus)
    {
        case 1: case 2: case 4: case 8: case 16: case 32: case 64: case 128: break;
        default:
            LOG_PRINTF(("[FLOPPY] is_fat: Invalid BPB_SecPerClus: %u\n", bpb->sec_per_clus));
            return -1;
    }
    /* BPB_Media can be 0xF0, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF */
    if ((bpb->media != 0xF0) && (bpb->media < 0xF8))
    {
        LOG_PRINTF(("[FLOPPY] is_fat: Invalid BPB_Media: %u\n", bpb->media));
        return -1;
    }
    /* Ok, the disk contains a FAT file system: read the geometry from the BPB */
    for (k = 0; k < num_formats; k++)
    {
        if ((tot_sec           == formats[k].size)
         && (bpb->byts_per_sec == 512) /* TODO: Add custom sector sizes */
         && (bpb->num_heads    == formats[k].heads)
         && (bpb->sec_per_trk  == formats[k].sec_per_trk))
        {
            LOG_PRINTF(("[FLOPPY] is_fat: FAT boot sector reports %s format\n", formats[k].name));
            return k;
        }
    }
    LOG_PRINTF(("[FLOPPY] is_fat: Disk contains FAT, but boot sector reports a strange geometry\n"));
    return -1;
}


int floppy_bootsector(Fdd *fdd, const FloppyFormat *formats, unsigned num_formats)
{
    BYTE sec_buf[512]; /* TODO: Add custom sector sizes */
    Chs  chs = { 0, 0, 1 };
    if (fdc_read(fdd, &chs, sec_buf, 1) < 0)
    {
        LOG_PRINTF(("[FLOPPY] Error reading the boot sector\n"));
        return -1;
    }
    return is_fat(sec_buf, formats, num_formats);
}
