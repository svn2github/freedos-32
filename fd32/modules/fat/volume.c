/* The FreeDOS-32 FAT Driver version 2.0
 * Copyright (C) 2001-2006  Salvatore ISAJA
 *
 * This file "volume.c" is part of the FreeDOS-32 FAT Driver (the Program).
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
 * \brief Low level part of the driver dealing with the hosting block device.
 */
/**
 * \addtogroup fat
 * @{
 */
#include "fat.h"


/* Buffer flags */
enum
{
	BUF_VALID = 1 << 0, /* Buffer contains valid data */
	BUF_DIRTY = 1 << 1  /* Buffer contains data to be written */
};


/* Searches for a volume buffer contaning the specified sector.
 * Returns the buffer address on success, or NULL on failure.
 */
static Buffer *find_buffer(Volume *v, Sector sector)
{
	unsigned k = v->num_buffers;
	Buffer  *b = v->buffers;
	for (; k--; b++)
	{
		if ((b->flags & BUF_VALID) && (sector >= b->sector) && (sector < b->sector + b->count))
			return b;
	}
	return NULL;
}


#if FAT_CONFIG_WRITE
/* Writes the content of the specified buffer in the hosting block
 * device, marking the buffer as not dirty.
 */
static int flush_buffer(Buffer *b)
{
	if ((b->flags & (BUF_VALID | BUF_DIRTY)) == (BUF_VALID | BUF_DIRTY))
	{
		Volume *v = b->v;
		ssize_t res = fat_blockdev_write(v, b->data, b->count, b->sector);
		if ((res < 0) || (res != b->count)) return -EIO;
		b->flags &= ~BUF_DIRTY;
	}
	return 0;
}


/* Flushes all buffers of the specified volume, calling flush_buffer. */
int fat_sync(Volume *v)
{
	unsigned k;
	int res;
	Buffer *b = NULL;
	if ((v->fat_type == FAT32) && v->fsinfo_changed)
	{
		struct fat_fsinfo *fsi;
		res = fat_readbuf(v, 1, &b, false);
		if (res < 0) return res;
		fsi = (struct fat_fsinfo *) b->data + res;
		fsi->free_clusters = v->free_clusters;
		fsi->next_free = v->next_free;
		res = fat_dirtybuf(b, false);
		v->fsinfo_changed = false;
	}
	for (k = 0; k < v->num_buffers; k++)
	{
		res = flush_buffer(&v->buffers[k]);
		if (res < 0) return res;
	}
	return 0;
}


/* Marks a buffer as dirty (i.e. it needs to be committed). */
int fat_dirtybuf(Buffer *b, bool write_through)
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
#endif /* #if FAT_CONFIG_WRITE */


/* Performs a buffered read from the hosting block device.
 * If the requested sector is already buffered, use that buffer.
 * If the sector is not buffered, read it from the hosting block device:
 * if "buffer" is NULL, flush the least recently used buffer and use it,
 * if "buffer" is not NULL, flush that buffer and use it (this can be useful
 * for sequential access).
 * On success, puts the address of the buffer in "buffer" and returns
 * the byte offset of the requested sector in the data of that buffer.
 * TODO: enable read_through
 */
int fat_readbuf(Volume *v, Sector sector, Buffer **buffer, bool read_through)
{
	int res;
	Buffer *b;

	v->buf_hit++;
	b = find_buffer(v, sector);
	if (!b)
	{
		v->buf_hit--;
		v->buf_miss++;
		b = *buffer;
		if (!b) b = (Buffer *) v->buffers_lru.begin;
		#if FAT_CONFIG_WRITE
		res = flush_buffer(b);
		if (res < 0) return res;
		#endif
		b->flags = 0;
		b->sector = sector & ~(v->sectors_per_buffer - 1);
		res = fat_blockdev_read(v, b->data, v->sectors_per_buffer, b->sector);
		if ((res < 0) || (sector >= b->sector + res)) return -EIO;
		b->count = res;
		b->flags = BUF_VALID;
	}
	v->buf_access++;
	list_erase(&v->buffers_lru, (ListItem *) b);
	list_push_back(&v->buffers_lru, (ListItem *) b);
	*buffer = b;
	return (sector - b->sector) << v->log_bytes_per_sector;
}


#if 0 //FAT_CONFIG_FD32
/* This function replaces the hosting block device's request funciton */
/* when a volume is mounted on it.                                    */
static fd32_request_t mounted_request;
static int mounted_request(DWORD function, void *params)
{
	fd32_ismounted_t *im = (fd32_ismounted_t *) params;
	/* If a request different from ISMOUNTED is presented to a device,
	 * the call will fail because a file system is mounted on it. */
	if (function != FD32_ISMOUNTED) return -EBUSY;
	if (im->Size < sizeof(fd32_ismounted_t)) return -EINVAL;
	im->fsreq   = fat_request;
	im->FSDevId = im->DeviceId;
	return 0;
}
#endif


/* Scans the FAT and returns the number of free clusters */
static int scan_free_clusters(Volume *v)
{
	int res;
	Cluster k, free_clusters = 0;
	for (k = 2; k < v->data_clusters + 2; k++)
	{
		res = v->fat_read(v, k, v->active_fat);
		if (res < 0) return res;
		if (!res) free_clusters++;
	}
	return free_clusters;
}


/* Collects volume garbage on unmount or mount error */
static void free_volume(Volume *v)
{
	if (v)
	{
		slabmem_destroy(&v->files_slab);
		slabmem_destroy(&v->channels_slab);
		if (v->buffers)
		{
			if (v->buffer_data)
				mfree(v->buffer_data, v->bytes_per_sector * v->sectors_per_buffer * v->num_buffers);
			mfree(v->buffers, sizeof(Buffer) * v->num_buffers);
		}
		if (v->nls) v->nls->release();
		#if FAT_CONFIG_FD32
		if (v->blk.is_open)
		{
			v->blk.bops->close(v->blk.handle);
			v->blk.is_open = false;
		}
		if (v->blk.bops) v->blk.bops->request(REQ_RELEASE);
		#else
		if (v->blk) fclose(v->blk);
		#endif
		v->magic = 0; /* Invalidate signature against bad pointers */
		mfree(v, sizeof(Volume));
	}
}


/* Unmounts a FAT volume releasing its state structure */
/* TODO: Add forced unmount */
int fat_unmount(Volume *v)
{
	#if 0 //FAT_CONFIG_FD32
	int res;
	#endif
	if (v->channels_open.size) return -EBUSY;
	#if 0 //FAT_CONFIG_FD32
	/* Restore the original device data for the hosting block device */
	res = fd32_dev_replace(v->blk.handle, v->blk.request, v->blk.devid);
	if (res < 0) return res;
	#endif
	free_volume(v);
	LOG_PRINTF(("[FAT2] Volume succesfully unmounted.\n"));
	return 0;
}


/* A simple macro that prints a log error message and aborts the mount function */
#define ABORT_MOUNT(e) { LOG_PRINTF(e); goto abort_mount; }


/* Mounts a FAT volume initializing its state structure */
int fat_mount(const char *blk_name, Volume **volume)
{
	#if FAT_CONFIG_FD32
	BlockMediumInfo bmi;
	#endif
	struct fat16_bpb *bpb;
	Volume  *v = NULL;
	uint8_t *p, *secbuf = NULL;
	unsigned block_size = 512;
	Sector   vol_sectors, total_blocks = UINT32_MAX;
	int      res, k;

	/* Allocate the FAT volume structure */
	v = (Volume *) malloc(sizeof(Volume));
	if (!v) return -ENOMEM;
	memset(v, 0, sizeof(Volume));
	v->magic = FAT_VOL_MAGIC;

	/* Initialize files */
	slabmem_create(&v->dentries_slab, sizeof(Dentry));
	v->root_dentry.v = v;
	v->root_dentry.attr = FAT_ADIR;
	v->root_dentry.references = 1; /* mounted root */
	v->num_dentries = 1; /* mounted root */
	slabmem_create(&v->files_slab, sizeof(File));
	slabmem_create(&v->channels_slab, sizeof(Channel));

	/* Open the block device, allocate a sector buffer and read the boot sector */
	#if FAT_CONFIG_FD32
	LOG_PRINTF(("[FAT2] Trying to mount a volume on device '%s'\n", blk_name));
	res = block_get(blk_name, BLOCK_OPERATIONS_TYPE, (void **)&v->blk.bops, &v->blk.handle);
	if (res < 0) ABORT_MOUNT(("[FAT2] Cannot get operations\n"));
	res = v->blk.bops->open(v->blk.handle);
	if (res < 0) ABORT_MOUNT(("[FAT2] Cannot open the device\n"));
	v->blk.is_open = true;
	res = v->blk.bops->get_medium_info(v->blk.handle, &bmi);
	if (res < 0) ABORT_MOUNT(("[FAT2] Cannot get medium information\n"));
	block_size   = bmi.block_bytes;
	total_blocks = bmi.blocks_count;
	res = -ENOMEM;
	secbuf = (uint8_t *) malloc(block_size);
	if (!secbuf) ABORT_MOUNT(("[FAT2] Cannot allocate a sector buffer\n"));
	res = v->blk.bops->read(v->blk.handle, secbuf, 0, 1, 0);
	if (res < 0)
	{
		res = -EIO;
		ABORT_MOUNT(("[FAT2] Cannot read the boot sector\n"));
	}
	#else /* !FAT_CONFIG_FD32 */
	res = -EIO;
	#if FAT_CONFIG_WRITE
	v->blk = fopen(blk_name, "r+b");
	#else
	v->blk = fopen(blk_name, "rb");
	#endif
	if (!v->blk) ABORT_MOUNT(("[FAT2] Cannot open the disk image\n"));
	res = -ENOMEM;
	secbuf = (uint8_t *) malloc(block_size);
	if (!secbuf) ABORT_MOUNT(("[FAT2] Cannot allocate a sector buffer\n"));
	res = fat_blockdev_read(&v->blk, secbuf, 1, 0);
	if (res < 0)
	{
		res = -EIO;
		ABORT_MOUNT(("[FAT2] Cannot read the boot sector\n"));
	}
	#endif
	#if 0
	/* Read the first block of the device */
	res = -ENOMEM;
	secbuf = (uint8_t *) malloc(block_size);
	if (!secbuf) ABORT_MOUNT(("[FAT2] Cannot allocate a sector buffer\n"));
	res = fat_blockdev_read(&v->blk, secbuf, 1, 0);
	if (res < 0)
	{
		res = -EIO;
		ABORT_MOUNT(("[FAT2] Cannot read the boot sector\n"));
	}
	#endif

	/* Check if the BPB is valid */
	res = -ENODEV; /* FIXME: Use custom error code for "unknown/invalid file system" */
	bpb = (struct fat16_bpb *) secbuf;
	if (*((uint16_t *) &secbuf[510]) != 0xAA55)
		ABORT_MOUNT(("[FAT2] Boot sector signature 0xAA55 not found\n"));
	vol_sectors = bpb->num_sectors_16;
	if (!vol_sectors) vol_sectors = bpb->num_sectors_32;
	if (!vol_sectors) ABORT_MOUNT(("[FAT2] Both num_sectors_16 and num_sectors_32 in BPB are zero\n"));
	if (vol_sectors > total_blocks)
		ABORT_MOUNT(("[FAT2] num_sectors in BPB larger than block device size: %u > %u\n", vol_sectors, total_blocks));
	switch (bpb->bytes_per_sector)
	{
		case 512 : v->log_bytes_per_sector = 9;  break;
		case 1024: v->log_bytes_per_sector = 10; break;
		case 2048: v->log_bytes_per_sector = 11; break;
		case 4096: v->log_bytes_per_sector = 12; break;
		default: ABORT_MOUNT(("[FAT2] Invalid bytes_per_sector in BPB: %u\n", bpb->bytes_per_sector));
	}
	v->bytes_per_sector = 1 << v->log_bytes_per_sector;
	switch (bpb->sectors_per_cluster)
	{
		case 1  : v->log_sectors_per_cluster = 0; break;
		case 2  : v->log_sectors_per_cluster = 1; break;
		case 4  : v->log_sectors_per_cluster = 2; break;
		case 8  : v->log_sectors_per_cluster = 3; break;
		case 16 : v->log_sectors_per_cluster = 4; break;
		case 32 : v->log_sectors_per_cluster = 5; break;
		case 64 : v->log_sectors_per_cluster = 6; break;
		case 128: v->log_sectors_per_cluster = 7; break;
		default: ABORT_MOUNT(("[FAT2] Invalid sectors_per_cluster in BPB: %u\n", bpb->sectors_per_cluster));
	}
	v->sectors_per_cluster = 1 << v->log_sectors_per_cluster;
	/* The media_id can be 0xF0, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF */
	if ((bpb->media_id != 0xF0) && (bpb->media_id < 0xF8))
		ABORT_MOUNT(("[FAT2] Invalid media_id in BPB: %u\n", bpb->media_id));

	v->fat_size = bpb->fat_size_16;
	if (!v->fat_size) v->fat_size = ((struct fat32_bpb *) secbuf)->fat_size_32;
	v->fat_start   = bpb->reserved_sectors;
	v->num_fats    = bpb->num_fats;
	v->root_sector = v->fat_start + v->num_fats * v->fat_size;
	v->root_size   = (bpb->root_entries * sizeof(struct fat_direntry)
	               + (v->bytes_per_sector - 1)) >> v->log_bytes_per_sector;
	v->data_start  = v->root_sector + v->root_size;
	v->data_clusters = (vol_sectors - v->data_start) >> v->log_sectors_per_cluster;

	/* Determine the FAT type */
	if (v->data_clusters < 4085)
	{
		v->fat_type  = FAT12;
		v->fat_read  = fat12_read;
		#if FAT_CONFIG_WRITE
		v->fat_write = fat12_write;
		#endif
	}
	else if (v->data_clusters < 65525)
	{
		v->fat_type  = FAT16;
		v->fat_read  = fat16_read;
		#if FAT_CONFIG_WRITE
		v->fat_write = fat16_write;
		#endif
	}
	else
	{
		v->fat_type  = FAT32;
		v->fat_read  = fat32_read;
		#if FAT_CONFIG_WRITE
		v->fat_write = fat32_write;
		#endif
	}

	/* Initialize buffers */
	res = -ENOMEM;
	v->num_buffers = FAT_NUM_BUFFERS;
	v->sectors_per_buffer = FAT_READ_AHEAD;
	v->buffers = (Buffer *) malloc(sizeof(Buffer) * v->num_buffers);
	if (!v->buffers) ABORT_MOUNT(("[FAT2] Could not allocate buffers\n"));
	v->buffer_data = (uint8_t *) malloc(v->bytes_per_sector * v->sectors_per_buffer * v->num_buffers);
	if (!v->buffer_data) ABORT_MOUNT(("[FAT2] Could not allocate buffer data\n"));
	memset(v->buffers, 0, sizeof(Buffer) * v->num_buffers);
	for (k = 0, p = v->buffer_data; k < v->num_buffers; k++, p += v->bytes_per_sector * v->sectors_per_buffer)
	{
		v->buffers[k].v = v;
		v->buffers[k].data = p;
		list_push_back(&v->buffers_lru, (ListItem *) &v->buffers[k]);
	}

	/* More pracalcs if FAT32 */
	v->serial_number = bpb->serial_number;
	memcpy(v->volume_label, bpb->volume_label, sizeof(v->volume_label));
	v->free_clusters = FAT_FSI_NA;
	v->next_free = FAT_FSI_NA;
	v->fsinfo_changed = false;
	if (v->fat_type == FAT32)
	{
		struct fat32_bpb *bpb32 = (struct fat32_bpb *) secbuf;
		struct fat_fsinfo *fsi;
		Buffer *b = NULL;
		res = -ENODEV; /* FIXME: Use custom error code for "unknown/invalid file system" */
		if (bpb32->fs_version) ABORT_MOUNT(("[FAT2] Unknown FAT32 version in BPB: %04xh\n", bpb32->fs_version));
		res = fat_readbuf(v, bpb32->fsinfo_sector, &b, false);
		if (res < 0) ABORT_MOUNT(("[FAT2] Error reading the FAT32 FSInfo sector\n"));
		fsi = (struct fat_fsinfo *) &b->data[res];
		res = -ENODEV; /* FIXME: Use custom error code for "unknown/invalid file system" */
		if ((fsi->sig1 != FAT_FSI_SIG1) || (fsi->sig2 != FAT_FSI_SIG2) || (fsi->sig3 != FAT_FSI_SIG3))
			ABORT_MOUNT(("[FAT2] Wrong signatures in the FAT32 FSInfo sector\n"));
		v->free_clusters = fsi->free_clusters;
		v->next_free = fsi->next_free;
		v->active_fat = bpb32->ext_flags & 0x0F; /* TODO: FAT mirroring enabled if !(ext_flags & 0x80) */
		v->root_cluster = bpb32->root_cluster;
		v->serial_number = bpb32->serial_number;
		memcpy(v->volume_label, bpb32->volume_label, sizeof(v->volume_label));
	}

	/* Scan free clusters if the value is unknown or invalid */
	if ((v->free_clusters == FAT_FSI_NA) || (v->free_clusters > v->data_clusters + 1))
	{
		LOG_PRINTF(("[FAT2] Free cluster count not available. Scanning the FAT...\n"));
		res = scan_free_clusters(v);
		if (res < 0) ABORT_MOUNT(("[FAT2] Error %i while scanning free clusters\n", res));
		v->free_clusters = (Cluster) res;
	}
	LOG_PRINTF(("[FAT2] %u/%u clusters available\n", v->free_clusters, v->data_clusters));
	res = nls_get("default", OT_NLS_OPERATIONS, (void **) &v->nls);
	if (res < 0) ABORT_MOUNT(("[FAT2] Could not get NLS operations"));

	#if 0 //FAT_CONFIG_FD32
	/* Request function and DeviceId of the hosting block device are backed up
	 * in v->blk_request and v->blk_devid, so we replace the device. */
	res = fd32_dev_replace(blk_handle, mounted_request, v);
	if (res < 0) ABORT_MOUNT(("[FAT2] Could not replace the block device\n"));
	#endif

	#if FAT_CONFIG_DEBUG
	LOG_PRINTF(("[FAT2] "));
	switch (v->fat_type)
	{
		case FAT12 : LOG_PRINTF(("FAT12")); break;
		case FAT16 : LOG_PRINTF(("FAT16")); break;
		case FAT32 : LOG_PRINTF(("FAT32")); break;
	}
	LOG_PRINTF((" volume successfully mounted on device '%s'\n", blk_name));
	#endif
	mfree(secbuf, block_size);
	*volume = v;
	return 0;

abort_mount:
	if (secbuf) mfree(secbuf, block_size);
	free_volume(v); 
	return res;
}


#if FAT_CONFIG_REMOVABLE && FAT_CONFIG_FD32
int fat_handle_attention(Volume *v)
{
	BlockMediumInfo bmi;
	uint8_t  volume_label[11];
	int      res;
	uint8_t *secbuf;
	uint32_t serial_number;

	res = v->blk.bops->revalidate(v->blk.handle);
	if (res < 0) return res;
	res = v->blk.bops->get_medium_info(v->blk.handle, &bmi);
	if (res < 0) return res;
	secbuf = (uint8_t *) malloc(bmi.block_bytes);
	if (!secbuf) return -ENOMEM;
	res = v->blk.bops->read(v->blk.handle, secbuf, 0, 1, BLOCK_RW_NOCACHE); //POSSIBLE LEAK!!!
	if (res < 0) return -EIO;
	if (v->fat_type == FAT32)
	{
		serial_number = ((struct fat32_bpb *) secbuf)->serial_number;
		memcpy(volume_label, ((struct fat32_bpb *) secbuf)->volume_label, 11);
	}
	else
	{
		serial_number = ((struct fat16_bpb *) secbuf)->serial_number;
		memcpy(volume_label, ((struct fat16_bpb *) secbuf)->volume_label, 11);
	}
	mfree(secbuf, bmi.block_bytes);
	if ((v->serial_number != serial_number) || memcmp(v->volume_label, volume_label, 11))
	{
		if (v->channels_open.size) return -EBUSY;
		#if 1
		fat_unmount(v);
		return -ENOTMOUNT;
		#else
		return 1;
		#endif
	}
	/* If there are not open files, discard the content of all buffers */
	res = 0;
	if (!v->channels_open.size)
	{
		unsigned k = v->num_buffers;
		Buffer  *b = v->buffers;
		for (; k--; b++)
		{
			if (b->flags & BUF_DIRTY)
			{
				res = -EBUSY;
				break;
			}
			b->flags = 0;
		}
	}
	return res;
}
#endif /* #if FAT_CONFIG_REMOVABLE */


/* Partition types supported by the FAT driver */
static const struct { unsigned id; const char *name; } partition_types[] =
{
	{ 0x01, "FAT12"                       },
	{ 0x04, "FAT16 up to 32 MB"           },
	{ 0x06, "FAT16 over 32 MB"            },
	{ 0x0B, "FAT32"                       },
	{ 0x0C, "FAT32 using LBA BIOS"        },
	{ 0x0E, "FAT16 using LBA BIOS"        },
	{ 0x1B, "Hidden FAT32"                },
	{ 0x1C, "Hidden FAT32 using LBA BIOS" },
	{ 0x1E, "Hidden VFAT"                 },
	{ 0, 0 }
};

/* Checks if the passed partition signature is supported by the FAT driver.
 * Returns zero if supported, nonzero if not.
 */
int fat_partcheck(unsigned id)
{
	unsigned k;
	for (k = 0; partition_types[k].id; k++)
		if (partition_types[k].id == id)
		{
			LOG_PRINTF(("[FAT2] Partition type is %02xh:%s\n", k, partition_types[k].name));
			return 0;
		}
	return -1;
}


#if FAT_CONFIG_FD32
/* Gets file system informations */
int fat_get_fsinfo(fd32_fs_info_t *fsinfo)
{
	if (fsinfo->Size < sizeof(fd32_fs_info_t)) return -EINVAL;
	#if FAT_CONFIG_LFN
	fsinfo->Flags   = FD32_FSICASEPRES | FD32_FSIUNICODE | FD32_FSILFN;
	fsinfo->NameMax = FD32_LFNMAX;
	fsinfo->PathMax = FD32_LFNPMAX;
	#else
	fsinfo->Flags   = 0;
	fsinfo->NameMax = FD32_SFNMAX;
	fsinfo->PathMax = FD32_SFNPMAX;
	#endif
	strncpy(fsinfo->FSName, "FAT", fsinfo->FSNameSize - 1); //TODO: Replace with snprintf or strlcpy */
	return 0;
}


/* Gets allocation informations on a FAT volume. */
int fat_get_fsfree(fd32_getfsfree_t *fsfree)
{
	Volume *v;
	int res;
	if (fsfree->Size < sizeof(fd32_getfsfree_t)) return -EINVAL;
	v = (Volume *) fsfree->DeviceId;
	if (v->magic != FAT_VOL_MAGIC) return -ENODEV;
	#if FAT_CONFIG_REMOVABLE
	res = fat_blockdev_test_unit_ready(v);
	if (res < 0) return res;
	#endif
	fsfree->SecPerClus  = v->sectors_per_cluster;
	fsfree->BytesPerSec = v->bytes_per_sector;
	fsfree->AvailClus   = v->free_clusters;
	fsfree->TotalClus   = v->data_clusters;
	return 0;
}
#endif

/* @} */
