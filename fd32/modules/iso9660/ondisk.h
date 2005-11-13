/* The FreeDOS-32 ISO 9660 Driver version 0.2
 * Copyright (C) 2005  Salvatore ISAJA
 *
 * This file "ondisk.h" is part of the FreeDOS-32 ISO 9660 Driver (the Program).
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
#ifndef __ISO9660_ONDISK_H
#define __ISO9660_ONDISK_H

#ifndef PACKED
 #define PACKED __attribute__ ((packed))
#endif


typedef struct
{
	uint16_t le;
	uint16_t be;
}
iso9660_u16_t;


typedef struct
{
	uint32_t le;
	uint32_t be;
}
iso9660_u32_t;


struct iso9660_dir_record_timestamp
{
	uint8_t year; /* since 1990 */
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minutes;
	uint8_t seconds;
	int8_t  timezone; /* in 15 min intervals from GMT */
} PACKED;


enum iso9660_file_flags
{
	ISO9660_FL_EX   = 1 << 0, /* hidden file (existence) */
	ISO9660_FL_DIR  = 1 << 1, /* is a directory */
	ISO9660_FL_AF   = 1 << 2, /* is an Associated File */
	ISO9660_FL_REC  = 1 << 3, /* use Record Format in an Extended Attributes Record */
	ISO9660_FL_PROT = 1 << 4, /* protection in Extended Attributes Record apply */
	ISO9660_FL_ME   = 1 << 7  /* is Multi-Extent (not the final directory record) */
};


struct iso9660_dir_record
{
	uint8_t       len_dr;
	uint8_t       len_ear;
	iso9660_u32_t extent;
	iso9660_u32_t data_length; /* not including any Extended Attribute Record */
	struct iso9660_dir_record_timestamp recording_time;
	uint8_t       flags; /* see enum iso9660_file_flags */
	uint8_t       file_unit_size;
	uint8_t       interleave_gap_size;
	iso9660_u16_t seq_number;
	uint8_t       len_fi;
	char          file_id[1]; /* len_fi bytes, 0-padded if len_fi is even */
	/* System Use bytes may follow, up to len_dr */
} PACKED;


struct iso9660_path_record
{
	uint8_t  len_di; /* length of Directory Identifier */
	uint8_t  len_ear; /* Extended Attribute Record length */
	uint32_t extent;
	uint16_t parent;
	char     dir_id[2]; /* len_di bytes, 0-padded if len_di is odd */
} PACKED;


struct iso9660_pvd_timestamp
{
	char   year[4];    /* 1 to 9999 */
	char   month[2];   /* 1 to 12 */
	char   day[2];     /* 1 to 31 */
	char   hour[2];    /* 0 to 23 */
	char   minute[2];  /* 0 to 59 */
	char   seconds[2]; /* 0 to 59 */
	char   hundredths[2];
	int8_t timezone;   /* in 15 min intervals from GMT */
} PACKED;


struct iso9660_pvd
{
	uint8_t       type; /* 1 */
	char          id[5]; /* CD001 */
	uint8_t       version; /* 1 */
	uint8_t       unused1; /* 0 */
	char          sys_id[32]; /* a-characters */
	char          vol_id[32]; /* d-characters */
	uint8_t       unused2[8]; /* 0 */
	iso9660_u32_t vol_space;
	uint8_t       unused3[32]; /* 0 */
	iso9660_u16_t set_size;
	iso9660_u16_t seq_number;
	iso9660_u16_t logical_block_size;
	iso9660_u32_t path_table_size;
	uint32_t      lpath_table_le;
	uint32_t      opt_lpath_table_le;
	uint32_t      mpath_table_be;
	uint32_t      opt_mpath_table_be;
	struct iso9660_dir_record root_directory; /* 34 bytes, no file_id */
	char          set_id[128]; /* d-characters */
	char          publisher_id[128];
	char          preparer_id[128];
	char          application_id[128];
	char          copyright_file[37];
	char          abstract_file[37];
	char          bibliografic_file[37];
	struct iso9660_pvd_timestamp creation_time;     /* 17 bytes */
	struct iso9660_pvd_timestamp modification_time; /* 17 bytes */
	struct iso9660_pvd_timestamp expiration_time;   /* 17 bytes */
	struct iso9660_pvd_timestamp effective_time;    /* 17 bytes */
	uint8_t       file_structure; /* 1 */
	uint8_t       unused4; /* 0 */
	uint8_t       application_use[512]; /* not specified */
	uint8_t       unused5[653]; /* 0 */
} PACKED;


#endif /* #ifndef __ISO9660_ONDISK_H */
