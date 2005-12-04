/* The FreeDOS-32 ISO 9660 Driver version 0.2
 * Copyright (C) 2005  Salvatore ISAJA
 *
 * This file "volume.c" is part of the FreeDOS-32 ISO 9660 Driver (the Program).
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
//#define NDEBUG
//#include <assert.h>


static void free_volume(Volume *v)
{
	if (v)
	{
		slabmem_destroy(&v->files_slab);
		slabmem_destroy(&v->dentries_slab);
		if (v->buffer.data) mfree(v->buffer.data, v->bytes_per_sector);
		#if ISO9660_CONFIG_FD32
		if (v->blockdev.is_open)
		{
			v->blockdev.bops->close(v->blockdev.handle);
			v->blockdev.is_open = false;
		}
		if (v->blockdev.bops) v->blockdev.bops->request(REQ_RELEASE);
		#else
		if (v->blockdev) fclose(v->blockdev);
		#endif
		v->magic = 0; /* Against bad pointers */
		mfree(v, sizeof(Volume));
	}
}


/* Unmounts an ISO 9660 volume releasing its state structure */
/* TODO: Add forced unmount */
int iso9660_unmount(Volume *v)
{
	if (v->files_open.size) return -EBUSY;
	free_volume(v);
	LOG_PRINTF(("[ISO 9660] Volume succesfully unmounted.\n"));
	return 0;
}


#define ABORT_MOUNT(e) { res = e; goto abort_mount; }
/* TODO: Add error checking */
int iso9660_mount(const char *device_name, Volume **volume)
{
	#if ISO9660_CONFIG_FD32
	BlockMediumInfo bmi;
	#endif
	int res;
	Volume *v = NULL;
	struct iso9660_pvd *pvd;

	v = (Volume *) malloc(sizeof(Volume));
	if (!v) return -ENOMEM;
	memset(v, 0, sizeof(Volume));

	/* Open the block device and get the sector size */
	#if ISO9660_CONFIG_FD32
	res = block_get(device_name, BLOCK_OPERATIONS_TYPE, &v->blockdev.bops, &v->blockdev.handle);
	if (res < 0) ABORT_MOUNT(res);
	res = v->blockdev.bops->open(v->blockdev.handle);
	if (res < 0) ABORT_MOUNT(res);
	res = v->blockdev.bops->get_medium_info(v->blockdev.handle, &bmi);
	if (res < 0) ABORT_MOUNT(res);
	if (bmi.block_bytes != 2048) ABORT_MOUNT(-EINVAL);
	#else
	v->blockdev = fopen(device_name, "rb");
	if (!v->blockdev) ABORT_MOUNT(-ENOENT);
	#endif
	v->bytes_per_sector     = 2048;
	v->log_bytes_per_sector = 11;

	/* Buffers */
	v->buffer.data = (uint8_t *) malloc(v->bytes_per_sector);
	if (!v->buffer.data) ABORT_MOUNT(-ENOMEM);

	/* Read the Primary Volume Descriptor */
	res = iso9660_blockdev_read(v, v->buffer.data, 1, 16);
	if (res < 0) ABORT_MOUNT(res);
	pvd = (struct iso9660_pvd *) v->buffer.data;

	/* Logical block size and blocks per sector */
	switch (ME(pvd->logical_block_size))
	{
		case 512:  v->log_block_size = 9;  break;
		case 1024: v->log_block_size = 10; break;
		case 2048: v->log_block_size = 11; break;
		default: ABORT_MOUNT(-EINVAL);
	}
	v->block_size = 1 << v->log_block_size;
	v->log_blocks_per_sector = v->log_bytes_per_sector - v->log_block_size;
	v->blocks_per_sector = 1 << v->log_blocks_per_sector;
	v->vol_space = ME(pvd->vol_space);

	/* Files and root directory */
	v->root_extent      = ME(pvd->root_directory.extent);
	v->root_len_ear     = pvd->root_directory.len_ear;
	v->root_data_length = ME(pvd->root_directory.data_length);
	memcpy(&v->root_recording_time, &pvd->root_directory.recording_time, sizeof(struct iso9660_dir_record_timestamp));

	/* Initialize files */
	slabmem_create(&v->dentries_slab, sizeof(Dentry));
	v->root_dentry.v = v;
	v->root_dentry.flags = pvd->root_directory.flags;
	v->root_dentry.references = 1; /* mounted root */
	v->num_dentries = 1; /* mounted root */
	slabmem_create(&v->files_slab, sizeof(File));

	memcpy(v->vol_id, pvd->vol_id, sizeof(v->vol_id));
	v->magic = ISO9660_VOL_MAGIC;
	*volume = v;
	return 0;
abort_mount:
	free_volume(v);
	LOG_PRINTF(("[ISO 9660] Mount aborted with result %i\n", res));
	return res;
}


/* Reads a logical block from the hosting block device fetching
 * the sector containing the logical block in a buffer.
 * If the requested sector is already buffered, uses that buffer.
 * On success, puts the address of the buffer in "buffer" and returns
 * the byte offset of the requested block in the data of that buffer.
 */
int iso9660_readbuf(Volume *v, Block block, Buffer **buffer)
{
	assert(v);
	assert(buffer);
	v->buf_hit++;
	if (!v->buffer.valid || (block < v->buffer.block) || (block >= v->buffer.block + v->blocks_per_sector))
	{
		ssize_t res;
		v->buf_hit--;
		v->buf_miss++;
		v->buffer.valid = 0;
		v->buffer.sector = block >> v->log_blocks_per_sector;
		v->buffer.block  = v->buffer.sector << v->log_blocks_per_sector;
		res = iso9660_blockdev_read(v, v->buffer.data, 1, v->buffer.sector);
		if (res != 1) return -EIO;
		v->buffer.valid = 1;
	}
	v->buf_access++;
	*buffer = &v->buffer;
	return (block - v->buffer.block) << v->log_bytes_per_sector;
}
