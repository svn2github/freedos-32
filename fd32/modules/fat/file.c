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
/**
 * \file
 * \brief Facilities to access open files.
 */
#include "fat.h"


#if FAT_CONFIG_WRITE
/**
 * \brief Formats the passed DOS timestamps from the current date and time.
 * \param dos_time word to receive the system time in DOS format;
 * \param dos_date word to receive the system date in DOS format;
 * \param dos_hund byte to receive the hundredths of seconds past the time in \c dos_time.
 */
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
#endif


/**
 * \brief  Backend for the "lseek" system call.
 * \param  c      the file instance to seek into;
 * \param  offset new byte offset for the file pointer according to \c whence;
 * \param  whence can be \c SEEK_SET, \c SEEK_CUR or \c SEEK_END; the latter
 *                is not allowed for directories.
 * \return On success, the new byte offset from the beginning of the file,
 *         or a negative error.
 */
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


/**
 * \brief  Backend for the "read" system call.
 * \param  c      the file instance to read from;
 * \param  buffer pointer to a buffer to receive the data;
 * \param  size   the number of bytes to read;
 * \return The number of bytes read on success (may be less than \c size), or a negative error.
 */
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
/**
 * \brief  Performs the underlying work for the "write" and "ftruncate" services.
 * \param  c      the file instance to write to;
 * \param  buffer pointer to the data to be written, or NULL to write zeroes;
 * \param  size   the number of bytes to write;
 * \param  offset the file offset where to write, that must be less or equal
 *                than the file size (not past EOF).
 * \return The number of bytes written on success (may be less than \c size), or a negative error.
 */
static ssize_t do_write(Channel *c, const void *buffer, size_t size, off_t offset)
{
	unsigned byte_in_sector;
	size_t  k = 0, count;
	int  res;
	Sector sector, sector_index;
	Buffer *b = NULL;
	File   *f = c->f;
	Volume *v = f->v;

	if (!size) return 0;
	assert(offset >= 0);
	while (k < size)
	{
		/* Locate the current sector and byte in sector position.
		 * Here the file position may be at, but not beyond, EOF.
		 * In this case, we need to allocate a new cluster for the file. */
		sector_index = offset >> v->log_bytes_per_sector;
		res = fat_get_file_sector(c, sector_index, &sector);
		if (res > 0) /* EOF */
		{
			res = fat_append_cluster(c);
			if (res < 0) return res;
			res = fat_get_file_sector(c, sector_index, &sector);
		}
		if (res < 0) return res;
		byte_in_sector = offset & (v->bytes_per_sector - 1);

		/* Write as much as we can in that cluster with a single operation.
		 * However, don't write more than "size" bytes. Extend the file up
		 * to the end of that cluster if needed. */
		count = v->bytes_per_sector - byte_in_sector;
		if (k + count > size) count = size - k;
		if (!(f->de.attr & FAT_ADIR) && (offset + count >= f->de.file_size))
			f->de.file_size = offset + count;

		/* Fetch the sector, write data, commit and continue */
		res = fat_readbuf(v, sector, &b, false);
		if (res < 0) return res;
		if (buffer)
			memcpy(b->data + res + byte_in_sector, (const uint8_t *) buffer + k, count);
		else
			memset(b->data + res + byte_in_sector, 0, count);
		res = fat_dirtybuf(b, c->flags & O_FSYNC);
		if (res < 0) return res;
		offset += count;
		k      += count;
	}
	/* Successful exit, update time-stamps if required */
	if (!(f->de.attr & FAT_ADIR))
	{
		fat_timestamps(&f->de.mod_time, &f->de.mod_date, NULL);
		if (!(c->flags & O_NOATIME)) f->de.acc_date = f->de.mod_date;
		f->de.attr |= FAT_AARCHIV;
		f->de_changed = true;
	}
	if (c->flags & O_SYNC)
	{
		res = fat_sync(v);
		if (res < 0) return res;
	}
	return (ssize_t) k;
}


/**
 * \brief  Backend for the "write" system call.
 * \param  c      the file instance to write to;
 * \param  buffer pointer to the data to be written;
 * \param  size   the number of bytes to write.
 * \return The number of bytes written on success (may be less than \c size), or a negative error.
 * \note   Due to the FAT file system design, O_APPEND is ignored for directories.
 */
ssize_t fat_write(Channel *c, const void *buffer, size_t size)
{
	ssize_t res, num_written = 0;
	File *f;

	if (!c || !buffer) return -EFAULT;
	if (c->magic != FAT_CHANNEL_MAGIC) return -EBADF;
	assert(c->file_pointer >= 0);
	if (((c->flags & O_ACCMODE) != O_WRONLY) && ((c->flags & O_ACCMODE) != O_RDWR))
		return -EBADF;
	if (!size) return 0;
	f = c->f;

	if (!(f->de.attr & FAT_ADIR) && (c->flags & O_APPEND))
		c->file_pointer = f->de.file_size;
	/* If the file pointer is past the EOF, zero pad until the file pointer.
	 * Note that the file pointer *at* EOF is allowed and needs no padding. */
	if (c->file_pointer > f->de.file_size)
	{
		res = do_write(c, NULL, c->file_pointer - f->de.file_size, f->de.file_size);
		if (res < c->file_pointer - f->de.file_size) return res;
		num_written = res;
	}
	/* Now write the buffer */
	res = do_write(c, buffer, size, c->file_pointer);
	if (res < 0) return res;
	c->file_pointer += res;
	num_written     += res;
	return num_written;
}


#if 0
/**
 * \brief  Backend for the "ftruncate" system call.
 * \param  c      the file instance to truncate or extend;
 * \param  length the desired new length for the file;
 * \return 0 on success, or a negative error.
 * \note   If \c length is greater than the file size, the file is extended
 *         by zero padding. If it is smaller it is truncated to the specified
 *         size. If \c length is equal to the file size, this is a no-op.
 * \note   Due to the FAT file system design, this is illegal for directories.
 */
int fat_ftruncate(Channel *c, off_t length)
{
	File *f;
	if (!c || (length < 0)) return -EINVAL;
	if (((c->flags & O_ACCMODE) != O_WRONLY) && ((c->flags & O_ACCMODE) != O_RDWR))
		return -EBADF;
	f = c->f;
	if (f->de.attr & FAT_ADIR) return -EBADF;
	if (length < f->de.file_size)
	{
		Channel *d;
		Volume *v = f->v;
		/* TODO: MISSING CLUSTER_BYTES */
		Cluster new_clusters_count = (length + v->cluster_bytes - 1) >> v->log_cluster_bytes;
		int res = fat_delete_clusters(f, new_clusters_count);
		if (res < 0) return res;
		f->de.file_size = length;
		/* Syncronize the cached file position of all open instances of a file */
		/* TODO: NONSENSE */
		for (d = v->channels_open; d; d = d->next)
			if ((d->f == c->f) && (d->file_pointer > c->file_pointer))
			{
				d->cluster_index = c->cluster_index;
				d->cluster = c->cluster;
			}
	}
	else if (length > f->de.file_size)
		return do_write(f, NULL, length - f->de.file_size, f->de.file_size);
	return 0;
}
#endif
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


/**
 * \brief  Backend for the "get attributes" system call.
 * \param  c the file instance;
 * \param  a buffer to receive the attributes.
 * \return 0 on success, or a negative error.
 */
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
/**
 * \brief  Backend for the "set attributes" system call.
 * \param  c the file instance;
 * \param  a buffer containing the new attributes.
 * \return 0 on success, or a negative error.
 */
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
