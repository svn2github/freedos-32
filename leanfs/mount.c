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
/* This file is "mount.c".
 * It contains the facilities to mount a LEAN file system volume.
 */
#include "leanfs.h"
//#define NDEBUG
#include <assert.h>


static int free_volume(struct Volume *v)
{
	return -EINVAL;
}


#define ABORT_MOUNT(v, res) { free_volume(v); return res; }


#if 0
static unsigned size_to_bits(unsigned size) /* UNUSED */
{
	unsigned k;
	for (k = 0; !(size & 1); k++, size >>= 1);
	if (size >> 1) return 0; /* More than one '1', not a power of two! */
	return k;
}
#endif


#if LEANFS_CONFIG_TEST
int leanfs_mount(const char *dev, struct Volume **new_v)
#else
int leanfs_mount(struct Device *dev, struct Volume **new_v)
#endif
{
	int res;
	struct Volume *v;
	
	if (!dev || !new_v) return -EINVAL;
	
	v = (struct Volume *) malloc(sizeof(struct Volume) + NUM_BUFFERS * sizeof(struct Buffer));
	if (!v) return -ENOMEM;
	memset(v, 0, sizeof(struct Volume) + NUM_BUFFERS * sizeof(struct Buffer));
	v->num_buffers = NUM_BUFFERS;

	#if LEANFS_CONFIG_TEST
	v->dev_priv = fopen(dev, "r+b");
	if (!v->dev_priv) ABORT_MOUNT(v, -ENOENT);
	res = 512;
	#else
	v->dev_priv = dev->priv;
	res = dev->request(REQ_GET_OPERATIONS, BLOCK_OPERATIONS, &v->bop);
	if (res < 0) ABORT_MOUNT(v, -ENOTBLK);
	
	res = dev->request(REQ_GET_BLOCK_SIZE, dev);
	if (res < 0) res = 512;
	#endif

	/*switch (res)
	{
		case  512: v->sector_bits =  9; break;
		case 1024: v->sector_bits = 10; break;
		case 2048: v->sector_bits = 11; break;
		case 4096: v->sector_bits = 12; break;
		default  : ABORT_MOUNT(v, -EINVAL);
	}*/
	//TODO: Check for valid v->sb->log_bytes_per_sector
	v->sector_bytes = (unsigned) res;

	res = leanfs_read_superblock(v);
	if (res < 0) ABORT_MOUNT(v, res);

	switch (v->sb->log_sectors_per_cluster)
	{
		case 0: case 1: case 2: case 3: break;
		default: ABORT_MOUNT(v, -EINVAL);
	}
	v->log_cluster_bytes   = v->sb->log_bytes_per_sector + v->sb->log_sectors_per_cluster;
	v->cluster_bytes       = 1 << v->log_cluster_bytes;
	v->sectors_per_cluster = 1 << v->sb->log_sectors_per_cluster;
	v->icluster_count      = v->cluster_bytes / sizeof(uint32_t) - 1;

	for (res = 0; res < v->num_buffers; res++)
	{
		v->buffers[res].v    = v;
		v->buffers[res].data = (uint8_t *) malloc(v->cluster_bytes);
		if (!v->buffers[res].data) ABORT_MOUNT(v, -ENOMEM);
	}

	*new_v = v;
	return 0;
}
