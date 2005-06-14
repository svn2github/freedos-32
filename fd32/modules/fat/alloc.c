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
/** \file
 * The part of the driver that manages allocation of clusters
 * and the File Allocation Table.
 */
#include "fat.h"


/* Gets the address of the "sector_index"-th sector of
 * the specified file in "sector".
 * Returns 0 on success, >0 on EOF, or a negative error.
 */
int fat_get_file_sector(Channel *c, Sector sector_index, Sector *sector)
{
	int res = 0;
	File   *f = c->f;
	Volume *v = f->v;
	if (!f->de_sector && !f->first_cluster)
	{
		*sector = sector_index + v->root_sector;
		if (sector_index >= v->root_size) res = 1;
	}
	else
	{
		Cluster  cluster_index     = sector_index >> v->log_sectors_per_cluster;
		unsigned sector_in_cluster = sector_index & (v->sectors_per_cluster - 1);
		if (!c->cluster_index || (cluster_index < c->cluster_index))
		{
			c->cluster_index = 0;
			c->cluster = f->first_cluster;
		}
		while (c->cluster_index < cluster_index)
		{
			int32_t nc = v->fat_read(v, c->cluster, v->active_fat);
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
