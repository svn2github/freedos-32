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
/* This file is "blockio.c".
 * This is the bottom part of the driver, that deals with the hosting
 * block device, providing buffered access to it.
 */
#include "leanfs.h"
//#define NDEBUG
#include <assert.h>


/* Buffer flags */
enum
{
	BUF_VALID = 1 << 0, /* Buffer contains valid data         */
	BUF_DIRTY = 1 << 1  /* Buffer contains data to be written */
};


/* Searches for a volume buffer contaning the specified cluster. */
/* Returns the buffer address on success, or NULL on failure.    */
static struct Buffer *
search_for_buffer(struct Volume *v, uint32_t cluster)
{
	unsigned k;
	for (k = 0; k < v->num_buffers; k++)
		if (v->buffers[k].flags & BUF_VALID)
			if (cluster == v->buffers[k].cluster) return &v->buffers[k];
	return NULL;
}


/* Returns the address of the least recently used buffer of a volume. */
/* It always succeeds in returning a buffer.                          */
static struct Buffer *
find_least_recently_used(struct Volume *v)
{
	unsigned k, lru = 0;
	unsigned min = v->buffer_access;
	for (k = 0; k < v->num_buffers; k++)
		if (v->buffers[k].access < min)
		{
			min = v->buffers[k].access;
			lru = k;
		}
	return &v->buffers[lru];
}


#if LEANFS_CONFIG_WRITE
/* Writes the content of the specified buffer in the hosting block */
/* device, marking the buffer as not dirty.                        */
/* Returns 0 on success, or a negative error code on failure.      */
static int flush_buffer(struct Buffer *b)
{
	if (b->flags & (BUF_VALID | BUF_DIRTY))
	{
		#if LEANFS_CONFIG_TEST
		fseek(b->v->dev_priv, b->sector * b->v->sector_bytes, SEEK_SET);
		size_t res = fwrite(b->data, b->v->sector_bytes, b->v->sectors_per_cluster, b->v->dev_priv);
		#else
		ssize_t res = b->v->bop->write(b->v->dev_priv, b->data, b->sector, b->v->sectors_per_cluster);
		if (res < 0) return res;
		#endif
		if ((unsigned) res != b->v->sectors_per_cluster) return -EIO;
		b->flags &= ~BUF_DIRTY;
	}
	return 0;
}


/* Flushes all buffers of the specified volume, calling flush_buffer. */
/* Returns 0 on success, or a negative error code on failure.         */
int leanfs_sync(struct Volume *v)
{
	unsigned k;
	int      res;
	struct Buffer *b = NULL;

	/* Update the superblock */
	#if LEANFS_CONFIG_TEST
	fseek(v->dev_priv, 1 * v->sector_bytes, SEEK_SET);
	res = fwrite(v->sb_buf, v->sector_bytes, 1, v->dev_priv);
	if (res != 1) return -EIO;
	#else
	res = v->bop->write(v->dev_priv, v->sb_buf, 1, 1);
	if (res < 0) return res;
	#endif

	/* Update the superblock backup */
	res = leanfs_readbuf(v, v->sb->backup_super, NULL, &b);
	if (res < 0) return res;
	memcpy(b->data, v->sb_buf, sizeof(struct SuperBlock));
	res = leanfs_dirtybuf(b, false);
	if (res < 0) return res;

	/* Flush all buffers */
	for (k = 0; k < v->num_buffers; k++)
	{
		res = flush_buffer(&v->buffers[k]);
		if (res < 0) return res;
	}
	return 0;
}


/* Marks a buffer as dirty (i.e. it needs to be committed).   */
/* Returns 0 on success, or a negative error code on failure. */
int leanfs_dirtybuf(struct Buffer *b, bool write_through)
{
	int res = -EINVAL;
	if (b->flags & BUF_VALID)
	{
		b->flags |= BUF_DIRTY;
		res = 0;
		if (write_through) res = flush_buffer(b);
	}
	return res;
}
#endif /* #if LEANFS_CONFIG_WRITE */


/*
 * Performs a buffered read from the hosting block device.
 * If the requested cluster is already buffered, use that buffer.
 * If the cluster is not buffered, we read it from the hosting block device:
 * if "buffer" is NULL, flush the least recently used buffer and use it,
 * if "buffer" is not NULL, flush that buffer and use it (this is useful for
 * sequential access, like when searching in the bitmap).
 * If the cluster belongs to a file, the caller must set "pf" accordingly,
 * otherwise "pf" must be NULL (this is for fsync and close). A cluster can
 * belong to only one file or no file at all (superblock, bitmap).
 * - On success, puts the address of the buffer in "buffer" and returns the
 *   non-negative offset where the cluster starts in the "buffer->data" array.
 * - On failure, returns a negative error code.
 */
int leanfs_readbuf(struct Volume *v, uint32_t cluster, const struct PFile *pf, struct Buffer **buffer)
{
	int            res;
	struct Buffer *b;
	
	assert(v);
	assert(buffer);
	
	v->buffer_hit++;
	b = search_for_buffer(v, cluster);
	if (!b)
	{
		v->buffer_hit--;
		v->buffer_miss++;
		b = *buffer;
		if (!b)
			b = find_least_recently_used(v);
	
		#if LEANFS_CONFIG_WRITE
		res = flush_buffer(b);
		if (res < 0) return res;
		#endif
	
		b->flags = 0;
		b->sector = cluster << v->sb->log_sectors_per_cluster; /* = cluster * sectors_per_cluster */
		#if LEANFS_CONFIG_TEST
		fseek(b->v->dev_priv, b->sector * b->v->sector_bytes, SEEK_SET);
		size_t res = fread(b->data, b->v->sector_bytes, b->v->sectors_per_cluster, b->v->dev_priv);
		#else
		res = v->bop->read(v->dev_priv, b->data, b->sector, v->sectors_per_cluster);
		if (res < 0) return res;
		#endif
		if ((unsigned) res != v->sectors_per_cluster) return -EIO;
		b->flags   = BUF_VALID;
		b->cluster = cluster;
		b->pf      = pf;
	}
	b->access = ++v->buffer_access;
	*buffer = b;
	return 0;
}


int leanfs_read_superblock(struct Volume *v)
{
	//TODO: Free garbage in mount.c/free_volume
	int res;
	v->sb_buf = (uint8_t *) malloc(v->sector_bytes);
	if (!v->sb_buf) return -ENOMEM;
	#if LEANFS_CONFIG_TEST
	fseek(v->dev_priv, 1 * v->sector_bytes, SEEK_SET);
	res = fread(v->sb_buf, v->sector_bytes, 1, v->dev_priv);
	if (res != 1) return -EIO;
	#else
	res = v->bop->read(v->dev_priv, v->sb_buf, 1, 1);
	if (res < 0) return res;
	#endif
	v->sb = (struct SuperBlock *) v->sb_buf;
	return 0;
}
