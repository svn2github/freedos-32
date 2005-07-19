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

#if 1 /* Old-style, zero-based access modes (O_RDONLY = 0, O_WRONLY = 1, O_RDWR = 2) */
 #define IS_NOT_READABLE(c)  (((c->flags & O_ACCMODE) != O_RDONLY) && ((c->flags & O_ACCMODE) != O_RDWR))
 #define IS_NOT_WRITEABLE(c) (((c->flags & O_ACCMODE) != O_RDWR) && ((c->flags & O_ACCMODE) != O_WRONLY))
#else /* New-style, bitwise-distinct access modes (O_RDWR = O_RDONLY | O_WRONLY) */
 #define IS_NOT_READABLE(c)  (!(c->flags & O_RDONLY))
 #define IS_NOT_WRITEABLE(c) (!(c->flags & O_WRONLY))
#endif


#if !FAT_CONFIG_FD32
/// Converts a broken-down time to a 16-bit DOS date
static int pack_dos_date(const struct tm *tm)
{
	assert((tm->tm_mday >= 1)  && (tm->tm_mday <= 31));
	assert((tm->tm_mon  >= 0)  && (tm->tm_mon  <= 11));
	assert((tm->tm_year >= 80) && (tm->tm_mon  <= 207));
	return tm->tm_mday | ((tm->tm_mon + 1) << 5) | ((tm->tm_year - 80) << 9);
}


/// Converts a broken-down time to a 16-bit DOS time
static int pack_dos_time(const struct tm *tm)
{
	assert((tm->tm_sec  >= 0) && (tm->tm_sec  <= 60));
	assert((tm->tm_min  >= 0) && (tm->tm_min  <= 59));
	assert((tm->tm_hour >= 0) && (tm->tm_hour <= 23));
	return (tm->tm_sec >> 1) | (tm->tm_min << 5) | (tm->tm_hour << 11);
}


/// Converts a DOS date and time to a broken-down time
static void unpack_dos_time_stamps(int dos_date, int dos_time, int dos_hundreths, struct tm *tm)
{
	tm->tm_mday = dos_date & 0x1F;
	tm->tm_mon  = ((dos_date >> 5) & 0x0F) - 1;
	tm->tm_year = (dos_date >> 9) & 0x7F;
	tm->tm_sec  = (dos_time & 0x1F) + dos_hundreths / 100;
	tm->tm_min  = (dos_time >> 5) & 0x3F;
	tm->tm_hour = (dos_time >> 11) & 0x1F;
}
#endif


#if FAT_CONFIG_WRITE
/**
 * \brief Formats the passed DOS timestamps from the current date and time.
 * \param dos_time word to receive the system time in DOS format;
 * \param dos_date word to receive the system date in DOS format;
 * \param dos_hund byte to receive the hundredths of seconds past the time in \c dos_time.
 */
void fat_timestamps(uint16_t *dos_time, uint16_t *dos_date, uint8_t *dos_hund)
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
	struct tm tm;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &tm);
	if (dos_date) *dos_date = pack_dos_date(&tm);
	if (dos_time) *dos_time = pack_dos_time(&tm);
	if (dos_hund) *dos_hund = (tm.tm_sec & 1) * 100 + tv.tv_usec / 10000;
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
	if (IS_NOT_READABLE(c)) return -EBADF;
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
ssize_t fat_do_write(Channel *c, const void *buffer, size_t size, off_t offset)
{
	unsigned byte_in_sector;
	size_t   k = 0, count;
	int      res;
	Sector   sector, sector_index;
	Buffer  *b = NULL;
	File    *f = c->f;
	Volume  *v = f->v;

	if (offset < 0) return -EINVAL;
	if (!size) return 0;
	if (f->de.attr & FAT_ADIR)
	{
		if (offset + size > UINT16_MAX * sizeof(struct fat_direntry)) return -EFBIG;
	}
	else
	{
		if ((int64_t) offset + (int64_t) size > (int64_t) UINT32_MAX) return -EFBIG;
	}
	while (k < size)
	{
		/* Locate the current sector and byte in sector position.
		 * While the file offset is at or beyond EOF, a new cluster is appended
		 * to the file, its content is undefined. */
		sector_index = offset >> v->log_bytes_per_sector;
		res = fat_get_file_sector(c, sector_index, &sector);
		if (res > 0) /* EOF */
		{
			res = fat_append_cluster(c);
			if ((res == -ENOSPC) && k) break;
			if (res < 0) return res;
			continue;
		}
		if (res < 0) return res;
		byte_in_sector = offset & (v->bytes_per_sector - 1);

		/* Write as much as we can in that sector with a single operation.
		 * However, don't write more than "size" bytes. Extend the file up
		 * to the end of that sector if needed. */
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
 * \param  c      the regular file (not a directory) instance to write to;
 * \param  buffer pointer to the data to be written;
 * \param  size   the number of bytes to write.
 * \return The number of bytes written on success (may be less than \c size), or a negative error.
 */
ssize_t fat_write(Channel *c, const void *buffer, size_t size)
{
	ssize_t res, num_written = 0;
	File *f;

	if (!c || !buffer) return -EFAULT;
	if (c->magic != FAT_CHANNEL_MAGIC) return -EBADF;
	f = c->f;
	assert(c->file_pointer >= 0);
	if (IS_NOT_WRITEABLE(c)) return -EBADF;
	if (f->de.attr & FAT_ADIR) return -EBADF;
	if (!size) return 0;
	if (c->flags & O_APPEND) c->file_pointer = f->de.file_size;
	/* If the file pointer is past the EOF, zero pad until the file pointer.
	 * Note that the file pointer *at* EOF is allowed and needs no padding. */
	if (c->file_pointer > f->de.file_size)
	{
		res = fat_do_write(c, NULL, c->file_pointer - f->de.file_size, f->de.file_size);
		if (res < c->file_pointer - f->de.file_size) return res; /* includes res < 0 */
		num_written = res;
	}
	/* Now write the buffer */
	res = fat_do_write(c, buffer, size, c->file_pointer);
	if (res < 0) return res;
	c->file_pointer += res;
	num_written     += res;
	return num_written;
}


/**
 * \brief  Backend for the "ftruncate" system call.
 * \param  c      the regular file (not a directory) instance to truncate or extend;
 * \param  length the desired new length for the file;
 * \return 0 on success, or a negative error.
 * \note   If \c length is greater than the file size, the file is extended
 *         by zero padding. If it is smaller it is truncated to the specified
 *         size. If \c length is equal to the file size, this is a no-op.
 */
int fat_ftruncate(Channel *c, off_t length)
{
	File *f;
	if (!c || (length < 0)) return -EINVAL;
	if (IS_NOT_WRITEABLE(c)) return -EBADF;
	f = c->f;
	if (f->de.attr & FAT_ADIR) return -EBADF;
	if (length < f->de.file_size)
	{
		int res;
		Cluster new_last_cluster = 0;
		Sector unused;
		if (length)
		{
			/* Find the cluster containing the last byte with the new length */
			res = fat_get_file_sector(c, (length - 1) >> f->v->log_bytes_per_sector, &unused);
			if (res < 0) return res;
			if (res > 0) return -EIO; /* EOF: file size not matched by the cluster chain */
			new_last_cluster = c->cluster;
		}
		res = fat_delete_clusters(f, new_last_cluster);
		if (res < 0) return res;
		fat_timestamps(&f->de.mod_time, &f->de.mod_date, NULL);
		if (!(c->flags & O_NOATIME)) f->de.acc_date = f->de.mod_date;
		f->de.file_size = length;
		f->de.attr |= FAT_AARCHIV;
		f->de_changed = true;
		if (c->flags & O_SYNC)
		{
			res = fat_sync(f->v);
			if (res < 0) return res;
		}
		/* Reset the cached cluster address for all open instances of the file */
		for (c = (Channel *) f->v->channels_open.begin; c; c = c->next)
			if (c->f == f) c->cluster_index = 0;
	}
	else if (length > f->de.file_size)
		return fat_do_write(c, NULL, length - f->de.file_size, f->de.file_size);
	return 0;
}
#endif /* #if FAT_CONFIG_WRITE */


#if !FAT_CONFIG_FD32
int fat_fstat(Channel *c, struct stat *buf)
{
	struct tm tm;
	File *f;
	if (!c || !buf) return -EFAULT;
	f = c->f;
	buf->st_mode  = 0; //TODO: Convert attributes to mode
	buf->st_ino   = (ino_t) f->first_cluster;
	buf->st_dev   = (dev_t) f->v;
	buf->st_nlink = 0;
	buf->st_uid   = 0;
	buf->st_gid   = 0;
	buf->st_size  = 0;
	unpack_dos_time_stamps(f->de.acc_date, 0, 0, &tm);
	buf->st_atim.tv_sec  = mktime(&tm);
	buf->st_atim.tv_nsec = 0;
	unpack_dos_time_stamps(f->de.mod_date, f->de.mod_time, 0, &tm);
	buf->st_mtim.tv_sec  = mktime(&tm);
	buf->st_mtim.tv_nsec = 0;
	unpack_dos_time_stamps(f->de.cre_date, f->de.cre_time, f->de.cre_time_hund, &tm);
	buf->st_ctim.tv_sec  = mktime(&tm);
	buf->st_ctim.tv_nsec = (f->de.cre_time_hund % 100) * 10000000;
	buf->st_blocks  = 0;
	buf->st_blksize = f->v->bytes_per_sector;
	if (!(f->de.attr & FAT_ADIR))
	{
		buf->st_blocks = (f->de.file_size + 511) >> 9;
		buf->st_size  = (off_t) f->de.file_size;
	}
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
