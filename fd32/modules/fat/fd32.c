/* The FreeDOS-32 FAT Driver version 2.0
 * Copyright (C) 2001-2005  Salvatore ISAJA
 *
 * This file "fd32.c" is part of the FreeDOS-32 FAT Driver (the Program).
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
 * \brief FreeDOS-32 specific part of the driver: initialization and requests.
 */
/**
 * \addtogroup fat
 * @{
 */
#include "fat.h"
#include <kernel.h>
#if FAT_CONFIG_DEBUG
 #define LOG_PRINTF(s) fd32_log_printf s
#else
 #define LOG_PRINTF(s)
#endif


ssize_t fat_blockdev_read(Volume *v, void *data, size_t count, Sector from)
{
	ssize_t res;
	unsigned k;
	for (k = 0;; k++)
	{
		if (k == 3) return -EIO;
		res = v->blk.bops->read(v->blk.handle, data, from, count, 0);
		LOG_PRINTF(("[FAT2] fat_blockdev_read returned %08xh\n", res));
		if (res >= 0) break;
		if (!BLOCK_IS_ERROR(res)) break;
		if (BLOCK_GET_SENSE(res) != BLOCK_SENSE_ATTENTION) break;
		res = fat_handle_attention(v);
		if (res < 0) return res;
	}
	return res;
}


ssize_t fat_blockdev_write(Volume *v, const void *data, size_t count, Sector from)
{
	ssize_t res;
	unsigned k;
	for (k = 0;; k++)
	{
		if (k == 3) return -EIO;
		res = v->blk.bops->write(v->blk.handle, data, from, count, 0);
		if (res >= 0) break;
		if (!BLOCK_IS_ERROR(res)) break;
		if (BLOCK_GET_SENSE(res) != BLOCK_SENSE_ATTENTION) break;
		res = fat_handle_attention(v);
		if (res < 0) return res;
	}
	return res;
}


int fat_blockdev_test_unit_ready(Volume *v)
{
	int res = v->blk.bops->test_unit_ready(v->blk.handle);
	LOG_PRINTF(("[FAT2] fat_blockdev_test_unit_ready returned %08xh\n", res));
	if (BLOCK_IS_ERROR(res) && (BLOCK_GET_SENSE(res) == BLOCK_SENSE_ATTENTION))
	{
		LOG_PRINTF(("[FAT2] fat_blockdev_test_unit_ready detected attention\n"));
		res = fat_handle_attention(v);
	}
	return res;
}


int fat_request(DWORD function, void *params)
{
	int res;
	switch (function)
	{
		case FD32_READ:
		{
			fd32_read_t *p = (fd32_read_t *) params;
			Channel *c = (Channel *) p->DeviceId;
			LOG_PRINTF(("[FAT2] FD32_READ: %lu bytes... \n", p->BufferBytes));
			#if FAT_CONFIG_REMOVABLE
			res = fat_blockdev_test_unit_ready(c->f->v);
			if (res < 0) return res;
			#endif
			res = fat_read(c, p->Buffer, p->BufferBytes);
			LOG_PRINTF(("[FAT2] FD32_READ: done %i\n", res));
			return res;
		}
		#if FAT_CONFIG_WRITE
		case FD32_WRITE:
		{
			fd32_write_t *p = (fd32_write_t *) params;
			Channel *c = (Channel *) p->DeviceId;
			LOG_PRINTF(("[FAT2] FD32_WRITE: %lu bytes... \n", p->BufferBytes));
			#if FAT_CONFIG_REMOVABLE
			res = fat_blockdev_test_unit_ready(c->f->v);
			if (res < 0) return res;
			#endif
			res = fat_write(c, p->Buffer, p->BufferBytes);
			LOG_PRINTF(("[FAT2] FD32_WRITE: done %i\n", res));
			return res;
		}
		#endif
		case FD32_LSEEK:
		{
			off_t res64;
			fd32_lseek_t *p = (fd32_lseek_t *) params;
			LOG_PRINTF(("[FAT2] FD32_LSEEK: %lli from %lu... \n", p->Offset, p->Origin));
			res64 = fat_lseek((Channel *) p->DeviceId, p->Offset, p->Origin);
			if (res64 < 0) return (int) res64;
			p->Offset = res64;
			LOG_PRINTF(("[FAT2] FD32_LSEEK: done %lli\n", res64));
			return 0;
		}
		case FD32_OPENFILE:
		{
			fd32_openfile_t *p = (fd32_openfile_t *) params;
			Volume *v = (Volume *) p->DeviceId;
			LOG_PRINTF(("[FAT2] FD32_OPENFILE: %s mode %lx... \n", p->FileName, p->Mode));
			#if FAT_CONFIG_REMOVABLE
			res = fat_blockdev_test_unit_ready(v);
			if (res < 0) return res;
			#endif
			res = fat_open_pr(&v->root_dentry, p->FileName, strlen(p->FileName), p->Mode, p->Attr, (Channel **) &p->FileId);
			LOG_PRINTF(("[FAT2] FD32_OPENFILE: done %i\n", res));
			return res;
		}
		case FD32_CLOSE:
		{
			fd32_close_t *p = (fd32_close_t *) params;
			Channel *c = (Channel *) p->DeviceId;
			LOG_PRINTF(("[FAT2] FD32_CLOSE... \n"));
			#if FAT_CONFIG_REMOVABLE
			res = fat_blockdev_test_unit_ready(c->f->v);
			if (res < 0) return res;
			#endif
			res = fat_close(c);
			LOG_PRINTF(("[FAT2] FD32_CLOSE: done %i\n", res));
			return res;
		}
		case FD32_FINDFIRST:
		{
			struct fd32_findfirst *p = (struct fd32_findfirst *) params;
			Volume *v = (Volume *) p->volume;
			LOG_PRINTF(("[FAT2] FD32_FINDFIRST: %s attrib %02xh... \n", p->path, p->attrib));
			if (v->magic != FAT_VOL_MAGIC) return -ENODEV;
			#if FAT_CONFIG_REMOVABLE
			res = fat_blockdev_test_unit_ready(v);
			if (res < 0) return res;
			#endif
			res = fat_findfirst_pr(&v->root_dentry, p->path, strlen(p->path), p->attrib, (fd32_fs_dosfind_t *) p->find_data);
			LOG_PRINTF(("[FAT2] FD32_FINDFIRST: done %i\n", res));
			return res;
		}
		case FD32_FINDNEXT:
		{
			struct fd32_findnext *p = (struct fd32_findnext *) params;
			Volume *v = (Volume *) p->volume;
			LOG_PRINTF(("[FAT2] FD32_FINDNEXT... \n"));
			if (v->magic != FAT_VOL_MAGIC) return -ENODEV;
			#if FAT_CONFIG_REMOVABLE
			res = fat_blockdev_test_unit_ready(v);
			if (res < 0) return res;
			#endif
			res = fat_findnext(v, (fd32_fs_dosfind_t *) p->find_data);
			LOG_PRINTF(("[FAT2] FD32_FINDNEXT: done %i\n", res));
			return res;
		}
		case FD32_FINDFILE:
		{
			struct fd32_findfile *p = (struct fd32_findfile *) params;
			Channel *c = (Channel *) p->dir;
			LOG_PRINTF(("[FAT2] FD32_FINDFILE: %s flags %08xh\n", p->name, p->flags));
			if (c->magic != FAT_CHANNEL_MAGIC) return -EBADF;
			#if FAT_CONFIG_REMOVABLE
			res = fat_blockdev_test_unit_ready(c->f->v);
			if (res < 0) return res;
			#endif
			res = fat_findfile(c, p->name, strlen(p->name), p->flags, (fd32_fs_lfnfind_t *) p->find_data);
			LOG_PRINTF(("[FAT2] FD32_FINDFILE: done %i\n", res));
			return res;
		}
		#if FAT_CONFIG_WRITE
		case FD32_FFLUSH:
		{
			fd32_fflush_t *p = (fd32_fflush_t *) params;
			Channel *c = (Channel *) p->DeviceId;
			LOG_PRINTF(("[FAT2] FD32_FFLUSH\n"));
			#if FAT_CONFIG_REMOVABLE
			res = fat_blockdev_test_unit_ready(c->f->v);
			if (res < 0) return res;
			#endif
			return fat_fsync(c, false);
		}
		#endif
		case FD32_OPEN:
		{
			fd32_open_t *p = (fd32_open_t *) params;
			LOG_PRINTF(("[FAT2] FD32_OPEN\n"));
			if (((Channel *) p->DeviceId)->magic != FAT_CHANNEL_MAGIC) return -EBADF;
			return ++((Channel *) p->DeviceId)->references;
		}
		case FD32_GETATTR:
		{
			fd32_getattr_t *p = (fd32_getattr_t *) params;
			LOG_PRINTF(("[FAT2] FD32_GETATTR\n"));
			return fat_get_attr((Channel *) p->FileId, (fd32_fs_attr_t *) p->Attr);
		}
		#if FAT_CONFIG_WRITE
		case FD32_SETATTR:
		{
			fd32_setattr_t *p = (fd32_setattr_t *) params;
			Channel *c = (Channel *) p->FileId;
			LOG_PRINTF(("[FAT2] FD32_SETATTR\n"));
			if (c->magic != FAT_CHANNEL_MAGIC) return -EBADF;
			#if FAT_CONFIG_REMOVABLE
			res = fat_blockdev_test_unit_ready(c->f->v);
			if (res < 0) return res;
			#endif
			return fat_set_attr(c, (const fd32_fs_attr_t *) p->Attr);
		}
		case FD32_UNLINK:
		{
			fd32_unlink_t *p = (fd32_unlink_t *) params;
			Volume *v = (Volume *) p->DeviceId;
			if (v->magic != FAT_VOL_MAGIC) return -ENODEV;
			#if FAT_CONFIG_REMOVABLE
			res = fat_blockdev_test_unit_ready(v);
			if (res < 0) return res;
			#endif
			return fat_unlink_pr(&v->root_dentry, p->FileName, strlen(p->FileName)); //p->Flags ignored
		}
		case FD32_RENAME:
		{
			fd32_rename_t *p = (fd32_rename_t *) params;
			Volume *v = (Volume *) p->DeviceId;
			if (v->magic != FAT_VOL_MAGIC) return -ENODEV;
			#if FAT_CONFIG_REMOVABLE
			res = fat_blockdev_test_unit_ready(v);
			if (res < 0) return res;
			#endif
			return fat_rename_pr(&v->root_dentry, p->OldName, strlen(p->OldName),
			                     &v->root_dentry, p->NewName, strlen(p->NewName));
		}
		case FD32_MKDIR:
		{
			fd32_mkdir_t *p = (fd32_mkdir_t *) params;
			Volume *v = (Volume *) p->DeviceId;
			if (v->magic != FAT_VOL_MAGIC) return -ENODEV;
			#if FAT_CONFIG_REMOVABLE
			res = fat_blockdev_test_unit_ready(v);
			if (res < 0) return res;
			#endif
			return fat_mkdir_pr(&v->root_dentry, p->DirName, strlen(p->DirName), 0); //fake mode
		}
		case FD32_RMDIR:
		{
			fd32_rmdir_t *p = (fd32_rmdir_t *) params;
			Volume *v = (Volume *) p->DeviceId;
			if (v->magic != FAT_VOL_MAGIC) return -ENODEV;
			#if FAT_CONFIG_REMOVABLE
			res = fat_blockdev_test_unit_ready(v);
			if (res < 0) return res;
			#endif
			return fat_rmdir_pr(&v->root_dentry, p->DirName, strlen(p->DirName));
		}
		#endif /* FAT_CONFIG_WRITE */
		case FD32_MOUNT:
		{
			LOG_PRINTF(("[FAT2] FD32_MOUNT\n"));
			fd32_mount_t *p = (fd32_mount_t *) params;
			return fat_mount(p->BlkDev, (Volume **) &p->FsDev);
		}
		case FD32_UNMOUNT:
		{
			LOG_PRINTF(("[FAT2] FD32_UNMOUNT\n"));
			fd32_unmount_t *p = (fd32_unmount_t *) params;
			Volume *v = (Volume *) p->DeviceId;
			if (v->magic != FAT_VOL_MAGIC) return -ENODEV;
			#if FAT_CONFIG_REMOVABLE
			res = fat_blockdev_test_unit_ready(v);
			if (res < 0) return res;
			#endif
			return fat_unmount(v);
		}
		case FD32_PARTCHECK:
			return fat_partcheck(((fd32_partcheck_t *) params)->PartId);
		case FD32_GETFSINFO:
			return fat_get_fsinfo((fd32_fs_info_t *) ((fd32_getfsinfo_t *) params)->FSInfo);
		case FD32_GETFSFREE:
			return fat_get_fsfree((fd32_getfsfree_t *) params);
	}
	return -ENOTSUP;
}


/* FAT Driver entry point. Called by the FD32 kernel. */
int fat_init()
{
	int res;
	fd32_message("[FAT2] Installing the FAT driver 2.0...\n");
	/* Register the FAT Driver to the File System Layer */
	res = fd32_add_fs(fat_request);
	if (res < 0) return res;
	/* Register the FAT Driver request function to the kernel symbol table */
	if (fd32_add_call("fat_request", fat_request, ADD) == -1)
	{
		fd32_message("[FAT2] Couldn't add 'fat_request' to the symbol table\n");
		return -ENOMEM;
	}
	fd32_message("[FAT2] FAT driver installed.\n");
	return 0;
}

/* @} */
