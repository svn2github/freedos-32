/* The FreeDOS-32 ISO 9660 Driver version 0.2
 * Copyright (C) 2005  Salvatore ISAJA
 *
 * This file "fd32.c" is part of the FreeDOS-32 ISO 9660 Driver (the Program).
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
#include <kernel.h>
#define TEST 0 /* Runs a test after installing the driver */


static int handle_attention(Volume *v)
{
	BlockMediumInfo bmi;
	int res = v->blockdev.bops->revalidate(&v->blockdev.handle);
	if (res < 0) return res;
	res = v->blockdev.bops->get_medium_info(v->blockdev.handle, &bmi);
	if (res < 0) return res;
	if (v->bytes_per_sector == bmi.block_bytes)
	{
		struct iso9660_pvd *pvd;
		v->buffer.valid = 0;
		res = v->blockdev.bops->read(v->blockdev.handle, v->buffer.data, 16, 1, BLOCK_RW_NOCACHE);
		if (res < 0) return -EIO;
		pvd = (struct iso9660_pvd *) v->buffer.data;
		if (!memcmp(v->vol_id, pvd->vol_id, sizeof(v->vol_id)))
			return 0; /* Medium not changed */
	}
	/* Medium changed */
	if (v->files_open.size) return -EBUSY;
	iso9660_unmount(v);
	return -ENOTMOUNT;
}


ssize_t iso9660_blockdev_read(Volume *v, void *data, size_t count, Sector from)
{
	ssize_t res;
	unsigned k;
	for (k = 0;; k++)
	{
		if (k == 3) return -EIO;
		res = v->blockdev.bops->read(v->blockdev.handle, data, from, count, 0);
		LOG_PRINTF(("[ISO 9660] Block device read result %li\n", res));
		if (res >= 0) break;
		if (!BLOCK_IS_ERROR(-res)) break;
		if (BLOCK_GET_SENSE(-res) != BLOCK_SENSE_ATTENTION) break;
		res = handle_attention(v);
		if (res < 0) return res;
	}
	return res;
}


int iso9660_blockdev_test_unit_ready(Volume *v)
{
	int res = v->blockdev.bops->test_unit_ready(v->blockdev.handle);
	if ((res < 0) && BLOCK_IS_ERROR(-res) && (BLOCK_GET_SENSE(-res) == BLOCK_SENSE_ATTENTION))
		res = handle_attention(v);
	return res;
}


int iso9660_request(DWORD function, void *params)
{
	int res;
	switch (function)
	{
		case FD32_READ:
		{
			fd32_read_t *p = (fd32_read_t *) params;
			File *f = (File *) p->DeviceId;
			LOG_PRINTF(("[ISO 9660] FD32_READ: %lu bytes... \n", p->BufferBytes));
			res = iso9660_blockdev_test_unit_ready(f->dentry->v);
			if (res < 0) return res;
			res = iso9660_read(f, p->Buffer, p->BufferBytes);
			LOG_PRINTF(("[ISO 9660] FD32_READ: done %i\n", res));
			return res;
		}
		case FD32_LSEEK:
		{
			off_t res64;
			fd32_lseek_t *p = (fd32_lseek_t *) params;
			LOG_PRINTF(("[ISO 9660] FD32_LSEEK: %lli from %lu... \n", p->Offset, p->Origin));
			res64 = iso9660_lseek((File *) p->DeviceId, p->Offset, p->Origin);
			if (res64 < 0) return (int) res64;
			p->Offset = res64;
			LOG_PRINTF(("[ISO 9660] FD32_LSEEK: done %lli\n", res64));
			return 0;
		}
		case FD32_OPENFILE:
		{
			fd32_openfile_t *p = (fd32_openfile_t *) params;
			Volume *v = (Volume *) p->DeviceId;
			LOG_PRINTF(("[ISO 9660] FD32_OPENFILE: %s mode %lx... \n", p->FileName, p->Mode));
			res = iso9660_blockdev_test_unit_ready(v);
			if (res < 0) return res;
			res = iso9660_open_pr(&v->root_dentry, p->FileName, strlen(p->FileName), p->Mode, p->Attr, (File **) &p->FileId);
			LOG_PRINTF(("[ISO 9660] FD32_OPENFILE: done %i\n", res));
			return res;
		}
		case FD32_CLOSE:
		{
			fd32_close_t *p = (fd32_close_t *) params;
			File *f = (File *) p->DeviceId;
			#if ISO9660_CONFIG_DEBUG
			Volume *v = f->dentry->v;
			#endif
			LOG_PRINTF(("[ISO 9660] FD32_CLOSE... \n"));
			res = iso9660_blockdev_test_unit_ready(f->dentry->v);
			if (res < 0) return res;
			res = iso9660_close(f);
			LOG_PRINTF(("[ISO 9660] FD32_CLOSE: done res=%i, %lu files, %u dentries\n",
				res, v->files_open.size, v->num_dentries));
			return res;
		}
		case FD32_FINDFIRST:
		{
			struct fd32_findfirst *p = (struct fd32_findfirst *) params;
			Volume *v = (Volume *) p->volume;
			LOG_PRINTF(("[ISO 9660] FD32_FINDFIRST: %s attrib %02xh... \n", p->path, p->attrib));
			if (v->magic != ISO9660_VOL_MAGIC) return -ENODEV;
			res = iso9660_blockdev_test_unit_ready(v);
			if (res < 0) return res;
			res = iso9660_findfirst_pr(&v->root_dentry, p->path, strlen(p->path), p->attrib, (fd32_fs_dosfind_t *) p->find_data);
			LOG_PRINTF(("[ISO 9660] FD32_FINDFIRST: done %i\n", res));
			return res;
		}
		case FD32_FINDNEXT:
		{
			struct fd32_findnext *p = (struct fd32_findnext *) params;
			Volume *v = (Volume *) p->volume;
			LOG_PRINTF(("[ISO 9660] FD32_FINDNEXT... \n"));
			if (v->magic != ISO9660_VOL_MAGIC) return -ENODEV;
			res = iso9660_blockdev_test_unit_ready(v);
			if (res < 0) return res;
			res = iso9660_findnext(v, (fd32_fs_dosfind_t *) p->find_data);
			LOG_PRINTF(("[ISO 9660] FD32_FINDNEXT: done %i\n", res));
			return res;
		}
		case FD32_FINDFILE:
		{
			struct fd32_findfile *p = (struct fd32_findfile *) params;
			File *f = (File *) p->dir;
			LOG_PRINTF(("[ISO 9660] FD32_FINDFILE: %s flags %08xh\n", p->name, p->flags));
			if (f->magic != ISO9660_FILE_MAGIC) return -EBADF;
			res = iso9660_blockdev_test_unit_ready(f->dentry->v);
			if (res < 0) return res;
			res = iso9660_findfile(f, p->name, strlen(p->name), p->flags, (fd32_fs_lfnfind_t *) p->find_data);
			LOG_PRINTF(("[ISO 9660] FD32_FINDFILE: done %i\n", res));
			return res;
		}
		case FD32_OPEN:
		{
			fd32_open_t *p = (fd32_open_t *) params;
			LOG_PRINTF(("[ISO 9660] FD32_OPEN\n"));
			if (((File *) p->DeviceId)->magic != ISO9660_FILE_MAGIC) return -EBADF;
			return ++((File *) p->DeviceId)->references;
		}
		case FD32_GETATTR:
		{
			fd32_getattr_t *p = (fd32_getattr_t *) params;
			LOG_PRINTF(("[ISO 9660] FD32_GETATTR\n"));
			return iso9660_get_attr((File *) p->FileId, (fd32_fs_attr_t *) p->Attr);
		}
		case FD32_MOUNT:
		{
			fd32_mount_t *p = (fd32_mount_t *) params;
			LOG_PRINTF(("[ISO 9660] FD32_MOUNT\n"));
			return iso9660_mount(p->BlkDev, (Volume **) &p->FsDev);
		}
		case FD32_UNMOUNT:
		{
			fd32_unmount_t *p = (fd32_unmount_t *) params;
			Volume *v = (Volume *) p->DeviceId;
			if (v->magic != ISO9660_VOL_MAGIC) return -ENODEV;
			res = iso9660_blockdev_test_unit_ready(v);
			if (res < 0) return res;
			return iso9660_unmount(v);
		}
		#if 0
		case FD32_PARTCHECK:
			return iso9660_partcheck(((fd32_partcheck_t *) params)->PartId);
		case FD32_GETFSINFO:
			return iso9660_get_fsinfo((fd32_fs_info_t *) ((fd32_getfsinfo_t *) params)->FSInfo);
		#endif
		case FD32_GETFSFREE:
			return iso9660_get_fsfree((fd32_getfsfree_t *) params);
	}
	return -ENOTSUP;
}


#if TEST
static void test()
{
	Volume *v;
	File *f;
	Buffer *b = NULL;
	int res;
	fd32_message("[ISO 9660] Running a test...\n");
	res = iso9660_mount("cdrom0", &v);
	fd32_message("[ISO 9660] Volume mounted with result %i\n", res);
	/* Directory listing */
	res = iso9660_open_pr(&v->root_dentry, "\\", strlen("\\"), O_RDONLY, 0, &f);
	fd32_message("[ISO 9660] File opened with result %i\n", res);
	if (res >= 0)
	{
		for (;;)
		{
			res = iso9660_do_readdir(f, &b, NULL, NULL);
			fd32_message("[ISO 9660] Readdir completed with result %i\n", res);
			if (res < 0) break;
		}
		iso9660_close(f);
	}
	res = iso9660_unmount(v);
	fd32_message("[ISO 9660] Test completed, unmount result %i\n", res);
}
#endif


/* ISO 9660 Driver entry point. Called by the FD32 kernel. */
int iso9660_init()
{
	int res;
	fd32_message("[ISO 9660] Installing the ISO 9660 driver...\n");
	/* Register the driver to the File System Layer */
	res = fd32_add_fs(iso9660_request);
	if (res < 0) return res;
	/* Register the driver request function to the kernel symbol table */
	if (add_call("iso9660_request", (DWORD) iso9660_request, ADD) == -1)
	{
			fd32_message("[ISO 9660] Couldn't add 'iso9660_request' to the symbol table\n");
		return -ENOMEM;
	}
	fd32_message("[ISO 9660] ISO 9660 driver installed.\n");
	#if TEST
	test();
	#endif
	return 0;
}
