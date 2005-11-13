/* The FreeDOS-32 ISO 9660 Driver version 0.2
 * Copyright (C) 2005  Salvatore ISAJA
 *
 * This file "file.c" is part of the FreeDOS-32 ISO 9660 Driver (the Program).
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
#include "iso9660.h"


/* Backend for the "lseek" system call. */
off_t iso9660_lseek(File *f, off_t offset, int whence)
{
	off_t res = -EINVAL;
	if (f)
	{
		switch (whence)
		{
			case SEEK_SET: res = offset; break;
			case SEEK_CUR: res = offset + f->file_pointer; break;
			case SEEK_END: res = offset + f->data_length; break;
		}
		if (offset >= 0) f->file_pointer = res;
	}
	return res;
}


/* Backend for the "read" system call. */
ssize_t iso9660_read(File *f, void *buffer, size_t size)
{
	off_t    offset;
	size_t   k = 0, count;
	Block    logical_block;
	unsigned byte_in_block;
	int      res;
	Buffer *b;
	Volume *v;

	if (!f || !buffer) return -EINVAL;
	if (f->magic != ISO9660_FILE_MAGIC) return -EBADF;
	assert(f->file_pointer >= 0);
	if ((f->open_flags & O_ACCMODE) != O_RDONLY) return -EBADF;
	v = f->dentry->v;
	offset = f->file_pointer;

	while ((k < size) && (offset < f->data_length))
	{
		/* Locate the current logical block and byte in block position */
		logical_block = f->extent + f->len_ear + (offset >> v->log_block_size);
		byte_in_block = offset & (v->block_size - 1);

		/* Read as much as we can in that logical block with a single operation.
		 * However, don't read past EOF or more than "size" bytes. */
		count = v->block_size - byte_in_block;
		if (offset + count >= f->data_length)
			count = f->data_length - offset;
		if (k + count > size)
			count = size - k;

		/* Fetch the block, read data and continue */
		res = iso9660_readbuf(v, logical_block, &b);
		if (res < 0) return res;
		memcpy((uint8_t *) buffer + k, b->data + res + byte_in_block, count);
		offset += count;
		k      += count;
	}
	f->file_pointer = offset;
	return (ssize_t) k;
}


#if 0
int iso9660_fstat(File *f, struct stat *buf)
{
	if (!f || !buf) return -EINVAL;
	buf->st_mode  = (mode_t) 0;
	buf->st_ino   = (ino_t) f->extent;
	buf->st_dev   = (dev_t) 0; //f->v;
	buf->st_nlink = (nlink_t) 0;
	buf->st_uid   = (uid_t) 0;
	buf->st_gid   = (gid_t) 0;
	buf->st_size  = (off_t) f->data_length;
	buf->st_atim.tv_sec  = (time_t) 0;
	buf->st_atim.tv_nsec = 0ul;
	buf->st_mtim.tv_sec  = (time_t) f->recording_time;
	buf->st_mtim.tv_nsec = 0ul;
	buf->st_ctim.tv_sec  = (time_t) f->recording_time;
	buf->st_ctim.tv_nsec = 0ul;
	buf->st_blocks  = (blkcnt_t) 0;
	buf->st_blksize = (unsigned int) f->v->block_size;
	return 0;
}
#endif


/* Backend for the close system call */
int iso9660_close(File *f)
{
	if ((f->magic != ISO9660_FILE_MAGIC) || !f->references) return -EBADF;
	LOG_PRINTF(("[ISO 9660] iso9660_close. Volume buffers: %u hits, %u misses on %u\n",
		f->dentry->v->buf_hit, f->dentry->v->buf_miss, f->dentry->v->buf_access));
	if (--f->references == 0)
	{
		if (f->dentry) iso9660_dput(f->dentry);
		f->magic = 0;
		list_erase(&f->dentry->v->files_open, (ListItem *) f);
		slabmem_free(&f->dentry->v->files_slab, f);
	}
	return 0;
}


/* Reads a directory record from the open directory "f".
 * The sector buffer used to fetch the directory record is returned in "buffer",
 * the block number and block offset of the directory record are returned
 * in "de_block" and "de_blkofs" if they are not NULL.
 */
int iso9660_do_readdir(File *f, Buffer **buffer, Block *de_block, unsigned *de_blkofs)
{
	struct iso9660_dir_record *dr;
	Volume  *v = f->dentry->v;
	int      res;
	Block    logical_block;
	unsigned byte_in_block;
	off_t    offset = f->file_pointer;
	for (;;)
	{
		if (offset >= f->data_length) return -ENMFILE;
		logical_block = f->extent + f->len_ear + (offset >> v->log_block_size);
		byte_in_block = offset & (v->block_size - 1);
		res = iso9660_readbuf(v, logical_block, buffer);
		if (res < 0) return res;
		dr = (struct iso9660_dir_record *) ((*buffer)->data + res + byte_in_block);
		if (dr->len_dr)
		{
			#if 1
			char n[60];
			unsigned k;
			for (k = 0; k < dr->len_fi; k++) n[k] = dr->file_id[k];
			n[k] = 0;
			LOG_PRINTF(("[ISO 9660] dir: %3u %37s %12u %12u %3u %3u %3u\n",
				dr->len_dr, n, ME(dr->extent), ME(dr->data_length),
				dr->flags, dr->file_unit_size, dr->interleave_gap_size));
			#endif
			offset += dr->len_dr;
			break;
		}
		else offset += v->bytes_per_sector - res - byte_in_block;
	}
	if (de_block)  *de_block  = logical_block;
	if (de_blkofs) *de_blkofs = byte_in_block;
	f->file_pointer = offset;
	return res;
}
