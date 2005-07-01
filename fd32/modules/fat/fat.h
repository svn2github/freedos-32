/* The FreeDOS-32 FAT Driver version 2.0
 * Copyright (C) 2001-2005  Salvatore ISAJA
 *
 * This file "fat.h" is part of the FreeDOS-32 FAT Driver (the Program).
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
/**
 * \file
 * \brief Declarations of the facilities provided by the FAT driver.
 */
#ifndef __FD32_FAT_DRIVER_H
#define __FD32_FAT_DRIVER_H

/* Compile time options for the FAT driver */
#define FAT_CONFIG_LFN       1 /* Enable Long File Names */
#define FAT_CONFIG_WRITE     0 /* Enable write facilities */
#define FAT_CONFIG_REMOVABLE 1 /* Enable support for media change */
#define FAT_CONFIG_FD32      1 /* Enable FD32 devices */
#define FAT_CONFIG_DEBUG     0 /* Enable log output */

#if FAT_CONFIG_WRITE
 #error The FAT driver 2.0 is not yet ready for write support!
#endif

#if !FAT_CONFIG_FD32
 #include <stdio.h>
 #include <sys/types.h>
 #include <errno.h>
 #define EFTYPE  2000
 #define ENMFILE 2001
 #define __USE_GNU
 #define O_NOATIME (1 << 31)
 #include <string.h>
 #include <fcntl.h>
 #include <stdint.h>
 #define mfree(p, size) free(p)
 #define FD32_OROPEN  1
 #define FD32_ORCREAT 2
 #define FD32_ORTRUNC 3
 #define LOG_PRINTF(s)
#else
 #include <dr-env.h>
 #define malloc fd32_kmem_get
 #define mfree fd32_kmem_free
 typedef int mode_t;
 #define SEEK_SET 0
 #define SEEK_CUR 1
 #define SEEK_END 2
 #if FAT_CONFIG_DEBUG
  #define LOG_PRINTF(s) fd32_log_printf s
 #else
  #define LOG_PRINTF(s)
 #endif
#endif
#ifndef __cplusplus
typedef enum { false = 0, true = !false } bool;
#endif
#include <nls/nls.h>
#include <unicode/unicode.h>
#include <filesys.h>
#include <list.h>
#include <slabmem.h>
#include "ondisk.h"

/* The character to use as path component separator */
/* TODO: Replace the backslash with a forward slash for internal operation. Use backslash as a quotation character. */
#define FAT_PATH_SEP '\\'

enum
{
	FAT_VOL_MAGIC = 0x46415456, /* "FATV": valid FAT volume signature */
	FAT_CHANNEL_MAGIC = 0x46415446, /* "FATF": valid FAT file signature */
	FAT_EOC = 0x0FFFFFFF,
	/* TODO: these should be command line options */
	FAT_NUM_BUFFERS = 30,
	FAT_READ_AHEAD  = 8
};

typedef enum { FAT12, FAT16, FAT32 } FatType;
typedef uint32_t Sector;
typedef uint32_t Cluster;
typedef struct LookupData   LookupData;
typedef struct Buffer       Buffer;
typedef struct Volume       Volume;
typedef struct File         File;
typedef struct Channel      Channel;


/* Lookup data block for the internal "readdir" function */
struct LookupData
{
	wchar_t  sfn[FAT_SFN_MAX];
	unsigned sfn_length;
	#if FAT_CONFIG_LFN
	struct fat_direntry cde[21]; /* Cached directory entries */
	wchar_t  lfn[FAT_LFN_MAX];
	unsigned cde_pos;
	unsigned lfn_entries;
	unsigned lfn_length;
	#else
	struct fat_direntry cde;
	#endif
	Sector   de_sector;
	unsigned de_secofs;
	off_t    de_dirofs;
};


/* A buffer containing one or more contiguous sectors */
struct Buffer
{
	const Volume *v;
	Buffer  *next;
	Sector   sector; /* First sector of this buffer */
	unsigned count;  /* Sectors in this buffer */
	int      flags;
	uint8_t *data; /* Raw data of the buffered sectors */
};


/* This structure stores the state of a mounted FAT volume */
struct Volume
{
	#if FAT_CONFIG_FD32
	/* Data referring to the hosting block device */
	uint32_t        blk_handle;
	fd32_request_t *blk_request;
	void           *blk_devid;
	#else
	FILE *blk;
	#endif
	struct nls_operations *nls;

	/* Some precalculated data */
	uint32_t magic; /* FAT_VOL_MAGIC */
	FatType  fat_type;
	unsigned num_fats;
	unsigned bytes_per_sector;
	unsigned log_bytes_per_sector;
	unsigned sectors_per_cluster;
	unsigned log_sectors_per_cluster;
	unsigned active_fat;
	uint32_t serial_number;
	uint8_t  volume_label[11];
	Sector   fat_size;
	Sector   fat_start;
	Sector   root_sector;
	Sector   root_size;
	Sector   data_start;
	Cluster  root_cluster;
	Cluster  data_clusters;
	Cluster  free_clusters;
	Cluster  next_free;

	/* Functions to access the file allocation table */
	int32_t (*fat_read) (Volume *v, Cluster n, unsigned fat_num);
	#if FAT_CONFIG_WRITE
	int     (*fat_write)(Volume *v, Cluster n, unsigned fat_num, Cluster value);
	#endif

	/* Buffers */
	unsigned  sectors_per_buffer;
	unsigned  buf_access;
	unsigned  buf_miss;
	unsigned  buf_hit;
	unsigned  num_buffers;
	Buffer   *buffers;
	uint8_t  *buffer_data;
	Buffer   *buffer_lru;
	Buffer   *buffer_mru;

	/* Files */
	slabmem_t files;
	slabmem_t channels;
	File     *files_open;
	Channel  *channels_open;

	/* A per-volume LookupData to avoid using too much stack */
	LookupData lud;
};


/* The state of a file (shared by open instances). */
struct File
{
	File *prev; /* from ListItem */
	File *next; /* from ListItem */
	struct fat_direntry de;
	bool     de_changed;
	Sector   de_sector;
	unsigned de_secoff;
	Volume  *v;
	Cluster  first_cluster;
	unsigned references;
};


/* An open instance of a file (called a "channel" in glibc documentation). */
struct Channel
{
	Channel *prev; /* from ListItem */
	Channel *next; /* from ListItem */
	off_t     file_pointer;  /* The one set by lseek and updated on r/w */
	File     *f;             /* State of this file */
	uint32_t  magic;         /* FAT_CHANNEL_MAGIC */
	int       flags;         /* Opening flags of this file instance */
	unsigned  references;    /* Number of times this instance is open */
	Cluster   cluster_index; /* Cached cluster position */
	Cluster   cluster;       /* Cached cluster address */
};


/* alloc.c */
int fat_get_file_sector(Channel *c, Sector sector_index, Sector *sector);
int32_t fat12_read(Volume *v, Cluster n, unsigned fat_num);
int32_t fat16_read(Volume *v, Cluster n, unsigned fat_num);
int32_t fat32_read(Volume *v, Cluster n, unsigned fat_num);
#if FAT_CONFIG_WRITE
int32_t fat_append_cluster(Channel *c);
int fat12_write(Volume *v, Cluster n, unsigned fat_num, Cluster value);
int fat16_write(Volume *v, Cluster n, unsigned fat_num, Cluster value);
int fat32_write(Volume *v, Cluster n, unsigned fat_num, Cluster value);
#endif

/* dir.c */
int fat_build_fcb_name(const struct nls_operations *nls, uint8_t *dest, const char *source, bool wildcards);
int fat_do_readdir(Channel *c, LookupData *lud);
int fat_lookup(Channel *c, const char *fn, size_t fnsize, LookupData *lud);

/* dos.c */
int fat_findfirst(Volume *v, const char *fn, int attr, fd32_fs_dosfind_t *df);
int fat_findnext (Volume *v, fd32_fs_dosfind_t *df);
int fat_findfile (Channel *c, const char *fn, size_t fnsize, int flags, fd32_fs_lfnfind_t *lfnfind);

#if FAT_CONFIG_FD32
/* fd32.c */
fd32_request_t fat_request;
#endif

/* file.c */
off_t   fat_lseek    (Channel *c, off_t offset, int whence);
ssize_t fat_read     (Channel *c, void *buffer, size_t size);
int     fat_get_attr (Channel *c, fd32_fs_attr_t *a);
#if FAT_CONFIG_WRITE
ssize_t fat_write    (Channel *c, const void *buffer, size_t size);
int     fat_ftruncate(Channel *c, off_t length);
int     fat_set_attr (Channel *c, const fd32_fs_attr_t *a);
#endif

/* open.c */
int fat_open      (Volume *v, const char *file_name, const char *fn_stop, int flags, mode_t mode, Channel **channel);
int fat_reopen_dir(Volume *v, Cluster first_cluster, unsigned entry_count, Channel **channel);
int fat_close     (Channel *c);

/* volume.c */
#if FAT_CONFIG_WRITE
int fat_sync    (Volume *v);
int fat_dirtybuf(Buffer *b, bool write_through);
#endif
int fat_readbuf (Volume *v, Sector sector, Buffer **buffer, bool read_through);
#if FAT_CONFIG_FD32
int fat_mount(DWORD blk_handle, Volume **volume);
int fat_get_fsinfo(fd32_fs_info_t *fsinfo);
int fat_get_fsfree(fd32_getfsfree_t *fsfree);
#else
int fat_mount(const char *blk_name, Volume **volume);
#endif
int fat_unmount(Volume *v);
int fat_partcheck(unsigned id);
#if FAT_CONFIG_REMOVABLE && FAT_CONFIG_FD32
int fat_mediachange(Volume *v);
#endif


#endif /* #ifndef __FD32_FAT_DRIVER_H */
