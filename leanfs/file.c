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
/* This file is "file.c".
 * It contains the facilities to access open files.
 */
#include "leanfs.h"
//#define NDEBUG
#include <assert.h>


/* Backend for the "lseek" system call.      */
/* Returns a negative error code on failure. */
off_t leanfs_lseek(struct File *f, off_t offset, int whence)
{
	off_t res = -EINVAL;
	if (f)
	{
		switch (whence)
		{
			case SEEK_SET: res = offset; break;
			case SEEK_CUR: res = offset + f->file_pointer; break;
			case SEEK_END: res = offset + f->pf->inode.file_size; break;
		}
		if (res >= 0) f->file_pointer = res;
	}
	return res;
}


/* Backend for the "read" system call.       */
/* Returns a negative error code on failure. */
ssize_t leanfs_read(struct File *f, void *buffer, size_t size)
{
	off_t          offset;
	size_t         k = 0, count;
	unsigned       byte_in_cluster;
	int            res;
	uint32_t       cluster, cluster_index;
	struct Buffer *b  = NULL;
	struct PFile  *pf = f->pf;
	struct Volume *v  = pf->v;

	if (!f || !buffer) return -EINVAL;
	if (f->magic != LEANFS_FILE_MAGIC) return -EBADF;
	assert(f->file_pointer >= 0);
	if (((f->flags & O_ACCMODE) != O_RDONLY) && ((f->flags & O_ACCMODE) != O_RDWR))
		return -EBADF;
	offset = f->file_pointer;

	while ((k < size) && (offset < pf->inode.file_size))
	{
		/* Locate the current cluster and byte in cluster position */
		cluster_index = (offset + sizeof(struct Inode)) >> v->log_cluster_bytes; /* / v->cluster_bytes */
		res = leanfs_get_file_cluster(f, cluster_index, &cluster);
		if (res < 0) return res;
		byte_in_cluster = (offset + sizeof(struct Inode)) & (v->cluster_bytes - 1); /* % v->cluster_bytes */

		/* Read as much as we can in that cluster with a single operation. */
		/* However, don't read past EOF or more than "size" bytes.         */
		count = v->cluster_bytes - byte_in_cluster;
		if (offset + count >= pf->inode.file_size)
			count = pf->inode.file_size - offset;
		if (k + count > size)
			count = size - k;

		/* Fetch the cluster, read data and continue */
		res = leanfs_readbuf(v, cluster, pf, &b);
		if (res < 0) return res;
		memcpy((uint8_t *) buffer + k, b->data + byte_in_cluster, count);
		offset += count;
		k      += count;
	}
	/* Successful exit, update file last-access time-stamp if required */
	#if LEANFS_CONFIG_WRITE
	if (!(f->flags & O_NOATIME))
		pf->inode.acc_time = (uint32_t) time(NULL);
	#endif
	f->file_pointer = offset;
	return (ssize_t) k;
}


#if LEANFS_CONFIG_WRITE
/* Performs the underlying work for the "write" and "ftruncate" services.
 * If "buffer" is NULL zeroes are written into the file.
 * The "offset" must be *less or equal* than the file size (not past EOF).
 * Returns the number of bytes written on success, or a negative error.
 */
static ssize_t do_write(struct File *f, const void *buffer, size_t size, off_t offset)
{
	unsigned       byte_in_cluster;
	size_t         k = 0, count;
	int            res;
	uint32_t       cluster, cluster_index;
	struct Buffer *b  = NULL;
	struct PFile  *pf = f->pf;
	struct Volume *v  = pf->v;

	if (size == 0) return 0;
	assert((offset >= 0) && (offset <= pf->inode.file_size));
	while (k < size)
	{
		/* Locate the current cluster and byte in cluster position.            */
		/* Here the file position may be equal to the file size, but not more. */
		/* In this case, we need to allocate a new cluster for the file.       */
		cluster_index = (offset + sizeof(struct Inode)) >> v->log_cluster_bytes; /* / v->cluster_bytes */
		assert(cluster_index <= pf->inode.clusters_count);
		if (cluster_index < pf->inode.clusters_count)
			res = leanfs_get_file_cluster(f, cluster_index, &cluster);
		else
			res = leanfs_append_cluster(f, &cluster);
		if (res < 0) return res;
		byte_in_cluster = (offset + sizeof(struct Inode)) & (v->cluster_bytes - 1); /* % v->cluster_bytes */

		/* Write as much as we can in that cluster with a single operation. */
		/* However, don't write more than "size" bytes. Extend the file up  */
		/* to the end of that cluster if needed.                            */
		count = v->cluster_bytes - byte_in_cluster;
		if (k + count > size) count = size - k;
		if (offset + count >= pf->inode.file_size)
			pf->inode.file_size = offset + count;

		/* Fetch the cluster, write data, commit and continue */
		res = leanfs_readbuf(v, cluster, pf, &b);
		if (res < 0) return res;
		if (buffer)
			memcpy(b->data + byte_in_cluster, (const uint8_t *) buffer + k, count);
		else
			memset(b->data + byte_in_cluster, 0, count);
		res = leanfs_dirtybuf(b, f->flags & O_FSYNC);
		if (res < 0) return res;
		offset += count;
		k      += count;
	}
	/* Successful exit, update file time-stamps */
	pf->inode.mod_time = (uint32_t) time(NULL);
	pf->inode.acc_time = pf->inode.mod_time;
	return (ssize_t) k;
}


/* Backend for the "write" system call.      */
/* Returns a negative error code on failure. */
ssize_t leanfs_write(struct File *f, const void *buffer, size_t size)
{
	ssize_t res, num_written = 0;

	if (!f || !buffer) return -EINVAL;
	if (f->magic != LEANFS_FILE_MAGIC) return -EBADF;
	assert(f->file_pointer >= 0);
	if (((f->flags & O_ACCMODE) != O_WRONLY) && ((f->flags & O_ACCMODE) != O_RDWR))
		return -EBADF;
	if (!size) return 0;

	if (f->flags & O_APPEND) f->file_pointer = f->pf->inode.file_size;
	/* If the file pointer is past the EOF, zero pad until the file pointer. */
	/* Note that the file pointer *at* EOF is allowed and needs no padding.  */
	if (f->file_pointer > f->pf->inode.file_size)
	{
		res = do_write(f, NULL, f->file_pointer - f->pf->inode.file_size, f->pf->inode.file_size);
		if (res < f->file_pointer - f->pf->inode.file_size) return res;
		num_written = res;
	}
	/* Now write the buffer */
	res = do_write(f, buffer, size, f->file_pointer);
	if (res < 0) return res;
	f->file_pointer += res;
	num_written     += res;
	return num_written;
}


/* Backend for the "ftruncate" system call.
 * Truncate or extends (by zero padding) a file to the specified length.
 * Returns 0 on success, or a negative error
 */
int leanfs_ftruncate(struct File *f, off_t length)
{
	struct PFile *pf;
	if (!f || (length < 0)) return -EINVAL;
	if (((f->flags & O_ACCMODE) != O_WRONLY) && ((f->flags & O_ACCMODE) != O_RDWR))
		return -EBADF;

	pf = f->pf;
	if (length < pf->inode.file_size)
	{
		uint32_t new_clusters_count = (length + sizeof(struct Inode) + pf->v->cluster_bytes - 1)
		                            >> pf->v->log_cluster_bytes;
		int res = leanfs_delete_clusters(f, new_clusters_count);
		if (res < 0) return res;
		pf->inode.file_size = length;
		leanfs_validate_pos(pf);
	}
	else if (length > pf->inode.file_size)
		return do_write(f, NULL, length - pf->inode.file_size, pf->inode.file_size);
	return 0;
}
#endif /* #if LEANFS_CONFIG_WRITE */


int leanfs_fstat(struct File *file, struct stat *buf)
{
	if (!file || !buf) return -EINVAL;
	struct PFile *pf = file->pf;
	buf->st_mode = (mode_t) 0; //TODO: Convert attributes to mode
	buf->st_ino = (ino_t) pf->first_cluster;
	buf->st_dev = (dev_t) pf->v;
	buf->st_nlink = (nlink_t) pf->inode.links_count;
	buf->st_uid = (uid_t) 0; //pf->inode.uid;
	buf->st_gid = (gid_t) 0; //pf->inode.gid;
	buf->st_size = (off_t) pf->inode.file_size;
	buf->st_atim.tv_sec = (time_t) pf->inode.acc_time;
	buf->st_atim.tv_nsec = (unsigned long int) 0;
	buf->st_mtim.tv_sec = (time_t) pf->inode.mod_time;
	buf->st_mtim.tv_nsec = (unsigned long int) 0;
	buf->st_ctim.tv_sec = (time_t) pf->inode.cre_time;
	buf->st_ctim.tv_nsec = (unsigned long int) 0;
	buf->st_blocks = (blkcnt_t) pf->inode.clusters_count
	               << (pf->v->sb->log_sectors_per_cluster + pf->v->sb->log_bytes_per_sector - 9);
	buf->st_blksize = (unsigned int) pf->v->cluster_bytes;
	return 0;
}
