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
/**
 * \defgroup fat FAT file system driver
 *
 * The FreeDOS-32 FAT driver is a highly portable software to gain access to
 * media formatted with the FAT file system.
 *
 * @{ */
#ifndef __FD32_FAT_DRIVER_H
#define __FD32_FAT_DRIVER_H

/* Compile time options for the FAT driver */
#define FAT_CONFIG_LFN       1 /* Enable Long File Names */
#define FAT_CONFIG_WRITE     1 /* Enable write facilities */
#define FAT_CONFIG_REMOVABLE 1 /* Enable support for media change */
#define FAT_CONFIG_FD32      1 /* Enable FD32 devices */
#define FAT_CONFIG_DEBUG     0 /* Enable log output */

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
 #include <time.h>
 #include <sys/time.h>
 #include <sys/stat.h>
 #define mfree(p, size) free(p)
 #define FD32_OROPEN  1
 #define FD32_ORCREAT 2
 #define FD32_ORTRUNC 3
 #if FAT_CONFIG_DEBUG
  #define LOG_PRINTF(s) printf s
 #else
  #define LOG_PRINTF(s)
 #endif
#else
 #include <dr-env.h>
 #include <block/block.h>
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


#if FAT_CONFIG_FD32
typedef struct BlockDev BlockDev;
struct BlockDev
{
	BlockOperations *bops;
	void *handle;
	bool is_open;
};
#else
typedef FILE* BlockDev;
#endif


/* Macros to check whether or not an open file is readable or writable */
#if 1 /* Old-style, zero-based access modes (O_RDONLY = 0, O_WRONLY = 1, O_RDWR = 2) */
 #define IS_NOT_READABLE(c)  (((c->flags & O_ACCMODE) != O_RDONLY) && ((c->flags & O_ACCMODE) != O_RDWR))
 #define IS_NOT_WRITEABLE(c) (((c->flags & O_ACCMODE) != O_RDWR) && ((c->flags & O_ACCMODE) != O_WRONLY))
#else /* New-style, bitwise-distinct access modes (O_RDWR = O_RDONLY | O_WRONLY) */
 #define IS_NOT_READABLE(c)  (!(c->flags & O_RDONLY))
 #define IS_NOT_WRITEABLE(c) (!(c->flags & O_WRONLY))
#endif


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
	FAT_READ_AHEAD  = 8,
	FAT_UNLINKED = 1 /* value of de_sector if file unlinked */
};


typedef enum { FAT12, FAT16, FAT32 } FatType;
typedef uint32_t Sector;
typedef uint32_t Cluster;
typedef struct LookupData LookupData;
typedef struct Buffer     Buffer;
typedef struct Dentry     Dentry;
typedef struct Volume     Volume;
typedef struct File       File;
typedef struct Channel    Channel;


/// Lookup data block for the internal "readdir" function.
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


/// A buffer containing one or more contiguous sectors.
struct Buffer
{
	Buffer  *prev;   /* From ListItem, less recently used */
	Buffer  *next;   /* From ListItem, more recently used */
	Volume  *v;      /* Volume owning this buffer */
	Sector   sector; /* First sector of this buffer */
	unsigned count;  /* Sectors in this buffer */
	int      flags;  /* Buffer state */
	uint8_t *data;   /* Raw data of the buffered sectors */
};


/// A node in the cached directory tree, to locate directory entries.
struct Dentry
{
	Dentry  *prev;        /* From ListItem, previous at the same level */
	Dentry  *next;        /* From ListItem, next at the same level */
	Dentry  *parent;      /* The parent Dentry */
	List     children;    /* List of children of this node */
	unsigned references;  /* Number of Channels and Dentries referring to this Dentry */
	Volume  *v;           /* FAT volume containing this Dentry */
	Sector   de_sector;   /* Sector containing the directory entry.
	                       * 0 if no directory entry (root directory),
	                       * FAT_UNLINKED if referring to an unlinked file. */
	uint16_t de_entcnt;   /* Offset of the short name entry in the parent in sizeof(struct fat_direntry) units */
	uint8_t  attr;        /* Attributes of the directory entry */
	uint8_t  lfn_entries; /* Number of LFN entries occupied by this entry. Undefined if !FAT_CONFIG_LFN */
};


/// A structure storing the state of a mounted FAT volume.
struct Volume
{
	BlockDev blk;
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
	unsigned  buf_access; /* statistics */
	unsigned  buf_miss;   /* statistics */
	unsigned  buf_hit;    /* statistics */
	unsigned  num_buffers;
	Buffer   *buffers;
	uint8_t  *buffer_data;
	List      buffers_lru;

	/* Files */
	unsigned  num_dentries; /* statistics */
	slabmem_t dentries_slab;
	slabmem_t files_slab;
	slabmem_t channels_slab;
	Dentry    root_dentry;
	List      files_open;
	List      channels_open;

	/* A per-volume LookupData to avoid using too much stack */
	LookupData lud;
};


/// The state of a file (shared by open instances).
struct File
{
	File *prev; /* from ListItem */
	File *next; /* from ListItem */
	struct fat_direntry de;
	bool     de_changed;
	Sector   de_sector; /* Sector containing the directory entry.
	                     * 0 if no directory entry (root directory),
	                     * FAT_UNLINKED if file queued for deletion. */
	unsigned de_secofs; /* Byte offset of the directory entry in de_sector */
	Volume  *v;
	Cluster  first_cluster;
	unsigned references;
};


/// An open instance of a file (called a "channel" in glibc documentation).
struct Channel
{
	Channel  *prev;          /* From ListItem */
	Channel  *next;          /* From ListItem */
	off_t     file_pointer;  /* The one set by lseek and updated on r/w */
	File     *f;             /* State of this file */
	uint32_t  magic;         /* FAT_CHANNEL_MAGIC */
	int       flags;         /* Opening flags of this file instance */
	unsigned  references;    /* Number of times this instance is open */
	Cluster   cluster_index; /* Cached cluster position (0 = N/A) */
	Cluster   cluster;       /* Cached cluster address (undefined if cluster_index==0) */
	Dentry   *dentry;        /* Cached directory node for this open instance */
};


/* alloc.c */
int32_t fat12_read(Volume *v, Cluster n, unsigned fat_num);
int32_t fat16_read(Volume *v, Cluster n, unsigned fat_num);
int32_t fat32_read(Volume *v, Cluster n, unsigned fat_num);
int     fat_get_file_sector(Channel *c, Sector sector_index, Sector *sector);
#if FAT_CONFIG_WRITE
int32_t fat_append_cluster(Channel *c);
int     fat_delete_file_clusters(File *f, Cluster new_last_cluster);
int     fat_delete_clusters(Volume *v, Cluster from);
int     fat12_write(Volume *v, Cluster n, unsigned fat_num, Cluster value);
int     fat16_write(Volume *v, Cluster n, unsigned fat_num, Cluster value);
int     fat32_write(Volume *v, Cluster n, unsigned fat_num, Cluster value);
#endif

/* dir.c */
int fat_build_fcb_name(const struct nls_operations *nls, uint8_t *dest, const char *src, size_t src_size, bool wildcards);
int fat_do_readdir(Channel *c, LookupData *lud);
#if FAT_CONFIG_WRITE
int fat_unlink(Dentry *d);
int fat_link  (Channel *c, const char *fn, size_t fnsize, int attr, unsigned hint, LookupData *lud);
int fat_rename(Dentry *od, Dentry *ndparent, const char *nn, size_t nnsize);
int fat_rmdir (Dentry *d);
int fat_mkdir (Dentry *dparent, const char *fn, size_t fnsize, mode_t mode);
#endif

/* dos.c */
int fat_findfirst(Dentry *dparent, const char *fn, size_t fnsize, int attr, fd32_fs_dosfind_t *df);
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
void    fat_timestamps(uint16_t *dos_time, uint16_t *dos_date, uint8_t *dos_hund);
ssize_t fat_do_write (Channel *c, const void *buffer, size_t size, off_t offset);
ssize_t fat_write    (Channel *c, const void *buffer, size_t size);
int     fat_ftruncate(Channel *c, off_t length);
int     fat_set_attr (Channel *c, const fd32_fs_attr_t *a);
#endif

/* open.c */
void fat_dget  (Dentry *d);
void fat_dput  (Dentry *d);
int  fat_open  (Dentry *dentry, int flags, Channel **channel);
int  fat_create(Dentry *dparent, const char *fn, size_t fnsize, int flags, mode_t mode, Channel **channel);
int  fat_reopen_dir(Volume *v, Cluster first_cluster, unsigned entry_count, Channel **channel);
#if FAT_CONFIG_WRITE
int  fat_fsync (Channel *c, bool datasync);
#endif
int  fat_close (Channel *c);
int  fat_lookup(Dentry **dentry, const char *fn, size_t fnsize);

/* volume.c */
#if FAT_CONFIG_WRITE
int fat_sync    (Volume *v);
int fat_dirtybuf(Buffer *b, bool write_through);
#endif
int fat_readbuf (Volume *v, Sector sector, Buffer **buffer, bool read_through);
#if FAT_CONFIG_FD32
int fat_get_fsinfo(fd32_fs_info_t *fsinfo);
int fat_get_fsfree(fd32_getfsfree_t *fsfree);
#endif
int fat_mount(const char *blk_name, Volume **volume);
int fat_unmount(Volume *v);
int fat_partcheck(unsigned id);
#if FAT_CONFIG_REMOVABLE && FAT_CONFIG_FD32
int fat_handle_attention(Volume *v);
#endif

/* Portability */
ssize_t fat_blockdev_read(Volume *v, void *data, size_t count, Sector from);
ssize_t fat_blockdev_write(Volume *v, const void *data, size_t count, Sector from);
int     fat_blockdev_test_unit_ready(Volume *v);

/* Functions with pathname resolution */
int fat_open_pr  (Dentry *dentry, const char *pn, size_t pnsize, int flags, mode_t mode, Channel **channel);
int fat_unlink_pr(Dentry *dentry, const char *pn, size_t pnsize);
int fat_rename_pr(Dentry *odentry, const char *on, size_t onsize,
                  Dentry *ndentry, const char *nn, size_t nnsize);
int fat_rmdir_pr (Dentry *dentry, const char *pn, size_t pnsize);
int fat_mkdir_pr (Dentry *dentry, const char *pn, size_t pnsize, mode_t mode);
int fat_findfirst_pr(Dentry *dentry, const char *pn, size_t pnsize, int attr, fd32_fs_dosfind_t *df);

/* @} */
#endif /* #ifndef __FD32_FAT_DRIVER_H */
