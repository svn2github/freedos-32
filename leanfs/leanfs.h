/*  The LEAN File System Driver version 0.1
 *  Copyright (C) 2004  Salvatore ISAJA
 *
 *  LEAN (Lean yet Effective Allocation and Naming) is a free, simple,
 *  portable, personal, full featured file system for embedded tasks
 *  designed by Salvatore Isaja, and somewhat inspired from the EXT2 fs.
 *  This driver provides read and write access to media formatted with LEAN.
 *
 *  This file is part of the LEAN File System Driver (the Program).
 *
 *  The Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  The Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the Program; see the file COPYING; if not, write to
 *  the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* This file is "leanfs.h".
 * It contains the format for the data structures for the LEAN
 * file system, and the declarations for the LEAN File System Driver.
 */
#ifndef __FD32_LEANFS_H
#define __FD32_LEANFS_H


#define LEANFS_CONFIG_TEST  1 /* Use regular files instead of FD32 devices */
#define LEANFS_CONFIG_WRITE 1 /* Enable write access support               */


#if 0 /* MinGW includes and declarations */
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#define ENOTBLK 10000
typedef unsigned long blkcnt_t;
#include <stdio.h>
#include <fcntl.h>
#define NAME_MAX 255
#define O_DIRECTORY 0x80000000
#define O_NOATIME   0x40000000
#define O_FSYNC     0x20000000
#define O_SYNC      O_FSYNC
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
enum
{
DT_UNKNOWN,
DT_REG,
DT_DIR,
DT_FIFO,
DT_SOCK,
DT_CHR,
DT_BLK
};
#include "devices.h"
#ifndef __cplusplus
typedef enum { false = 0, true = !false } bool;
#endif
#else /* Linux GCC includes and declarations */
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#define O_DIRECTORY 0x80000000
#define O_NOATIME   0x40000000
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#if !LEANFS_CONFIG_TEST
#include "devices.h"
#endif

#ifndef __cplusplus
typedef enum { false = 0, true = !false } bool;
#endif
#endif


/******************************************************************************/
/*                                                                            */
/*                              On-disk format                                */
/*                                                                            */
/******************************************************************************/


/* Bits and values for struct SuperBlock */
enum
{
	LEANFS_SB_MAGIC = 0x4E41454C, /* 'LEAN' in little endian  */
	LEANFS_SB_CLEAN = 1 << 0,     /* state: clean unmount     */
	LEANFS_SB_ERROR = 1 << 1,     /* state: file system error */
};


/* Values for struct DirEntry.type */
enum
{
	LEANFS_FT_REG = 0, /* Regular file  */
	LEANFS_FT_DIR = 1, /* Directory     */
	LEANFS_FT_LNK = 2, /* Symbolic link */
};


/* Bits and values for struct Inode.attributes */
enum
{
	/* Permission bits (rwxrwxrwx) */
	LEANFS_IRUSR = 1 << 8,  /* Owner:  read permission    */
	LEANFS_IWUSR = 1 << 7,  /* Owner:  write permission   */
	LEANFS_IXUSR = 1 << 6,  /* Owner:  execute permission */
	#if 0 /* No multiuser support for now */
	LEANFS_IRGRP = 1 << 5,  /* Group:  read permission    */
	LEANFS_IWGRP = 1 << 4,  /* Group:  write permission   */
	LEANFS_IXGRP = 1 << 3,  /* Group:  execute permission */
	LEANFS_IROTH = 1 << 2,  /* Others: read permission    */
	LEANFS_IWOTH = 1 << 1,  /* Others: write permission   */
	LEANFS_IXOTH = 1 << 0,  /* Others: execute permission */
	LEANFS_ISUID = 1 << 11, /* Set-user-ID on execute     */
	LEANFS_ISGID = 1 << 10, /* Set-groupt-ID on execute   */
	LEANFS_ISVTX = 1 << 9,  /* Sticky bit                 */
	#endif
	/* Behaviour flags */
	LEANFS_HIDDEN_FL    = 1 << 12, /* Don't show in directory listing */
	LEANFS_SYSTEM_FL    = 1 << 13, /* Warn that this is a system file */
	LEANFS_ARCHIVE_FL   = 1 << 14, /* File changed since last backup  */
	LEANFS_SYNC_FL      = 1 << 15, /* Synchronous updates             */
	LEANFS_NOATIME_FL   = 1 << 16, /* Don't update last access time   */
	LEANFS_IMMUTABLE_FL = 1 << 17, /* Don't move file clusters        */
	/* File type codes */
	LEANFS_IFMT  = 7 << 29, /* Bit mask to extract file type */
	LEANFS_IFREG = LEANFS_FT_REG << 29, /* Regular file  */
	LEANFS_IFDIR = LEANFS_FT_DIR << 29, /* Directory     */
	LEANFS_IFLNK = LEANFS_FT_LNK << 29, /* Symbolic link */
};


/* This macro converts the file type codes of an inode structure
 * to the file type for a directory entry.
 */
#define LEANFS_IFMT_TO_FT(attr) ((attr & LEANFS_IFMT) >> 29)


#define MAX_DIRECT_CLUSTER 15
struct Inode
{
	uint64_t file_size;
	uint32_t clusters_count;
	uint32_t acc_time;
	uint32_t cre_time;
	uint32_t mod_time;
	uint32_t dclusters[MAX_DIRECT_CLUSTER];
	uint32_t iclusters_head;
	uint32_t attributes;
	uint32_t uid;
	uint32_t gid;
	uint32_t links_count;
	uint8_t  reserved[24]; /* 0 pad to 128 bytes */
};


struct DirEntryHeader
{
	uint32_t inode;
	uint16_t rec_len;
	uint8_t  name_len;
	uint8_t  type;
};


struct DirEntry /* must be zero padded to a multiple of 4 bytes */
{
	uint32_t inode;
	uint16_t rec_len;
	uint8_t  name_len;
	uint8_t  type;
	char     name[256]; /* variable length */
};


struct SuperBlock
{
	uint32_t magic;
	uint32_t serial_number;
	char     volume_label[64];
	uint16_t fs_version;
	uint8_t  log_bytes_per_sector;
	uint8_t  log_sectors_per_cluster;
	uint32_t backup_super;
	uint32_t clusters_count;
	uint32_t free_clusters_count;
	uint32_t bitmap_start_cluster;
	uint32_t root_start_cluster;
	uint32_t bad_start_cluster;
	uint32_t checksum;
	uint32_t state;
	uint8_t  reserved[404]; /* 0 pad to 512 bytes */
};


/* This macro computes the superblock checksum. */
/* Call passing a struct SuperBlock.            */
#define LEANFS_SB_CHECKSUM(sb)  (sb.magic + sb.fs_version + sb.log_bytes_per_sector + sb.log_sectors_per_cluster + sb.backup_super + sb.clusters_count + sb.bitmap_start_cluster + sb.root_start_cluster + sb.serial_number)

/* This macro computes the superblock checksum.   */
/* Call passing a pointer to a struct SuperBlock. */
#define LEANFS_SB_CHECKSUM_PTR(sb)  (sb->magic + sb->fs_version + sb->log_bytes_per_sector + sb->log_sectors_per_cluster + sb->backup_super + sb->clusters_count + sb->bitmap_start_cluster + sb->root_start_cluster + sb->serial_number)

/******************************************************************************/
/*                                                                            */
/*                           Driver related stuff                             */
/*                                                                            */
/******************************************************************************/


/* A buffer containing a whole cluster */
struct Buffer
{
	blkcnt_t  sector; /* First sector of this cluster */
	const struct Volume *v;
	const struct PFile *pf; /* File owning this cluster (NULL if none) */
	unsigned flags;
	unsigned access;
	uint32_t  cluster; /* This buffered cluster number */
	uint8_t  *data; /* Raw content of the cluster */
};


/* This structure represents a mounted LEAN volume */
#define VOLUME_MAGIC 0
#define NUM_BUFFERS 8
struct Volume
{
	uint32_t magic;
	
	/* The hosting block device */
	void    *dev_priv;
	unsigned sector_bytes;
	struct BlockOperations *bop;
	
	/* Super block */
	uint8_t *sb_buf;
	struct SuperBlock *sb;
	
	/* Cluster size precalcs */
	unsigned cluster_bytes;     /* Cluster size in bytes */
	unsigned log_cluster_bytes; /* log2(cluster_bytes) for quick conversion */
	unsigned sectors_per_cluster;
	unsigned icluster_count;    /* Cluster addresses per indirect cluster */
	
	/* Cluster buffers */
	unsigned num_buffers;
	unsigned buffer_access;
	unsigned buffer_hit;
	unsigned buffer_miss;
	struct Buffer buffers[0];
};


struct PFile
{
	struct Inode   inode;
	struct Volume *v;
	uint32_t       first_cluster;
	unsigned       references;
};


#define LEANFS_FILE_MAGIC 0
struct File
{
	struct PFile *pf;           /* Physical informations of this file      */
	int           flags;        /* Opening flags of this file instance     */
	unsigned      references;   /* Number of times this instance is open   */
	uint32_t      magic;        /* Must be FILE_MAGIC for a valid File     */
	off_t         file_pointer; /* The one set by lseek and updated by r/w */
	uint32_t      icluster;
	uint32_t      icluster_first;
};


/* Organize open file cache in a linked list of page-sized tables */
#define FILES_PER_TABLE ((4096 - sizeof(void *)) / sizeof(struct File))
struct FileTable
{
	struct File       f[FILES_PER_TABLE];
	struct FileTable *next;
};


/* Organize inode cache in a linked list of page-sized tables */
#define PFILES_PER_TABLE ((4096 - sizeof(void *)) / sizeof(struct PFile))
struct PFileTable
{
	struct PFile       pf[PFILES_PER_TABLE];
	struct PFileTable *next;
};


#ifndef __READDIR_CALLBACK_T__
/* This callback function allows to store data "readdir" in any structure */
/* format, letting the caller chose the preferred "struct dirent" format. */
/* An idea borrowed from the Linux kernel.                                */
typedef int (*readdir_callback_t)(void *dirent, const char *d_name, int d_namlen, off_t f_pos, ino_t d_fileno, unsigned d_type);
#endif


/* Driver functions.
 * Unless otherwise stated, these functions return 0 on success,
 * or the negative of a standard error code (from errno.h) on failure.
 */


/* alloc.c */
int leanfs_get_file_cluster(struct File *f, uint32_t cluster_index, uint32_t *cluster);
int leanfs_append_cluster(struct File *f, uint32_t *cluster);
int leanfs_create_inode(struct File *parent, uint32_t attributes, const char *name);
int leanfs_delete_clusters(struct File *f, uint32_t new_clusters_count);

/* blockio.c */
int leanfs_read_superblock(struct Volume *v);
int leanfs_sync(struct Volume *v);
int leanfs_readbuf (struct Volume *v, uint32_t cluster, const struct PFile *pf, struct Buffer **buffer);
int leanfs_dirtybuf(struct Buffer *b, bool write_through);

/* dir.c */
int leanfs_readdir(struct File *dir, void *dirent, readdir_callback_t cb);
int leanfs_find   (struct File *dir, const char *name, struct DirEntry *de);
int leanfs_do_link(struct File *dir, uint32_t first_cluster, unsigned ftype, const char *name);
int leanfs_unlink (struct File *parent, const char *name);
int leanfs_mkdir  (struct File *parent, const char *name, mode_t mode);
int leanfs_rmdir  (struct File *parent, const char *name);

/* file.c */
off_t   leanfs_lseek(struct File *f, off_t offset, int whence);
ssize_t leanfs_read (struct File *f, void *buffer, size_t size);
ssize_t leanfs_write(struct File *f, const void *buffer, size_t size);
int     leanfs_ftruncate(struct File *f, off_t length);
struct PFile *leanfs_is_open(const struct Volume *v, uint32_t first_cluster);
void leanfs_validate_pos(const struct PFile *pf);

/* mount.c */
#if LEANFS_CONFIG_TEST
int leanfs_mount(const char *dev, struct Volume **new_v);
#else
int leanfs_mount(struct Device *dev, struct Volume **new_v);
#endif

/* open.c */
int leanfs_open(struct File *__restrict__ parent, struct File **__restrict__ file, const char *name, int flags, ...);
int leanfs_close(struct File *f);
int fs_open(struct Volume *v, const char *file_name, int flags, struct File **f); /* ONLY FOR TEST */

#endif /* #ifndef __FD32_LEANFS_H */
