/* The FreeDOS-32 FAT Driver version 2.0
 * Copyright (C) 2001-2005  Salvatore ISAJA
 *
 * This file "alloc.c" is part of the FreeDOS-32 FAT Driver (the Program).
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
 * \addtogroup fat
 * @{
 */
/**
 * \file
 * \brief Manages allocation of clusters and the File Allocation Table.
 */
#include "fat.h"


/******************************************************************************
 * Seek functions.
 * Find the location of the "n"-th cluster entry in the file allocation table.
 * On success, put the number of the sector containing the entry in "sector"
 * and return the byte offset of the entry in that sector.
 ******************************************************************************/

static int fat12_seek(Volume *v, Cluster n, unsigned fat_num, Sector *sector)
{
	Cluster offset;
	if (n > v->data_clusters + 1) return -ENXIO;
	offset = n + (n >> 1); /* floor(n * 1.5) */
	*sector = fat_num * v->fat_size + v->fat_start + (offset >> v->log_bytes_per_sector);
	return offset & (v->bytes_per_sector - 1); /* % bytes_per_sector */
}


static int fat16_seek(Volume *v, Cluster n, unsigned fat_num, Sector *sector)
{
	Cluster offset;
	if (n > v->data_clusters + 1) return -ENXIO;
	offset = n << 1;
	*sector = fat_num * v->fat_size + v->fat_start + (offset >> v->log_bytes_per_sector);
	return offset & (v->bytes_per_sector - 1); /* % bytes_per_sector */
}


static int fat32_seek(Volume *v, Cluster n, unsigned fat_num, Sector *sector)
{
	Cluster offset;
	if (n > v->data_clusters + 1) return -ENXIO;
	offset = n << 2;
	*sector = fat_num * v->fat_size + v->fat_start + (offset >> v->log_bytes_per_sector);
	return offset & (v->bytes_per_sector - 1); /* % bytes_per_sector */
}


/******************************************************************************
 * Read functions.
 * Read the value of the specified cluster entry of the file allocation table.
 * On success, return the not negative entry value.
 ******************************************************************************/


/**
 * \brief  Reads a cluster entry from the active copy of the FAT.
 * \param  v the volume the FAT to read belongs to;
 * \param  n number of the cluster entry to read.
 * \return The non-negative value of the cluster entry, or a negative error.
 */
static int32_t read_fat_entry(Volume *v, Cluster n)
{
	return v->fat_read(v, n, v->active_fat);
}


int32_t fat12_read(Volume *v, Cluster n, unsigned fat_num)
{
	int     res, offset;
	Sector  sector;
	Cluster c;
	Buffer *b = NULL;

	offset = fat12_seek(v, n, fat_num, &sector);
	if (offset < 0) return offset;
	res = fat_readbuf(v, sector, &b, false);
	if (res < 0) return res;
	if (offset < v->bytes_per_sector - 1)
		c = *((uint16_t *) &b->data[offset + res]);
	else /* the entry spans across two sectors */
	{
		Buffer *b2 = NULL;
		int res2 = fat_readbuf(v, sector + 1, &b2, false);
		if (res2 < 0) return res2;
		c = (Cluster) b->data[offset + res] + ((Cluster) b2->data[res2] << 8);
	}
	/* If the entry number is odd we need the highest 12 bits of "c",
	 * whereas if it's even we need the lowest 12 bits. */
	if (n & 1) c >>= 4; else c &= 0x0FFF;
	if (IS_FAT12_EOC(c)) c = FAT_EOC;
	return c;
}


int32_t fat16_read(Volume *v, Cluster n, unsigned fat_num)
{
	int     res, offset;
	Sector  sector;
	Cluster c;
	Buffer *b = NULL;

	offset = fat16_seek(v, n, fat_num, &sector);
	if (offset < 0) return offset;
	res = fat_readbuf(v, sector, &b, false);
	if (res < 0) return res;
	c = (Cluster) *((uint16_t *) &b->data[offset + res]);
	if (IS_FAT16_EOC(c)) c = FAT_EOC;
	return c;
}


int32_t fat32_read(Volume *v, Cluster n, unsigned fat_num)
{
	int     res, offset;
	Sector  sector;
	Cluster c;
	Buffer *b = NULL;

	offset = fat32_seek(v, n, fat_num, &sector);
	if (offset < 0) return offset;
	res = fat_readbuf(v, sector, &b, false);
	if (res < 0) return res;
	c = (Cluster) *((uint32_t *) &b->data[offset + res]) & 0x0FFFFFFF;
	if (IS_FAT32_EOC(c)) c = FAT_EOC;
	return c;
}


#if FAT_CONFIG_WRITE
/******************************************************************************
 * Write functions.
 * Write the value of the specified cluster entry in the file allocation table.
 ******************************************************************************/


/**
 * \brief  Writes a cluster entry in the FAT.
 * \param  v the volume the FAT to write belongs to;
 * \param  n number of the cluster entry to write;
 * \param  value the new value for the specified cluster entry.
 * \return 0 on success, or a negative error.
 */
/* TODO: FAT mirroring as an option */
static int write_fat_entry(Volume *v, Cluster n, Cluster value)
{
	unsigned k;
	int res;
	for (k = 0; k < v->num_fats; k++)
	{
		res = v->fat_write(v, n, k, value);
		if (res < 0) return res;
	}
	return 0;
}


int fat12_write(Volume *v, Cluster n, unsigned fat_num, Cluster value)
{
	int      res, offset;
	Sector   sector;
	Buffer  *b1 = NULL;
	Buffer  *b2 = NULL;
	uint8_t *lsb, *msb;

	offset = fat12_seek(v, n, fat_num, &sector);
	if (offset < 0) return offset;
	res = fat_readbuf(v, sector, &b1, false);
	if (res < 0) return res;
	lsb = &b1->data[offset + res];
	if (offset < v->bytes_per_sector - 1)
		msb = lsb + 1;
	else /* the entry spans across two sectors */
	{
		res = fat_readbuf(v, sector + 1, &b2, false);
		if (res < 0) return res;
		msb = &b2->data[res];
	}
	if (value == FAT_EOC) value = FAT12_EOC;
	if (n & 1) /* odd entry, highest 12 bits */
	{
		value <<= 4;
		*lsb &= 0x0F;
		*msb  = 0x00;
	}
	else /* even entry, lowest 12 bits */
	{
		value &= 0x0FFF;
		*lsb  = 0x00;
		*msb &= 0xF0;
	}
	*lsb |= (uint8_t) value;
	*msb |= (uint8_t) (value >> 8);
	res = fat_dirtybuf(b1, false);
	if (b2)
	{
		if (res < 0) return res;
		res = fat_dirtybuf(b2, false);
	}
	return res;
}


int fat16_write(Volume *v, Cluster n, unsigned fat_num, Cluster value)
{
	int     res, offset;
	Sector  sector;
	Buffer *b = NULL;

	offset = fat16_seek(v, n, fat_num, &sector);
	if (offset < 0) return offset;
	res = fat_readbuf(v, sector, &b, false);
	if (res < 0) return res;
	if (value == FAT_EOC) value = FAT16_EOC;
	*((uint16_t *) &b->data[offset + res]) = (uint16_t) value;
	return fat_dirtybuf(b, false);
}


int fat32_write(Volume *v, Cluster n, unsigned fat_num, Cluster value)
{
	int     res, offset;
	Sector  sector;
	Buffer *b = NULL;

	offset = fat32_seek(v, n, fat_num, &sector);
	if (offset < 0) return offset;
	res = fat_readbuf(v, sector, &b, false);
	if (res < 0) return res;
	if (value == FAT_EOC) value = FAT32_EOC;
	value &= 0x0FFFFFFF;
	*((uint32_t *) &b->data[offset + res]) &= 0xF0000000;
	*((uint32_t *) &b->data[offset + res]) |= value;
	return fat_dirtybuf(b, false);
}
#endif /* #if FAT_CONFIG_WRITE */


/**
 * \brief   Gets the address of a sector of a file.
 * \param   c            the channel to get the sector of;
 * \param   sector_index index of the sector of the file to get the address of;
 * \param   sector       to receive the address of the sector corresponding to \c sector_index;
 * \return  0 on success, >0 on EOF, or a negative error.
 * \remarks \c c->cluster_index and \c c->cluster are updated to cache the address of the last
 *          cluster of the file reached by this function. On EOF, they relate to the last
 *          cluster of the file, if any.
 */
int fat_get_file_sector(Channel *c, Sector sector_index, Sector *sector)
{
	int res = 0;
	File   *f = c->f;
	Volume *v = f->v;
	if (!f->de_sector && !f->first_cluster) /* FAT12/FAT16 root directory */
	{
		if (sector_index >= v->root_size) return 1;
		*sector = sector_index + v->root_sector;
	}
	else /* File/directory with linked allocation */
	{
		Cluster  cluster_index     = sector_index >> v->log_sectors_per_cluster;
		unsigned sector_in_cluster = sector_index & (v->sectors_per_cluster - 1);
		if (!f->first_cluster) return 1;
		if (!c->cluster_index || (cluster_index < c->cluster_index))
		{
			c->cluster_index = 0;
			c->cluster = f->first_cluster;
		}
		while (c->cluster_index < cluster_index)
		{
			int32_t nc = read_fat_entry(v, c->cluster);
			if (nc < 0) return nc;
			if (nc == FAT_EOC) return 1;
			if ((nc < 2) || (nc > v->data_clusters + 1)) return -EIO;
			c->cluster_index++;
			c->cluster = (Cluster) nc;
		}
		*sector = ((c->cluster - 2) << v->log_sectors_per_cluster)
		        + v->data_start + sector_in_cluster;
	}
	return res;
}


#if FAT_CONFIG_WRITE
/**
 * \brief  Searches for a free cluster from "from" (included) to "to" (excluded).
 * \retval >0 the address of the first free cluster, \c v->next_free updated accordingly;
 * \retval -ENOSPC no free clusters in the specified range;
 * \retval <0 other errors.
 */
static int32_t find_free_cluster_in_range(Volume *v, Cluster from, Cluster to)
{
	Cluster k;
	int32_t res;
	for (k = from; k < to; k++)
	{
		res = read_fat_entry(v, k);
		if (res < 0) return res;
		if (!res)
		{
			v->next_free = (Cluster) k;
			return (int32_t) k;
		}
	}
	return -ENOSPC;
}


/**
 * \brief   Searches a volume for a free cluster.
 * \retval  >0 the address of the first free cluster, \c v->next_free updated accordingly;
 * \retval  -ENOSPC no free clusters in the volume;
 * \retval  <0 other errors.
 * \remarks \c v->next_free is used as a hint, assuming that a free cluster
 *          may be located near to that cluster number.
 */
static int32_t find_free_cluster(Volume *v)
{
	int32_t res;
	Cluster hint = v->next_free;
	if ((hint == FAT_FSI_NA) || (hint < 2) || (hint > v->data_clusters + 1)) hint = 2;
	res = find_free_cluster_in_range(v, hint, v->data_clusters + 2);
	if (res != -ENOSPC) return res;
	res = find_free_cluster_in_range(v, 2, hint);
	if (res != -ENOSPC) return res;
	v->next_free = FAT_FSI_NA;
	return -ENOSPC;
}


/**
 * \brief   Allocates a new cluster and append it to a file.
 * \param   c the open instance of the file to append the cluster to.
 * \return  The address of the new cluster, or a negative error.
 * \remarks This function must be called on write when #fat_get_file_sector reports
 *          EOF and the file needs to be extended. The cached cluster position is
 *          assumed to refer to the last cluster of the file.
 */
int32_t fat_append_cluster(Channel *c)
{
	File   *f = c->f;
	Volume *v = f->v;
	int32_t cluster;
	int res;

	if (!f->de_sector && !f->first_cluster) return -EFBIG; /* FAT12/FAT16 root directory */
	cluster = find_free_cluster(v);
	if (cluster < 0) return cluster;
	if (!f->first_cluster)
	{
		f->first_cluster = cluster;
		f->de.first_cluster_hi = (uint16_t) (cluster >> 16);
		f->de.first_cluster_lo = (uint16_t) cluster;
		f->de_changed = true;
	}
	else
	{
		res = write_fat_entry(v, c->cluster, cluster);
		if (res < 0) return res;
	}
	v->free_clusters--;
	res = write_fat_entry(v, cluster, FAT_EOC);
	if (res < 0) return res;
	return cluster;
}


/**
 * \brief Deletes all clusters of a chain starting from a specified cluster.
 * \param f the file to delete clusters from;
 * \param new_last_cluster the address of the cluster to become the last cluster of the file,
 *                         or zero to delete all clusters;
 * \note  After deletion if the file size is not zero the last cluster is marked with EOC.
 */
int fat_delete_file_clusters(File *f, Cluster new_last_cluster)
{
	int32_t  next;
	int      res;
	Volume  *v = f->v;

	assert(f->de_sector || f->first_cluster); /* Not a FAT12/FAT16 root directory */
	/* Mark the last cluster if needed */
	next = (int32_t) f->first_cluster;
	if (new_last_cluster)
	{
		next = read_fat_entry(v, new_last_cluster);
		if (next < 0) return next;
		res = write_fat_entry(v, new_last_cluster, FAT_EOC);
		if (res < 0) return res;
	}
	else
	{
		f->first_cluster = 0;
		f->de.first_cluster_hi = 0;
		f->de.first_cluster_lo = 0;
		f->de_changed = true;
	}
	return fat_delete_clusters(v, (Cluster) next);
}


int fat_delete_clusters(Volume *v, Cluster from)
{
	int32_t  next;
	int      res;
	/* Delete the cluster chain */
	do
	{
		next = read_fat_entry(v, from);
		if (next < 0) return next;
		res = write_fat_entry(v, from, 0);
		if (res < 0) return res;
		v->free_clusters++;
		from = (Cluster) next;
	}
	while (next != FAT_EOC);
	return 0;
}
#endif /* #if FAT_CONFIG_WRITE */

/* @} */
