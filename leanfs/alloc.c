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
/* This file is "alloc.c".
 * The part of the driver that manages the cluster allocation.
 */
#include "leanfs.h"
//#define NDEBUG
#include <assert.h>


/* Fetches the indirect cluster containing the pointer to the cluster
 * "cluster_index" of the file "f", putting it in the "b" buffer.
 * Returns the non-negative offset of the "uint32_t" containing the pointer.
 * NOTE: On return, f->icluster_first == f->pf->inode.clusters_count means that
 *       EOF has been reached, and the indirect cluster shall be eventually
 *       allocated.
 */
int leanfs_fetch_indirect(struct File *f, uint32_t cluster_index, struct Buffer **b)
{
	assert(f);
	assert(cluster_index > MAX_DIRECT_CLUSTER);
	assert(b);

	struct PFile *pf = f->pf;
	struct Volume *v = pf->v;
	uint32_t icluster       = f->icluster;
	uint32_t icluster_first = f->icluster_first;

	if (!icluster_first || (cluster_index < icluster_first))
	{
		icluster = pf->inode.iclusters_head;
		icluster_first = MAX_DIRECT_CLUSTER + 1;
	}

	while (icluster_first < pf->inode.clusters_count)
	{
		int res = leanfs_readbuf(pf->v, icluster, pf, b);
		if (res < 0)
		{
			f->icluster = f->icluster_first = 0;
			return res;
		}
		if ((cluster_index >= icluster_first) && (cluster_index < icluster_first + v->icluster_count)) break;
		icluster_first += v->icluster_count;
		icluster = *((uint32_t *) (*b)->data + v->icluster_count);
	}

	f->icluster = icluster;
	f->icluster_first = icluster_first;
	return cluster_index - icluster_first;
}


/* Gets the address of the "cluster_index"-th cluster of the file "f"
 * in "cluster". Returns 0 on success, or a negative error.
 */
int leanfs_get_file_cluster(struct File *f, uint32_t cluster_index, uint32_t *cluster)
{
	if (cluster_index == 0)
		*cluster = f->pf->first_cluster;
	else if (cluster_index <= MAX_DIRECT_CLUSTER)
		*cluster = f->pf->inode.dclusters[cluster_index - 1];
	else
	{
		struct Buffer *b;
		int res = leanfs_fetch_indirect(f, cluster_index, &b);
		if (res < 0) return res;
		
		assert(res < f->pf->v->cluster_bytes / sizeof(uint32_t));
		*cluster = *((uint32_t *) b->data + res);
	}
	return 0;
}


#if LEANFS_CONFIG_WRITE
/* Searches the bitmap of the volume "v" for a free cluster and returns
 * its address in "free_cluster". Returns 0 on success, or a negative error.
 */
int leanfs_bitmap_find_free(struct Volume *v, uint32_t *free_cluster)
{
	uint32_t bitmap_cluster;
	struct Buffer *b = NULL;
	int res;
	uint32_t cluster = 0, k, map;

	if (!v->sb->free_clusters_count) return -ENOSPC;

	for (bitmap_cluster = v->sb->bitmap_start_cluster; cluster < v->sb->clusters_count; bitmap_cluster++)
	{
		res = leanfs_readbuf(v, bitmap_cluster, NULL, &b);
		if (res < 0) return res;
		
		for (k = 0; (k < v->cluster_bytes) && (cluster < v->sb->clusters_count); k += 4, cluster += 32)
		{
			map = *((uint32_t *) (b->data + k));
			if (map != 0xFFFFFFFF)
			{
				/* One of these 32 bits is a free cluster */
				unsigned bit;
				for (bit = 0; (bit < 32) && (cluster < v->sb->clusters_count); bit++, cluster++)
					if (!(map & (1 << bit)))
					{
						*free_cluster = cluster;
						return 0;
					}
			}
		}
	}
	/* Should not arrive here, since we have v->sb->free_clusters_count */
	assert(v->sb->free_clusters_count == 0);
	return -ENOSPC;
}


/* Change the bitmap state of the specified "cluster" of the volume "v"
 * to the value of "allocate". Returns 0 on success, or a negative error.
 */
int leanfs_bitmap_set(struct Volume *v, uint32_t cluster, bool allocate)
{
	uint32_t bitmap_cluster = cluster >> (3 + v->log_cluster_bytes); /* = cluster / (v->cluster_bytes * 8); */
	unsigned bit_of_cluster = cluster & (v->cluster_bytes * 8 - 1);
	unsigned int_of_cluster = bit_of_cluster / (sizeof(unsigned) * 8);
	unsigned bit_of_int     = bit_of_cluster % (sizeof(unsigned) * 8);

	struct Buffer *b = NULL;
	int res = leanfs_readbuf(v, bitmap_cluster + v->sb->bitmap_start_cluster, NULL, &b);
	if (res < 0) return res;
	assert(int_of_cluster * sizeof(unsigned) < v->cluster_bytes);

	unsigned *map = (unsigned *) b->data + int_of_cluster;

	if (allocate)
	{
		if (!(*map & (1 << bit_of_int)))
		{
			assert(v->sb->free_clusters_count > 0);
			*map |= 1 << bit_of_int;
			res = leanfs_dirtybuf(b, false);
			if (res < 0) return res;
			v->sb->free_clusters_count--;
		}
	}
	else
	{
		if (*map & (1 << bit_of_int))
		{
			assert(v->sb->free_clusters_count < v->sb->clusters_count);
			*map &= ~(1 << bit_of_int);
			res = leanfs_dirtybuf(b, false);
			if (res < 0) return res;
			v->sb->free_clusters_count++;
		}
	}
	return 0;
}


/* Allocates and appends a new cluster to the file specified by "f".
 * On success, returns 0 and fills "cluster" (if not NULL) with the address
 * of the new cluster. Returns a negative error on failure.
 * NOTE: This function shall not be used to allocate the first cluster
 *       of a file. Use leanfs_create_inode for that.
 */
int leanfs_append_cluster(struct File *f, uint32_t *cluster)
{
	assert(f);
	assert(index);

	struct PFile *pf = f->pf;
	struct Volume *v = pf->v;
	uint32_t  free_cluster;
	uint32_t  index = pf->inode.clusters_count;
	uint32_t *pointer;
	int       res;

	if (index <= MAX_DIRECT_CLUSTER)
		pointer = &pf->inode.dclusters[index - 1];
	else
	{
		if (pf->inode.clusters_count == 271)
			printf("AH\n");
		struct Buffer *b = NULL;
		res = leanfs_fetch_indirect(f, index, &b);
		if (res < 0) return res;
		if (f->icluster_first == index)
		{
			/* Allocate an indirect cluster */
			res = leanfs_bitmap_find_free(v, &free_cluster);
			if (res < 0) return res;
			res = leanfs_bitmap_set(v, free_cluster, true);
			if (res < 0) return res;
			/* Link the new indirect cluster */
			if (index == MAX_DIRECT_CLUSTER + 1)
			{
				assert(b == NULL); /* No indirect cluster fetched */
				pf->inode.iclusters_head = free_cluster;
			}
			else
			{
				/* The "b" buffer contains the last indirect cluster */
				*((uint32_t *) b->data + v->icluster_count) = free_cluster;
				res = leanfs_dirtybuf(b, false);
				if (res < 0) return res;
			}
			res = leanfs_readbuf(v, free_cluster, pf, &b);
			if (res < 0) return res;
			f->icluster = free_cluster;
			assert(res == 0); /* Below we need to write the first address */
		}
		pointer = (uint32_t *) b->data + res;
	}

	res = leanfs_bitmap_find_free(v, &free_cluster);
	if (res < 0) return res;
	res = leanfs_bitmap_set(v, free_cluster, true);
	//TODO: Set error bit
	if (res < 0) return res;
	pf->inode.clusters_count++;
	*pointer = free_cluster;
	if (cluster) *cluster = free_cluster;
	return 0;
}


/* Creates a new inode with the specified "attributes", and a its first
 * hard link "name" in the directory specified by "parent".
 * Returns 0 on success, or a negative error on failure.
 */
int leanfs_create_inode(struct File *parent, uint32_t attributes, const char *name)
{
	int res;
	uint32_t free_cluster;
	struct Inode *i;
	struct Buffer *b = NULL;
	struct Volume *v = parent->pf->v;

	/* Allocate the first cluster and store the struct Inode */
	res = leanfs_bitmap_find_free(v, &free_cluster);
	if (res < 0) return res;
	res = leanfs_bitmap_set(v, free_cluster, true);
	if (res < 0) return res;
	res = leanfs_readbuf(v, free_cluster, NULL, &b);
	if (res < 0) return res;

	/* Initialize and store the inode */
	i = (struct Inode *) b->data;
	memset(i, 0, sizeof(struct Inode));
	i->cre_time = time(NULL);
	i->mod_time = i->cre_time;
	i->acc_time = i->cre_time;
	i->attributes = attributes;
	i->clusters_count = 1;
	i->links_count = 1;
	res = leanfs_dirtybuf(b, false);
	if (res < 0) return res;

	/* Create a hard link for the new inode */
	res = leanfs_do_link(parent, free_cluster, LEANFS_IFMT_TO_FT(attributes), name);
	if (res < 0) return res;
	return 0;
}


/* Deletes the last clusters of the file "f" making it "new_clusters_count"
 * clusters long. The clusters (either data and indirect) are deleted by
 * marking them as free in the bitmap.
 */
int leanfs_delete_clusters(struct File *f, uint32_t new_clusters_count)
{
	assert(f);
	uint32_t index, cluster;
	int res;
	struct PFile *pf = f->pf;
	struct Volume *v = pf->v;

	for (index = new_clusters_count; index < pf->inode.clusters_count; index++)
	{
		res = leanfs_get_file_cluster(f, index, &cluster);
		if (res < 0) return res;
		assert(!f->icluster_first || (f->icluster_first > MAX_DIRECT_CLUSTER));
		res = leanfs_bitmap_set(v, cluster, false);
		if (res < 0) return res;
		if (index && (index == f->icluster_first))
		{
			res = leanfs_bitmap_set(v, f->icluster, false);
			if (res < 0) return res;
		}
	}
	pf->inode.clusters_count = new_clusters_count;
	return 0;
}
#endif /* #if LEANFS_CONFIG_WRITE */
