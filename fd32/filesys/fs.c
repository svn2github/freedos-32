/**************************************************************************
 * FreeDOS32 File System Layer                                            *
 * Wrappers for file system driver functions and JFT support              *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2002-2005, Salvatore Isaja                               *
 *                                                                        *
 * This is "fs.c" - Wrappers for file system drivers' miscellaneous       *
 *                  service (e.g. open, rename, read, delete, etc.).      *
 *                                                                        *
 *                                                                        *
 * This file is part of the FreeDOS32 File System Layer (the SOFTWARE).   *
 *                                                                        *
 * The SOFTWARE is free software; you can redistribute it and/or modify   *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation; either version 2 of the License, or (at  *
 * your option) any later version.                                        *
 *                                                                        *
 * The SOFTWARE is distributed in the hope that it will be useful, but    *
 * WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with the SOFTWARE; see the file GPL.txt;                         *
 * if not, write to the Free Software Foundation, Inc.,                   *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA                *
 **************************************************************************/

/* TODO: Character device detection is completely broken */
//#define CHARDEVS

#include <ll/i386/hw-data.h>

#include <devices.h>
#include <filesys.h>
#include <errno.h>
#ifndef NULL
#define NULL 0
#endif


#ifdef CHARDEVS
/* TODO: Character devs ABSOLUTELY WRONG! */
static fd32_dev_file_t *is_char_device(char *FileName)
{
  char           *s;
  fd32_dev_t     *Dev;
  
  /* Move to the end of the FullPath string */
  for (s = FileName; *s; s++);
  /* Now move backward until '\' is found. The name starts next the '\' */
  for (; (*s != '\\') && (s != FileName); s--);
  if (*s == '\\') s++;
  if (*s == 0) return NULL;
  if ((Dev = fd32_dev_find(s)) == NULL) return NULL;
  return fd32_dev_query(Dev, FD32_DEV_FILE);
}
#endif


int fs_open(char *file_name, DWORD mode, WORD attr, WORD alias_hint, fd32_request_t **request, void **file)
{
	fd32_request_t  *r;
	void            *device_id;
	char            *path;
	char             aux_name[FD32_LFNPMAX];
	fd32_openfile_t  of;
	int              res;

	#if 0 //def CHARDEVS
	/* First check if we are requested to open a character device */
	if ((F = is_char_device(FileName)))
	{
		F->open(F->P);
		Jft[Handle].Ops = F;
		if (Result) *Result = FD32_OROPEN;
		return Handle;
	}
	#endif

	/* Otherwise call the open function of the file system device */
	res = fd32_truename(aux_name, file_name, FD32_TNSUBST);
	if (res < 0) return res;
	do
	{
		res = fd32_get_drive(aux_name, &r, &device_id, &path);
		if (res < 0) return res;
		of.Size      = sizeof(fd32_openfile_t);
		of.DeviceId  = device_id;
		of.FileName  = path;
		of.Mode      = mode;
		of.Attr      = attr;
		of.AliasHint = alias_hint;
		res = r(FD32_OPENFILE, &of);
	}
	while (res == -ENOTMOUNT);
	if (res >= 0)
	{
		*request = r;
		*file    = of.FileId;
	}
	return res;
}


int fs_close(fd32_request_t *request, void *file)
{
	fd32_close_t p;
	p.Size     = sizeof(fd32_close_t);
	p.DeviceId = file;
	return request(FD32_CLOSE, &p);
}


int fs_fflush(fd32_request_t *request, void *file)
{
	fd32_fflush_t p;
	p.Size     = sizeof(fd32_fflush_t);
	p.DeviceId = file;
	return request(FD32_FFLUSH, &p);
}


ssize_t fs_read(fd32_request_t *request, void *file, void *buffer, size_t count)
{
	fd32_read_t p;
	p.Size        = sizeof(fd32_read_t);
	p.DeviceId    = file;
	p.Buffer      = buffer;
	p.BufferBytes = count;
	return request(FD32_READ, &p);
}


ssize_t fs_write(fd32_request_t *request, void *file, const void *buffer, size_t count)
{
	fd32_write_t p;
	p.Size        = sizeof(fd32_write_t);
	p.DeviceId    = file;
	p.Buffer      = (void *) buffer; /* TODO: cleanup const modifier */
	p.BufferBytes = count;
	return request(FD32_WRITE, &p);
}


int fd32_unlink(char *file_name, DWORD flags)
{
	fd32_request_t *request;
	fd32_unlink_t p;
	char aux[FD32_LFNPMAX];
	int  res;

	res = fd32_truename(aux, file_name, FD32_TNSUBST);;
	if (res < 0) return res;
	p.Size  = sizeof(fd32_unlink_t);
	p.Flags = flags;
	for (;;)
	{
		res = fd32_get_drive(aux, &request, &p.DeviceId, &p.FileName);
		if (res < 0) return res;
		res = request(FD32_UNLINK, &p);
		if (res != -ENOTMOUNT) return res;
	}
}


off_t fs_lseek(fd32_request_t *request, void *file, off_t offset, int whence)
{
	fd32_lseek_t p;
	int res;
	p.Size     = sizeof(fd32_lseek_t);
	p.DeviceId = file;
	p.Offset   = offset;
	p.Origin   = whence;
	res = request(FD32_LSEEK, &p);
	if (res < 0) return res;
	return p.Offset;
}


int fs_get_attributes(fd32_request_t *request, void *file, fd32_fs_attr_t *a)
{
	fd32_getattr_t p;
	p.Size   = sizeof(fd32_getattr_t);
	p.FileId = file;
	p.Attr   = (void *) a;
	return request(FD32_GETATTR, &p);
}


int fs_set_attributes(fd32_request_t *request, void *file, const fd32_fs_attr_t *a)
{
	fd32_setattr_t p;
	p.Size   = sizeof(fd32_setattr_t);
	p.FileId = file;
	p.Attr   = (void *) a;
	return request(FD32_SETATTR, &p);
}


int fd32_rename(/*const*/ char *old_name, /*const */char *new_name)
{
	fd32_request_t *request_old;
	fd32_request_t *request_new;
	fd32_rename_t   p;
	void *dev_old;
	void *dev_new;
	char  aux_old[FD32_LFNPMAX];
	char  aux_new[FD32_LFNPMAX];
	int   res;

	res = fd32_truename(aux_old, old_name, FD32_TNSUBST);
	if (res < 0) return res;
	res = fd32_truename(aux_new, new_name, FD32_TNSUBST);
	if (res < 0) return res;
	for (;;)
	{
		res = fd32_get_drive(aux_old, &request_old, &dev_old, &p.OldName);
		if (res < 0) return res;
		res = fd32_get_drive(aux_new, &request_new, &dev_new, &p.NewName);
		if (res < 0) return res;
		if ((request_old != request_new) || (dev_old != dev_new)) return -EXDEV;
		p.Size     = sizeof(fd32_rename_t);
		p.DeviceId = dev_old;
		res = request_old(FD32_RENAME, &p);
		if (res != -ENOTMOUNT) return res;
	}
}


int fd32_get_fsinfo(char *pathname, fd32_fs_info_t *fsinfo)
{
	fd32_request_t *request;
	fd32_getfsinfo_t p;
	char aux[FD32_LFNPMAX];
	int  res;

	res = fd32_truename(aux, pathname, FD32_TNSUBST);
	if (res < 0) return res;
	p.Size   = sizeof(fd32_getfsinfo_t);
	p.FSInfo = fsinfo;
	for (;;)
	{
		res = fd32_get_drive(aux, &request, &p.DeviceId, NULL);
		if (res < 0) return res;
		res = request(FD32_GETFSINFO, &p);
		if (res != -ENOTMOUNT) return res;
	}
}


int fd32_get_fsfree(char *pathname, fd32_getfsfree_t *fsfree)
{
	fd32_request_t *request;
	char aux[FD32_LFNPMAX];
	int res;

	res = fd32_truename(aux, pathname, FD32_TNSUBST);
	if (res < 0) return res;
	if (fsfree->Size < sizeof(fd32_getfsfree_t)) return -EINVAL;
	for (;;)
	{
		res = fd32_get_drive(aux, &request, &fsfree->DeviceId, NULL);
		if (res < 0) return res;
		res = request(FD32_GETFSFREE, fsfree);
		if (res != -ENOTMOUNT) return res;
	}
}


/* TODO: mkdir should forbid directory creation if path name becomes
         longer than 260 chars for LFN or 64 chars for short names. */
int fd32_mkdir(/*const*/ char *dir_name)
{
	fd32_request_t *request;
	fd32_mkdir_t p;
	char aux[FD32_LFNPMAX];
	int res;

	res = fd32_truename(aux, dir_name, FD32_TNSUBST);
	if (res < 0) return res;
	p.Size = sizeof(fd32_mkdir_t);
	for (;;)
	{
		res = fd32_get_drive(aux, &request, &p.DeviceId, &p.DirName);
		if (res < 0) return res;
		res = request(FD32_MKDIR, &p);
		if (res != -ENOTMOUNT) return res;
	}
}


int fd32_rmdir(/*const*/ char *dir_name)
{
	fd32_request_t *request;
	fd32_rmdir_t p;
	char aux[FD32_LFNPMAX];
	int res;

	res = fd32_truename(aux, dir_name, FD32_TNSUBST);
	if (res < 0) return res;
	p.Size = sizeof(fd32_rmdir_t);
	for (;;)
	{
		res = fd32_get_drive(aux, &request, &p.DeviceId, &p.DirName);
		if (res < 0) return res;
		res = request(FD32_RMDIR, &p);
		if (res != -ENOTMOUNT) return res;
	}
}
