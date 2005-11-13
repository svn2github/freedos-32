/* The FreeDOS-32 ISO 9660 Driver version 0.2
 * Copyright (C) 2005  Salvatore ISAJA
 *
 * This file "iso9660.h" is part of the FreeDOS-32 ISO 9660 Driver (the Program).
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
#ifndef __ISO9660_H
#define __ISO9660_H

#define ISO9660_CONFIG_FD32         1 /* FreeDOS-32 target */
#define ISO9660_CONFIG_DEBUG        1 /* Enable debugging output */
#define ISO9660_CONFIG_STRIPVERSION 1 /* Strip version number from file identifiers */
#define ISO9660_CONFIG_DOS          1 /* Enable DOS-specific behaviors */
/* Set this to x.le for little endian or x.be for big endian */
#define ME(x) x.le

#ifndef __cplusplus
 typedef enum { false = 0, true = !false } bool;
#endif
#if !ISO9660_CONFIG_FD32
 #include <stdio.h>
 #include <time.h>
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <errno.h>
 #include <stdlib.h>
 #include <string.h>
 #include <fcntl.h>
 #include <limits.h>
 #include <stdint.h>
 #define restrict __restrict__
 #define O_DIRECTORY (1 << 31)
 //#define NDEBUG
 #include <assert.h>
 #define ENMFILE 10000
 #define FD32_OROPEN 1
 #define mfree(b, s) free(b)
 typedef FILE* BlockDev;
 #if ISO9660_CONFIG_DEBUG
  #define LOG_PRINTF(s) printf s
 #else
  #define LOG_PRINTF(s)
 #endif
 #include <filesys.h>
#else /* #if ISO9660_CONFIG_FD32 */
 #include <dr-env.h>
 #include <block/block.h>
 #include <filesys.h>
 #include <errno.h>
 typedef struct
 {
 	BlockOperations *bops;
 	void *handle;
 	bool is_open;
 }
 BlockDev;
 typedef uint32_t time_t;
 typedef int mode_t;
 #define SEEK_SET 0
 #define SEEK_CUR 1
 #define SEEK_END 2
 #define malloc fd32_kmem_get
 #define mfree fd32_kmem_free
 #if ISO9660_CONFIG_DEBUG
  #define LOG_PRINTF(s) fd32_log_printf s
 #else
  #define LOG_PRINTF(s)
 #endif
#endif
#include <list.h>
#include <slabmem.h>
#include "ondisk.h"

enum
{
	/* 4-characters signatures to identify correct state structures */
	ISO9660_VOL_MAGIC  = 0x49534F56, /* 'ISOV' */
	ISO9660_FILE_MAGIC = 0x49534F46, /* 'ISOF' */
};

typedef uint32_t Sector;
typedef uint32_t Block;
typedef struct Buffer    Buffer;
typedef struct Volume    Volume;
typedef struct Dentry    Dentry;
typedef struct File      File;


/* A buffer containing one sector, or one or more contiguous logical blocks */
struct Buffer
{
	Sector   sector;
	Block    block; /* First logical block of this buffer */
	int      valid; /* Nonzero if buffered data is valid */
	uint8_t *data;  /* Raw data of the buffered sector */
};


struct Dentry
{
	Dentry  *prev;        /* From ListItem, previous at the same level */
	Dentry  *next;        /* From ListItem, next at the same level */
	Dentry  *parent;      /* The parent Dentry */
	List     children;    /* List of children of this node */
	unsigned references;  /* Number of Channels and Dentries referring to this Dentry */
	Volume  *v;           /* Volume containing this Dentry */
	Sector   de_block;    /* Block containing the directory entry.
	                       * 0 if no directory entry (root directory). */
	unsigned de_blkofs;   /* Byte offset of the directory entry in de_sector */
	unsigned flags;       /* File flags as per directory record */
};


/* State of a mounted ISO 9660 volume */
struct Volume
{
	BlockDev blockdev;

	/* Some precalculated data */
	char vol_id[32];
	uint32_t magic; /* ISO9660_VOL_MAGIC */
	unsigned bytes_per_sector;
	unsigned log_bytes_per_sector;
	unsigned block_size;
	unsigned log_block_size;
	unsigned blocks_per_sector;
	unsigned log_blocks_per_sector;

	/* Root directory */
	Block    root_extent;
	unsigned root_len_ear;
	uint32_t root_data_length;

	/* Buffers */
	unsigned buf_access; /* statistics */
	unsigned buf_miss;   /* statistics */
	unsigned buf_hit;    /* statistics */
	Buffer   buffer;

	/* Files */
	unsigned  num_dentries; /* statistics */
	SlabCache dentries_slab;
	SlabCache files_slab;
	Dentry    root_dentry;
	List      files_open;
};


/* State of an open file */
struct File
{
	File    *prev; /* from ListItem */
	File    *next; /* from ListItem */
	Dentry  *dentry;        /* Cached directory node for this open instance */
	Block    extent;
	uint32_t data_length;   /* not including any Extended Attribute Record */
	unsigned len_ear;
	time_t   recording_time;
	unsigned file_flags;    /* see enum iso9660_file_flags */
	off_t    file_pointer;  /* the one set by lseek and updated on r/w */
	uint32_t magic;         /* ISO9660_FILE_MAGIC */
	int      open_flags;    /* opening flags of this file instance */
	unsigned references;    /* number of times this instance is open */
};


/*****************************************************************************
 *  Driver functions
 *  Unless otherwise stated, they all return 0 on success or a negative
 *  error code on failure.
 *****************************************************************************/


/* volume.c */
int iso9660_unmount(Volume *v);
int iso9660_mount(const char *device_name, Volume **volume);
int iso9660_readbuf(Volume *v, Block block, Buffer **buffer);

/* dentry.c */
void iso9660_dget  (Dentry *d);
void iso9660_dput  (Dentry *d);
int  iso9660_open  (Dentry *dentry, int flags, File **file);
int  iso9660_lookup(Dentry **dentry, const char *fn, size_t fnsize);

/* dos.c */
int iso9660_build_fcb_name(char *dest, const char *src, size_t src_size, bool wildcards);
int iso9660_compare_fcb_names(const char *s, const char *sw);
int iso9660_dos_fnmatch(const char *p, size_t psize, const char *s, size_t ssize);
int iso9660_findfirst(Dentry *dparent, const char *fn, size_t fnsize, int attr, fd32_fs_dosfind_t *df);
int iso9660_findnext (Volume *v, fd32_fs_dosfind_t *df);
int iso9660_findfile (File *f, const char *fn, size_t fnsize, int flags, fd32_fs_lfnfind_t *lfnfind);
#if ISO9660_CONFIG_FD32
int iso9660_get_attr(File *f, fd32_fs_attr_t *a);
int iso9660_get_fsfree(fd32_getfsfree_t *fsfree);
#endif

/* file.c */
off_t   iso9660_lseek     (File *f, off_t offset, int whence);
ssize_t iso9660_read      (File *f, void *buffer, size_t size);
//int     iso9660_fstat     (File *f, struct stat *buf);
int     iso9660_do_readdir(File *f, Buffer **buffer, Block *de_block, unsigned *de_blkofs);
int     iso9660_close     (File *f);

/* Portability */
ssize_t iso9660_blockdev_read(Volume *v, void *data, size_t count, Sector from);
int     iso9660_blockdev_test_unit_ready(Volume *v);

/* Functions with pathname resolution */
int iso9660_open_pr     (Dentry *dentry, const char *pn, size_t pnsize, int flags, mode_t mode, File **file);
int iso9660_findfirst_pr(Dentry *dentry, const char *pn, size_t pnsize, int attr, fd32_fs_dosfind_t *df);

#endif /* #ifndef __ISO9660_H */
