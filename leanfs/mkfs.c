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
/* This file is "mkfs.c".
 * An utility program to create a LEAN file system on a block device.
 */
#include "leanfs.h"
//#define NDEBUG
#include <assert.h>


static size_t write_sectors(const void *data, size_t count, blkcnt_t sector, size_t sector_size, FILE *f)
{
	fseek(f, sector * sector_size, SEEK_SET);
	return fwrite(data, sector_size, count, f);
}


static int write_cluster(const void *data, const struct SuperBlock *sb, blkcnt_t cluster, FILE *f)
{
	return write_sectors(data, 1 << sb->log_sectors_per_cluster,
	                     cluster << sb->log_sectors_per_cluster,
	                     1 << sb->log_bytes_per_sector, f) != 1;
}


static inline uint32_t leanfs_checksum(const uint32_t *superblock)
{
	unsigned k;
	uint32_t res = 0;
	for (k = 0; k < 25; k++)
		res += superblock[k];
	return res;
}


static int mkleanfs(const char *filename, uint32_t clusters, unsigned log_bytes_per_sector, unsigned log_sectors_per_cluster)
{
	unsigned sector_bytes  = 1 << log_bytes_per_sector;
	unsigned cluster_bytes = sector_bytes << log_sectors_per_cluster;
	unsigned bitmap_clusters_count = (clusters + cluster_bytes * 8 - 1) / (cluster_bytes * 8);
	uint8_t  buffer[cluster_bytes];

	printf("[mkleanfs] Opening \"%s\"...\n", filename);
	FILE *f = fopen(filename, "r+b");
	if (!f)
	{
		printf("[mkleanfs] Unable to open \"%s\"\n", filename);
		return -1;
	}

	/* Prepare and store the superblock */
	printf("[mkleanfs] Writing the superblock on sector 1...\n");
	struct SuperBlock sb;
	memset(&sb, 0, sizeof(struct SuperBlock));
	sb.magic                   = LEANFS_SB_MAGIC;
	sb.fs_version              = 0;
	sb.log_bytes_per_sector    = (uint8_t) log_bytes_per_sector;
	sb.log_sectors_per_cluster = (uint8_t) log_sectors_per_cluster;
	sb.clusters_count          = clusters;
	sb.bitmap_start_cluster    = 1;
	if (log_sectors_per_cluster == 0) sb.bitmap_start_cluster++;
	sb.backup_super            = sb.bitmap_start_cluster + bitmap_clusters_count;
	sb.root_start_cluster      = sb.backup_super + 1;
	sb.free_clusters_count     = clusters - sb.root_start_cluster - 1;
	sb.serial_number           = 0;
	sb.checksum                = leanfs_checksum((const uint32_t *) &sb);
	sb.state                   = LEANFS_SB_CLEAN;
	strcpy(sb.volume_label, "New LEAN volume");
	assert(sizeof(struct SuperBlock) <= sizeof(buffer));
	memset(buffer, 0, sizeof(buffer));
	memcpy(buffer, &sb, sizeof(struct SuperBlock));
	if (write_sectors(buffer, 1, 1, sector_bytes, f) != 1)
	{
		printf("[mkleanfs] Error writing the superblock.\n");
		fclose(f);
		return -1;
	}

	/* Store the superblock backup */
	printf("[mkleanfs] Writing the superblock backup on cluster %u...\n", sb.backup_super);
	if (!write_cluster(buffer, &sb, sb.backup_super, f))
	{
		printf("[mkleanfs] Error writing the superblock backup.\n");
		fclose(f);
		return -1;
	}

	/* Store the bitmap */
	printf("[mkleanfs] Writing the bitmap (%u clusters starting from %u)...\n",
	       bitmap_clusters_count, sb.bitmap_start_cluster);
	unsigned cluster = 0, bitmap_cluster;
	for (bitmap_cluster = 0; bitmap_cluster < bitmap_clusters_count; bitmap_cluster++)
	{
		unsigned k = 0, bit = 0;
		memset(buffer, 0, sizeof(buffer));
		printf("[mkleanfs]   bitmap cluster %u/%u (at %u)\n", bitmap_cluster + 1,
		       bitmap_clusters_count, bitmap_cluster + sb.bitmap_start_cluster);
		while ((cluster <= sb.root_start_cluster) && (k < cluster_bytes))
		{
			buffer[k] |= 1 << bit;
			if (++bit == 8)
			{
				bit = 0;
				k++;
			}
			cluster++;
		}
		if (!write_cluster(buffer, &sb, bitmap_cluster + sb.bitmap_start_cluster, f))
		{
			printf("[mkleanfs] Error writing the bitmap cluster.\n");
			fclose(f);
			return -1;
		}
	}

	/* Prepare the . and .. entries of the root directory */
	printf("[mkleanfs] Writing the root directory on cluster %u...\n", sb.root_start_cluster);
	struct
	{
		struct Inode inode;
		struct DirEntryHeader dot_deh;
		char dot_name[4];
		struct DirEntryHeader dotdot_deh;
		char dotdot_name[4];
	} root;
	root.dot_deh.inode = sb.root_start_cluster;
	root.dot_deh.rec_len = sizeof(struct DirEntryHeader) + sizeof(root.dot_name);
	root.dot_deh.name_len = 1;
	root.dot_deh.type = LEANFS_FT_DIR;
	strncpy(root.dot_name, ".", sizeof(root.dot_name));
	root.dotdot_deh.inode = sb.root_start_cluster;
	root.dotdot_deh.rec_len = sizeof(struct DirEntryHeader) + sizeof(root.dotdot_name);
	root.dotdot_deh.name_len = 2;
	root.dotdot_deh.type = LEANFS_FT_DIR;
	strncpy(root.dotdot_name, "..", sizeof(root.dotdot_name));

	/* Prepare the root directory inode */
	memset(&root.inode, 0, sizeof(struct Inode));
	root.inode.file_size      = sizeof(root) - sizeof(struct Inode);
	root.inode.clusters_count = (root.inode.file_size + cluster_bytes - 1) / cluster_bytes;
	root.inode.attributes     = LEANFS_IRUSR | LEANFS_IWUSR | LEANFS_IXUSR | LEANFS_IFDIR;
	root.inode.links_count    = 2;

	/* Store the root directory */
	assert(sizeof(root) <= sizeof(buffer));
	memset(buffer, 0, sizeof(buffer));
	memcpy(buffer, &root, sizeof(root));
	if (!write_cluster(buffer, &sb, sb.root_start_cluster, f))
	{
		printf("[mkleanfs] Error writing the root directory.\n");
		fclose(f);
		return -1;
	}
	printf("[mkleanfs] LEAN File System successfully created, %u clusters free.\n", sb.free_clusters_count);
	fclose(f);
	return 0;
}


int main()
{
	return mkleanfs("leantest.img", 1440, 9, 1);
//	return mkleanfs("/dev/fd0", 1440, 9, 1);
}
