/* The FreeDOS-32 FAT Driver version 2.0
 * Copyright (C) 2001-2005  Salvatore ISAJA
 *
 * This file "file.c" is part of the FreeDOS-32 FAT Driver (the Program).
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
 * Facilities to access open files.
 */
#include "fat.h"


static void fat_timestamps(uint16_t *dos_time, uint16_t *dos_date, uint8_t *dos_hund)
{
	#if FAT_CONFIG_FD32
	fd32_date_t cur_date;
	fd32_time_t cur_time;
	fd32_get_date(&cur_date);
	fd32_get_time(&cur_time);

	if (dos_date)
		*dos_date = (cur_date.Day & 0x1F) + ((cur_date.Mon & 0x0F) << 5) + (((cur_date.Year - 1980) & 0xFF) << 9);
	if (dos_hund)
		*dos_hund = cur_time.Hund + cur_time.Sec % 2 * 100;
	if (dos_time)
		*dos_time = ((cur_time.Sec / 2) & 0x1F) + ((cur_time.Min  & 0x3F) << 5) + ((cur_time.Hour & 0x1F) << 11);
	#else
	if (dos_date) *dos_date = 0;
	if (dos_time) *dos_time = 0;
	if (dos_hund) *dos_hund = 0;
	#endif
}


/* Backend for the "lseek" system call */
off_t fat_lseek(Channel *c, off_t offset, int whence)
{
	off_t res = -EINVAL;
	if (!c) return -EFAULT;
	if (c->magic != FAT_CHANNEL_MAGIC) return -EBADF;
	switch (whence)
	{
		case SEEK_SET: res = offset; break;
		case SEEK_CUR: res = offset + c->file_pointer; break;
		case SEEK_END:
			if (!(c->f->de.attr & FAT_ADIR))
				res = offset + c->f->de.file_size;
			break;
	}
	if (res < 0) return -EINVAL;
	c->file_pointer = res;
	return res;
}


/* Backend for the "read" system call */
ssize_t fat_read(Channel *c, void *buffer, size_t size)
{
	off_t    offset;
	size_t   k = 0, count;
	unsigned byte_in_sector;
	int      res;
	Sector   sector, sector_index;
	Buffer  *b = NULL;
	File    *f = c->f;
	Volume  *v = f->v;

	if (!c || !buffer) return -EFAULT;
	if (c->magic != FAT_CHANNEL_MAGIC) return -EBADF;
	assert(c->file_pointer >= 0);
	if (((c->flags & O_ACCMODE) != O_RDONLY) && ((c->flags & O_ACCMODE) != O_RDWR))
		return -EBADF;
	offset = c->file_pointer;

	while (k < size)
	{
		if (!(f->de.attr & FAT_ADIR) && (offset >= f->de.file_size)) break; /* EOF */
		/* Locate the current sector and byte in sector position */
		sector_index = offset >> v->log_bytes_per_sector;
		res = fat_get_file_sector(c, sector_index, &sector);
		if (res < 0) return res;
		if (res > 0) break; /* EOF */
		byte_in_sector = offset & (v->bytes_per_sector - 1);

		/* Read as much as we can in that sector with a single operation.
		 * However, don't read past EOF or more than "size" bytes. */
		count = v->bytes_per_sector - byte_in_sector;
		if (!(f->de.attr & FAT_ADIR) && (offset + count >= f->de.file_size))
			count = f->de.file_size - offset;
		if (k + count > size)
			count = size - k;

		/* Fetch the sector, read data and continue */
		res = fat_readbuf(v, sector, &b, false);
		if (res < 0) return res;
		memcpy((uint8_t *) buffer + k, b->data + res + byte_in_sector, count);
		offset += count;
		k      += count;
	}
	/* Successful exit, update file last-access time-stamp if required */
	#if FAT_CONFIG_WRITE
	if (!(c->flags & O_NOATIME) && !(f->de.attr & FAT_ADIR))
	{
		fat_timestamps(NULL, &f->de.acc_date, NULL);
		f->de_changed = true;
	}
	#endif
	c->file_pointer = offset;
	return (ssize_t) k;
}


#if FAT_CONFIG_WRITE
/* Performs the underlying work for the "write" and "ftruncate" services.
 * If "buffer" is NULL zeroes are written into the file.
 * The "offset" must be *less or equal* than the file size (not past EOF).
 * Returns the number of bytes written on success, or a negative error.
 */
static ssize_t do_write(Channel *c, const void *buffer, size_t size, off_t offset)
{
	unsigned       byte_in_cluster;
	size_t         k = 0, count;
	int            res;
	lean_cluster_t cluster, cluster_index;
	Buffer *b = NULL;
	File   *f = c->f;
	Volume *v = f->v;

	if (size == 0) return 0;
	assert((offset >= 0) && (offset <= pf->inode.file_size));
	while (k < size)
	{
		/* Locate the current cluster and byte in cluster position.            */
		/* Here the file position may be equal to the file size, but not more. */
		/* In this case, we need to allocate a new cluster for the file.       */
		cluster_index = (offset + sizeof(struct lean_inode)) >> v->log_cluster_bytes; /* / v->cluster_bytes */
		assert(cluster_index <= pf->inode.clusters_count);
		if (cluster_index < pf->inode.clusters_count)
			res = lean_get_file_cluster(f, cluster_index, &cluster);
		else
			res = lean_append_cluster(f, &cluster);
		if (res < 0) return res;
		byte_in_cluster = (offset + sizeof(struct lean_inode)) & (v->cluster_bytes - 1); /* % v->cluster_bytes */

		/* Write as much as we can in that cluster with a single operation. */
		/* However, don't write more than "size" bytes. Extend the file up  */
		/* to the end of that cluster if needed.                            */
		count = v->cluster_bytes - byte_in_cluster;
		if (k + count > size) count = size - k;
		if (offset + count >= pf->inode.file_size)
			pf->inode.file_size = offset + count;

		/* Fetch the cluster, write data, commit and continue */
		res = lean_readbuf(v, cluster, pf, &b);
		if (res < 0) return res;
		if (buffer)
			memcpy(b->data + byte_in_cluster, (const uint8_t *) buffer + k, count);
		else
			memset(b->data + byte_in_cluster, 0, count);
		res = lean_dirtybuf(b, f->flags & O_FSYNC);
		if (res < 0) return res;
		offset += count;
		k      += count;
	}
	/* Successful exit, update file time-stamps */
	pf->inode.mod_time = (uint32_t) time(NULL);
	pf->inode.acc_time = pf->inode.mod_time;
	return (ssize_t) k;
}


/* Backend for the "write" system call.
 * Due to the FAT file system design, O_APPEND can't work for directories.
 */
ssize_t fat_write(struct File *f, const void *buffer, size_t size)
{
	ssize_t res, num_written = 0;

	if (!f || !buffer) return -EINVAL;
	if (f->magic != FAT_FILE_MAGIC) return -EBADF;
	assert(f->file_pointer >= 0);
	if (((f->flags & O_ACCMODE) != O_WRONLY) && ((f->flags & O_ACCMODE) != O_RDWR))
		return -EBADF;
	if (!size) return 0;

	if (!(f->pf->de.attr & FAT_ADIR) && (f->flags & O_APPEND))
		f->file_pointer = f->pf->de.file_size;
	/* If the file pointer is past the EOF, zero pad until the file pointer.
	 * Note that the file pointer *at* EOF is allowed and needs no padding. */
	if (f->file_pointer > f->pf->inode.file_size)
	{
		res = do_write(f, NULL, f->file_pointer - f->pf->de.file_size, f->pf->de.file_size);
		if (res < f->file_pointer - f->pf->de.file_size) return res;
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
		lean_cluster_t new_clusters_count = (length + sizeof(struct lean_inode) + pf->v->cluster_bytes - 1)
		                                  >> pf->v->log_cluster_bytes;
		int res = leanfs_delete_clusters(f, new_clusters_count);
		if (res < 0) return res;
		pf->inode.file_size = length;
		/* Syncronizes the cached file position of all open instances of a file.
		 * Used to avoid access past the end of file when an instance is truncated. */
		Volume *v = c->f->v;
		Channel *d;
		for (d = v->channels_open; d; d = d->next)
			if ((d->f == c->f) && (d->file_pointer > c->file_pointer))
			{
				d->cluster_index = c->cluster_index;
				d->cluster = c->cluster;
			}
	}
	else if (length > pf->inode.file_size)
		return do_write(f, NULL, length - pf->inode.file_size, pf->inode.file_size);
	return 0;
}
#endif /* #if FAT_CONFIG_WRITE */


#if 0
int fat_fstat(struct File *file, struct stat *buf)
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
#endif


/* Backend for the "get attributes" system call */
int fat_get_attr(Channel *c, fd32_fs_attr_t *a)
{
	File *f;
	if (c->magic != FAT_CHANNEL_MAGIC) return -EBADF;
	f = c->f;
	if (a->Size < sizeof(fd32_fs_attr_t)) return -EINVAL;
	a->Attr  = (WORD) f->de.attr;
	a->MDate = f->de.mod_date;
	a->MTime = f->de.mod_time;
	a->ADate = f->de.acc_date;
	a->CDate = f->de.cre_date;
	a->CTime = f->de.cre_time;
	a->CHund = f->de.cre_time_hund;
	return 0;
}


#if FAT_CONFIG_WRITE
/* Backend for the "set attributes" system call */
int fat_set_attr(Channel *c, const fd32_fs_attr_t *a)
{
	File *f = c->f;
	if (a->Size < sizeof(fd32_fs_attr_t)) return -EINVAL;
	f->de.attr = (uint8_t) a->Attr;
	f->de.mod_date = a->MDate;
	f->de.mod_time = a->MTime;
	f->de.acc_date = a->ADate;
	f->de.cre_date = a->CDate;
	f->de.cre_time = a->CTime;
	f->de.cre_time_hund = (uint8_t) a->CHund;
	f->de_changed = true;
	return 0;
}
#endif
