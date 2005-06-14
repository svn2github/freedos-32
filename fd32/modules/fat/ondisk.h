/* The FreeDOS-32 FAT Driver version 2.0
 * Copyright (C) 2001-2005  Salvatore ISAJA
 *
 * This file "ondisk.h" is part of the FreeDOS-32 FAT Driver (the Program).
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
 * Format for the on-disk data structures for the FAT file system.
 */
#ifndef __FAT_ONDISK_H
#define __FAT_ONDISK_H

#ifndef PACKED
 #define PACKED __attribute__ ((packed))
#endif


/* EOC (End Of Clusterchain) check macros.
 * These expressions are true (nonzero) if the value of a FAT entry is
 * an EOC for the FAT type. An EOC indicates the last cluster of a file.
 */
#define  IS_FAT12_EOC(entry_value)  (entry_value >= 0x0FF8)
#define  IS_FAT16_EOC(entry_value)  (entry_value >= 0xFFF8)
#define  IS_FAT32_EOC(entry_value)  (entry_value >= 0x0FFFFFF8)


/* File attributes */
#define FAT_ARDONLY  0x01 /* Read only */
#define FAT_AHIDDEN  0x02 /* Hidden */
#define FAT_ASYSTEM  0x04 /* System */
#define FAT_AVOLID   0x08 /* Volume label */
#define FAT_ADIR     0x10 /* Directory */
#define FAT_AARCHIV  0x20 /* Modified since last backup */
#define FAT_ALFN     0x0F /* Long file name directory slot (R+H+S+V) */
#define FAT_AALL     0x3F /* Select all attributes */
#define FAT_ANONE    0x00 /* Select no attributes */
#define FAT_ANOVOLID 0x37 /* All attributes but volume label */


/* Because of the FAT??_BAD markers, the following are the max cluster
 * number for a FAT file system:
 * FAT12: 4086 (0x0FF6), FAT16: 65526 (0xFFF6), FAT32: 0x0FFFFFF6.
 */
enum
{
	/* Set a FAT entry to FATxx_BAD to mark the cluster as bad. */
	FAT12_BAD = 0x0FF7,
	FAT16_BAD = 0xFFF7,
	FAT32_BAD = 0x0FFFFFF7,
	/* The default End Of Clusterchain marker */
	FAT12_EOC = 0x0FFF,
	FAT16_EOC = 0xFFFF,
	FAT32_EOC = 0x0FFFFFFF,
	/* Special codes for the first byte of a directory entry */
	FAT_FREEENT  = 0xE5, /* The directory entry is free             */
	FAT_ENDOFDIR = 0x00, /* This and the following entries are free */
	/* Signatures for the FSInfo sector */
	FAT_FSI_SIG1 = 0x41615252,
	FAT_FSI_SIG2 = 0x61417272,
	FAT_FSI_SIG3 = 0xAA550000,
	FAT_FSI_NA   = 0xFFFFFFFF,  /* FSInfo value Not Available */

	FAT_SFN_MAX  = 12, /* Max characters in a short file name (excluding the null terminator) */
	#if FAT_CONFIG_LFN
	FAT_LFN_MAX  = 260 /* Max characters in a long file name (excluding the null terminator) */
	#endif
};


/* Boot Sector and BIOS Parameter Block for FAT12 and FAT16 */
struct fat16_bpb
{
	/* These fields are common to FAT12, FAT16 and FAT32 */
	uint8_t  jump[3];             /* assembly JMP to boot code */
	uint8_t  oem_name[8];         /* who formatted the volume */
	uint16_t bytes_per_sector;
	uint8_t  sectors_per_cluster;
	uint16_t reserved_sectors;
	uint8_t  num_fats;
	uint16_t root_entries;        /* size of the FAT12/FAT16 root directory */
	uint16_t num_sectors_16;      /* 0 if it does not fit in 16 bits */
	uint8_t  media_id;
	uint16_t fat_size_16;         /* 0 if it does not fit in 16 bits */
	uint16_t sectors_per_track;
	uint16_t num_heads;
	uint32_t hidden_sectors;
	uint32_t num_sectors_32;      /* if num_sectors_16 is 0 */
	/* The following fields are present also in FAT32, but at offset 64 */
	uint8_t  bios_drive;
	uint8_t  reserved1;
	uint8_t  boot_sig;
	uint32_t serial_number;
	uint8_t  volume_label[11];
	uint8_t  fs_name[8];       /* a descriptive file system name */
} PACKED;


/* Boot Sector and BIOS Parameter Block for FAT32 */
struct fat32_bpb
{
	/* These fields are common to FAT12, FAT16 and FAT32 */
	uint8_t  jump[3];             /* assembly JMP to boot code */
	uint8_t  oem_name[8];         /* who formatted the volume */
	uint16_t bytes_per_sector;
	uint8_t  sectors_per_cluster;
	uint16_t reserved_sectors;
	uint8_t  num_fats;
	uint16_t root_entries;        /* size of the FAT12/FAT16 root directory */
	uint16_t num_sectors_16;      /* 0 if it does not fit in 16 bits */
	uint8_t  media_id;
	uint16_t fat_size_16;         /* 0 if it does not fit in 16 bits */
	uint16_t sectors_per_track;
	uint16_t num_heads;
	uint32_t hidden_sectors;
	uint32_t num_sectors_32;      /* if num_sectors_16 is 0 */
	/* Here start the FAT32 specific fields (offset 36) */
	uint32_t fat_size_32;       /* if fat_size_16 is 0 */
	uint16_t ext_flags;
	uint16_t fs_version;
	uint32_t root_cluster;
	uint16_t fsinfo_sector;
	uint16_t bootsector_backup; /* sector containing the backup */
	uint8_t  reserved[12];
	/* The following fields are present in a FAT12/FAT16 BPB too,
	 * but at offset 36. In a FAT32 BPB they are at offset 64 instead. */
	uint8_t  bios_drive;
	uint8_t  reserved1;
	uint8_t  boot_sig;
	uint32_t serial_number;
	uint8_t  volume_label[11];
	uint8_t  fs_name[8];       /* a descriptive file system name */
} PACKED;


/* FAT32 FSInfo Sector structure */
struct fat_fsinfo
{
	uint32_t sig1;           /* FAT_FSI_SIG1 */
	uint8_t  reserved1[480]; /* zero */
	uint32_t sig2;           /* FAT_FSI_SIG2 */
	uint32_t free_clusters;  /* count of free clusters, or FAT_FSI_NA */
	uint32_t next_free;      /* hint for the next free cluster, or FAT_FSI_NA */
	uint8_t  reserved2[12];  /* zero */
	uint32_t sig3;           /* FAT_FSI_SIG3 */
} PACKED;


/* 32-byte Directory Entry structure */
struct fat_direntry
{
	uint8_t  name[11];
	uint8_t  attr;
	uint8_t  nt_case;
	uint8_t  cre_time_hund;
	uint16_t cre_time;
	uint16_t cre_date;
	uint16_t acc_date;
	uint16_t first_cluster_hi;
	uint16_t mod_time;
	uint16_t mod_date;
	uint16_t first_cluster_lo;
	uint32_t file_size;
} PACKED;


#if FAT_CONFIG_LFN
/* 32-byte Long File Name Directory Entry structure */
struct fat_lfnentry
{
	uint8_t  order;         /* Sequence number for slot */
	uint16_t name0_4[5];    /* UTF-16 characters */
	uint8_t  attr;          /* 0x0F */
	uint8_t  reserved;      /* 0x00 */
	uint8_t  checksum;      /* Checksum of 8.3 name */
	uint16_t name5_10[6];   /* UTF-16 characters */
	uint16_t first_cluster; /* 0x0000 */
	uint16_t name11_12[2];  /* UTF-16 characters */
} PACKED;
#endif


#endif /* #ifndef __FAT_ONDISK_H */
