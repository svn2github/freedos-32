/* The FreeDOS-32 FAT Driver version 2.0
 * Copyright (C) 2001-2005  Salvatore ISAJA
 *
 * This file "pathreso.c" is part of the FreeDOS-32 FAT Driver (the Program).
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
 * \brief File system facilities with pathname resolution support.
 */
#include "fat.h"

//TODO: Check trailing slash everywhere but open

#if FAT_CONFIG_FD32
 #ifdef LOG_PRINTF
  #undef LOG_PRINTF
 #endif
 #define LOG_PRINTF(x) fd32_log_printf x
#endif


#define EMPTY_PATHNAME_ERROR EINVAL


/* Returns a pointer to the last component of a pathname, that is the part
 * after the last slash. The pathname is not null terminated, in UTF-8.
 */
static const char *get_base_name(const char *pn, size_t pnsize)
{
	const char *res = pn;
	for (; pnsize; pn++, pnsize--)
		if (*pn == FAT_PATH_SEP) res = pn + 1;
	return res;
}


/**
 * \brief Resolve the specified pathname.
 * \param[in]  dentry cached node of the starting lookup directory;
 * \param[out] dentry cached directory node for the resolved pathname;
 * \param      pn     pathname of the to lookup, not null terminated;
 * \param      pnsize length in bytes of the pathname in \c pn;
 * \remarks The output cached directory node must be released with #fat_dput
 *          when no longer used. Using #fat_close on an open file referring to the
 *          cached directory node implicitly calls #fat_dput.
 * \remarks The input \c dentry is released before returning successfully.
 * \remarks Multiple slashes are collapsed to a single slash. A pathname ending
 *          with slash is considered to have a "." component appended to it.
 * \return  0 on success, or a negative error.
 */
static int resolve_path(Dentry **dentry, const char *pn, size_t pnsize)
{
	const char *comp;
	int res;
	while (pnsize)
	{
		while (pnsize && (*pn == FAT_PATH_SEP)) pn++, pnsize--;
		comp = pn;
		while (pnsize && (*pn != FAT_PATH_SEP)) pn++, pnsize--;
		#if 0
		/* Check if ".." jumps out of a mounted tree */
		if ((pn - comp == 2) && (!memcmp(comp, "..", 2))) ;
		#endif
		res = fat_lookup(dentry, comp, pn - comp);
		if (res < 0) return res;
		/* Process symbolic links here */
	}
	return 0;
}


/**
 * \brief  Opens or creates a file with pathname resolution.
 * \param  dentry  cached node of the starting lookup directory;
 * \param  pn      pathname of the file to open, in UTF-8 not null terminated;
 * \param  pnsize  length in bytes of the pathname in \c pn.
 * \param  flags   opening flags;
 * \param  mode    permissions in case the file is created;
 * \param  channel to receive the pointer to the open file description.
 * \retval FD32_OROPEN  existing file successfully opened;
 * \retval FD32_ORTRUNC existing file successfully truncated;
 * \retval FD32_ORCREAT non-existing file successfully created;
 * \retval <0 error.
 */
int fat_open_pr(Dentry *dentry, const char *pn, size_t pnsize, int flags, mode_t mode, Channel **channel)
{
	Volume *v = dentry->v;
	const char *bn;
	size_t bnsize;
	int res;

	if (!pnsize) return -EMPTY_PATHNAME_ERROR;
	bn = get_base_name(pn, pnsize);
	bnsize = pnsize - (bn - pn);
	fat_dget(dentry);
	res = resolve_path(&dentry, pn, bn - pn);
	if (res < 0) goto quit;
	res = resolve_path(&dentry, bn, bnsize); //FOLLOW
	if (!res)
	{
		res = -EEXIST;
		if ((flags & O_CREAT) && (flags & O_EXCL)) goto quit;
		res = fat_open(dentry, flags, channel);
		if (res < 0) goto quit;
		res = FD32_OROPEN;
		if (flags & O_TRUNC)
		{
			res = fat_ftruncate(*channel, 0);
			if (res < 0)
			{
				fat_close(*channel);
				goto quit;
			}
			res = FD32_ORTRUNC;
		}
	}
	else
	{
		assert(res < 0);
		assert(bnsize);
		if ((res == -ENOENT) && (flags & O_CREAT))
		{
			res = fat_create(dentry, bn, bnsize, flags, mode, channel);
			if (res < 0) goto quit;
			res = FD32_ORCREAT;
		}
	}
quit: /* catch-like as well as regular exit point */
	fat_dput(dentry);
	LOG_PRINTF(("[FAT2] OPEN COMPLETE. %lu channels, %lu files, %u dentries.\n",
		v->channels_open.size, v->files_open.size, v->num_dentries));
	return res;
}


/**
 * \brief  Unlinks a file with pathname resolution.
 * \param  dentry cached node of the starting lookup directory;
 * \param  pn     pathname of the file to unlink, in UTF-8 not null terminated;
 * \param  pnsize length in bytes of the pathname in \c pn.
 * \return 0 on success, or a negative error.
 */
int fat_unlink_pr(Dentry *dentry, const char *pn, size_t pnsize)
{
	Volume *v = dentry->v;
	int res;
	if (!pnsize) return -EMPTY_PATHNAME_ERROR;
	fat_dget(dentry);
	res = resolve_path(&dentry, pn, pnsize);
	if (!res) res = fat_unlink(dentry);
	fat_dput(dentry);
	LOG_PRINTF(("[FAT2] UNLINK COMPLETE. %lu channels, %lu files, %u dentries.\n",
		v->channels_open.size, v->files_open.size, v->num_dentries));
	return res;
}


/**
 * \brief  Renames a file with pathname resolution.
 * \param  odentry cached node of the starting lookup directory for the old pathname;
 * \param  on      pathname of the file to rename, in UTF-8 not null terminated;
 * \param  onsize  length in bytes of the pathname in \c on;
 * \param  ndentry cached node of the starting lookup directory for the new pathname;
 * \param  nn      new pathname for the file to rename, in UTF-8 not null terminated;
 * \param  nnsize  length in bytes of the pathname in \c nn.
 * \return 0 on success, or a negative error.
 * \remarks The caller shall assure that the two pathnames, thus the two
 *          starting cached directory nodes, refer to the same file system volume.
 */
int fat_rename_pr(Dentry *odentry, const char *on, size_t onsize,
                  Dentry *ndentry, const char *nn, size_t nnsize)
{
	Volume *v = odentry->v;
	int res;
	const char *bn;
	size_t bnsize;
	if (!onsize || !nnsize) return -EMPTY_PATHNAME_ERROR;
	fat_dget(odentry);
	res = resolve_path(&odentry, on, onsize);
	if (!res)
	{
		bn = get_base_name(nn, nnsize);
		bnsize = nnsize - (bn - nn);
		fat_dget(ndentry);
		res = resolve_path(&ndentry, nn, bn - nn);
		if (!res) res = fat_rename(odentry, ndentry, bn, bnsize);
		fat_dput(ndentry);
	}
	fat_dput(odentry);
	LOG_PRINTF(("[FAT2] RENAME COMPLETE. %lu channels, %lu files, %u dentries.\n",
		v->channels_open.size, v->files_open.size, v->num_dentries));
	return res;
}


/**
 * \brief  Removes a directory with pathname resolution.
 * \param  dentry cached node of the starting lookup directory;
 * \param  pn     pathname of the directory to remove, in UTF-8 not null terminated;
 * \param  pnsize length in bytes of the pathname in \c pn.
 * \return 0 on success, or a negative error.
 */
int fat_rmdir_pr(Dentry *dentry, const char *pn, size_t pnsize)
{
	Volume *v = dentry->v;
	int res;
	if (!pnsize) return -EMPTY_PATHNAME_ERROR;
	fat_dget(dentry);
	res = resolve_path(&dentry, pn, pnsize);
	if (!res) res = fat_rmdir(dentry);
	fat_dput(dentry);
	LOG_PRINTF(("[FAT2] RMDIR COMPLETE. %lu channels, %lu files, %u dentries.\n",
		v->channels_open.size, v->files_open.size, v->num_dentries));
	return res;
}


/**
 * \brief  Creates a directory with pathname resolution.
 * \param  dentry cached node of the starting lookup directory;
 * \param  pn     pathname of the directory to create, in UTF-8 not null terminated;
 * \param  pnsize length in bytes of the pathname in \c pn;
 * \param  mode   permissions for the new directory.
 * \return 0 on success, or a negative error.
 */
int fat_mkdir_pr(Dentry *dentry, const char *pn, size_t pnsize, mode_t mode)
{
	Volume *v = dentry->v;
	int res;
	const char *bn;
	size_t bnsize;

	if (!pnsize) return -EMPTY_PATHNAME_ERROR;
	bn = get_base_name(pn, pnsize);
	bnsize = pnsize - (bn - pn);
	fat_dget(dentry);
	res = resolve_path(&dentry, pn, bn - pn);
	if (!res)
	{
		res = resolve_path(&dentry, bn, bnsize); /* don't follow */
		if (res == -ENOENT) res = fat_mkdir(dentry, bn, bnsize, mode);
		else if (!res) res = -EEXIST;
	}
	fat_dput(dentry);
	LOG_PRINTF(("[FAT2] MKDIR COMPLETE. %lu channels, %lu files, %u dentries.\n",
		v->channels_open.size, v->files_open.size, v->num_dentries));
	return res;
}


/**
 * \brief  Backend for the "FindFirst" DOS system call with pathname resolution.
 * \param  dentry cached node of the starting lookup directory;
 * \param  pn     pathname of the file specification to lookup, in UTF-8 not null terminated;
 * \param  pnsize length in bytes of the pathname in \c pn;
 * \param  attr   DOS attributes that must be matched during the search;
 * \param  df     buffer containing the DOS FindData record (44 bytes).
 * \return 0 on success, or a negative error.
 */
int fat_findfirst_pr(Dentry *dentry, const char *pn, size_t pnsize, int attr, fd32_fs_dosfind_t *df)
{
	Volume *v = dentry->v;
	int res;
	const char *bn;
	size_t bnsize;

	if (!pnsize) return -EMPTY_PATHNAME_ERROR;
	bn = get_base_name(pn, pnsize);
	bnsize = pnsize - (bn - pn);
	fat_dget(dentry);
	res = resolve_path(&dentry, pn, bn - pn);
	if (!res) res = fat_findfirst(dentry, bn, bnsize, attr, df);
	fat_dput(dentry);
	LOG_PRINTF(("[FAT2] FINDFIRST COMPLETE. %lu channels, %lu files, %u dentries.\n",
		v->channels_open.size, v->files_open.size, v->num_dentries));
	return res;
}


#if 0
/* The first element of an opaque file description shall be a pointer to the file operations. */
struct file_operations
{
	off_t   (*lseek)    (void *filedes, off_t offset, int whence);
	ssize_t (*read)     (void *filedes, void *buffer, size_t size);
	ssize_t (*write)    (void *filedes, const void *buffer, size_t size);
	int     (*ftruncate)(void *filedes, off_t length);
	int     (*fstat)    (void *filedes, struct stat *buf);
	int     (*fsync)    (void *filedes, bool datasync);
	int     (*addref)   (void *filedes);
	int     (*close)    (void *filedes);
	int     (*readdir)  (void *filedes, struct dirent *de);
};


/* The first element of an opaque cached directory node shall be a pointer to the dir operations.
 * Any cached directory node shall be released by the caller when no longer used.
 */
struct dir_operations
{
	int  (*lookup)(void **d, const char *fn, size_t fnsize);
	int  (*open)  (void *d, int flags, void **filedes);
	int  (*create)(void *dparent, const char *fn, size_t fnsize, int flags, mode_t mode, void **filedes);
	void (*dget)  (void *d);
	void (*dput)  (void *d);
	int  (*unlink)(void *d);
	int  (*rename)(void *od, void *ndparent, const char *nn, size_t nnsize);
	int  (*link)  (void *od, void *ndparent, const char *nn, size_t nnsize);
	int  (*rmdir) (void *d);
	int  (*mkdir) (void *dparent, const char *fn, size_t fnsize, mode_t mode);
};
#endif
