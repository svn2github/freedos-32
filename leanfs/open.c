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
/* This file is "open.c".
 * It contains the facilities to open and close files.
 */
#include "leanfs.h"
//#define NDEBUG
#include <assert.h>
#include <stdarg.h>


static struct PFileTable *pfiles_head = NULL;
static struct FileTable  *files_head  = NULL;


/* Caches the specified inode.
 * If the inode is already cached, the cached one is used. It the inode is not
 * cached, fetches it. The inode cache is extended if needed.
 * On success, returns 0 and fills "out_pf".
 */
static int take_pfile(struct Volume *v, uint32_t first_cluster, struct PFile **out_pf)
{
	struct PFileTable *prev_pft = NULL, *pft;
	struct PFile      *free_pf  = NULL, *pf;
	struct Buffer     *b = NULL;
	int                res;

	assert(v);
	assert(first_cluster);
	assert(out_pf);

	/* If the specified PFile is already open, just increment its reference */
	/* count and return it. At the same time, search for the first free     */
	/* struct PFile to be used if the specified PFile is not yet open.      */
	for (pft = pfiles_head; pft; prev_pft = pft, pft = pft->next)
	{
		pf = pft->pf;
		for (res = 0; res < PFILES_PER_TABLE; res++, pf++)
		{
			if ((pf->v == v) && (pf->first_cluster == first_cluster) && (pf->references))
			{
				pf->references++;
				*out_pf = pf;
				return 0;
			}
			if (!pf->references) free_pf = pf;
		}
	}
	/* If we arrive here, the specified PFile is not yet open. Open it. */
	if (!free_pf)
	{
		/* No free struct PFile available. Allocate and append a new table. */
		pft = (struct PFileTable *) malloc(sizeof(struct PFileTable));
		if (!pft) return -ENOMEM;
		if (prev_pft) prev_pft->next = pft;
		         else pfiles_head    = pft;
		memset(pft, 0, sizeof(struct PFileTable));
		free_pf = pft->pf;
	}

	/* Read the cluster. Don't associate it to the PFile yet, since read may    */
	/* fail and the file wouldn't be open. We will associate it manually later. */
	res = leanfs_readbuf(v, first_cluster, NULL, &b);
	if (res < 0) return res;
	//assert(b->pf == 0); /* Since we are opening the file for the first time */
	memcpy(&free_pf->inode, b->data + res, sizeof(struct Inode));
	b->pf = free_pf;
	free_pf->first_cluster = first_cluster;
	free_pf->v             = (struct Volume *) v;
	free_pf->references    = 1;
	*out_pf = free_pf;
	return 0;
}


/* Releases a cached inode.
 * If it has no more references, the inode is written to disk.
 * Returns the number of references after release, or a negative error.
 */
static int release_pfile(struct PFile *pf)
{
	if (!pf->references) return -EBADF;
	#if LEANFS_CONFIG_WRITE
	if (--pf->references == 0)
	{
		struct Buffer *b = NULL;
		int res = leanfs_readbuf(pf->v, pf->first_cluster, pf, &b);
		if (res < 0) return res;
		memcpy(b->data, &pf->inode, sizeof(struct Inode));
		res = leanfs_dirtybuf(b, false);
		if (res < 0) return res;
	}
	#endif
	return pf->references;
}


/* Checks if the specified inode is open (i.e. cached).
 * Returns the cached inode, or NULL if it is not open.
 */
struct PFile *leanfs_is_open(const struct Volume *v, uint32_t first_cluster)
{
	struct PFileTable *pft;
	struct PFile *pf;
	unsigned k;
	for (pft = pfiles_head; pft; pft = pft->next)
	{
		pf = pft->pf;
		for (k = 0; k < PFILES_PER_TABLE; k++, pf++)
			if (pf->references && (pf->v == v) && (pf->first_cluster == first_cluster))
				return pf;
	}
	return NULL;
}


static struct File *take_file(struct PFile *pf, int flags)
{
	struct FileTable *prev_ft = NULL, *ft;
	struct File      *free_f  = NULL, *f;
	unsigned          k;
	
	/* If the specified PFile is already open, just increment its reference */
	/* count and return it. At the same time, search for the first free     */
	/* struct PFile to be used if the specified PFile is not yet open.      */
	for (ft = files_head; ft; prev_ft = ft, ft = ft->next)
	{
		f = ft->f;
		for (k = 0; k < FILES_PER_TABLE; k++, f++)
			if (!f->references)
				free_f = f;
	}
	/* If we arrive here, the specified PFile is not yet open. Open it. */
	if (!free_f)
	{
		/* No free struct PFile available. Allocate and append a new table. */
		ft = (struct FileTable *) malloc(sizeof(struct FileTable));
		if (!ft) return NULL;
		if (prev_ft) prev_ft->next = ft;
		        else files_head    = ft;
		memset(ft, 0, sizeof(struct FileTable));
		free_f = ft->f;
	}
	free_f->pf             = pf;
	free_f->flags          = flags;
	free_f->references     = 1;
	free_f->magic          = LEANFS_FILE_MAGIC;
	free_f->file_pointer   = 0;
	free_f->icluster       = 0;
	free_f->icluster_first = 0;
	return free_f;
}


#if 0
/* Releases a linked channel to a file.
 * The file is deleted if there are no hard links to it.
 * Returns 0 on success, or a negative error on failure.
 */
static int release_file(struct File *f)
{
	int res = -EBADF;
	if (f->references)
	{
		if (--f->references == 0)
			release_pfile(f->pf);
		res = 0;
		if (!f->pf->inode.links_count)
			res = leanfs_delete_clusters(f, 0);
	}
	return res;
}
#endif


/* Synchronizes the file positions of all files referencing a cached inode
 * so that their cached cluster position is not past "new_clusters_count".
 */
void leanfs_validate_pos(const struct PFile *pf)
{
	struct FileTable *ft;
	struct File *f;
	unsigned k;
	for (ft = files_head; ft; ft = ft->next)
	{
		f = ft->f;
		for (k = 0; k < FILES_PER_TABLE; k++, f++)
			if (f->references && (f->pf == pf) && (f->icluster_first >= pf->inode.clusters_count))
			{
				f->icluster = 0;
				f->icluster_first = 0;
			}
	}
}


#define leanfs_mode_to_attributes(x) x
int leanfs_open(struct File *__restrict__ parent, struct File **__restrict__ file, const char *name, int flags, ...)
{
	struct PFile   *pf;
	int             res;
	struct DirEntry de;
	bool            create = false;

	if (!parent || !file || !name) return -EINVAL;
	res = leanfs_find(parent, name, &de);
	assert(res <= 0);
	if ((res == -ENOENT) && (flags & O_CREAT))
	{
		va_list ap;
		va_start(ap, flags);
		mode_t mode = va_arg(ap, mode_t);
		va_end(ap);
		uint32_t attributes = leanfs_mode_to_attributes(mode);
		res = leanfs_create_inode(parent, attributes, name);
		if (res < 0) return res;
		res = leanfs_find(parent, name, &de);
		if (res < 0) return res;
		create = true;
	}
	else if (res < 0) return res;
	/* Here the file exists, cache its inode */
	if (!create && (flags & O_CREAT) && (flags & O_EXCL))
		return -EEXIST;
	res = take_pfile(parent->pf->v, de.inode, &pf);
	if (res < 0) return res;

	/* Check for permissions */
	if (!create)
	{
		res = -EACCES;
		if (!(pf->inode.attributes & LEANFS_IRUSR))
			if ((flags & O_RDONLY) || (flags & O_RDWR))
				goto bad_out;
		if (!(pf->inode.attributes & LEANFS_IWUSR))
			if ((flags & O_WRONLY) || (flags & O_RDWR) || (flags & O_TRUNC))
				goto bad_out;
	}
	res = -EISDIR;
	if (((pf->inode.attributes & LEANFS_IFMT) == LEANFS_IFDIR) && !(flags & O_DIRECTORY))
		if ((flags & O_WRONLY) || (flags & O_RDWR) || (flags & O_TRUNC))
			goto bad_out;
	res = -ENOTDIR;
	if ((flags & O_DIRECTORY) && ((pf->inode.attributes & LEANFS_IFMT) != LEANFS_IFDIR))
		goto bad_out;

	res = -ENOMEM;
	*file = take_file(pf, flags);
	if (!(*file)) goto bad_out;
	return 0;

bad_out:
	release_pfile(pf);
	return res;
}


/* Closes a linked channel to a file.
 * The reference counter of the file structure is decreased.
 * If there are no other open instances of the file, all buffers are flushed
 * and the file is eventaully deleted if it has no hard links.
 * Returns 0 on success, or a negative error on failure.
 */
int leanfs_close(struct File *f)
{
	if (!f->references) return -EBADF;
	if (--f->references == 0)
		release_pfile(f->pf);
	#if LEANFS_CONFIG_WRITE
	if (!f->pf->references)
	{
		int res;
		if (!f->pf->inode.links_count)
		{
			res = leanfs_delete_clusters(f, 0);
			if (res < 0) return res;
		}
		res = leanfs_sync(f->pf->v);//_fsync(f->pf);
		/* TODO: Invalidate all buffers associated to this PFile */
		if (res < 0) return res;
	}
	#endif
	return 0;
}


#define PATHSEP '/'
static char *leanfs_basename(const char *file_name)
{
	/* Move to the end of the file_name strings */
	const char *s = strchr(file_name, '\0');
	if (s == file_name) return NULL;

	/* Skip all trailing path separators, if any */
	for (--s; (*s == PATHSEP) && (s != file_name); s--);
	if (s == file_name) return NULL;

	/* Move backwards until the first path separator (if any) is found */
	for (; (*s != PATHSEP) && (s != file_name); s--);
	if (*s == PATHSEP) s++;
	return (char *) s;
}


int fs_open(struct Volume *v, const char *file_name, int flags, struct File **f)
{
	char         comp[NAME_MAX];
	char        *pcomp = comp;
	const char  *bname;
	struct File *parent;
	struct File *child;
	int          res;

	struct PFile *pf;
	res = take_pfile(v, v->sb->root_start_cluster, &pf);
	if (res < 0) return res;
	parent = take_file(pf, O_RDONLY | O_DIRECTORY);
	
	if (*file_name == PATHSEP)
	{
		/* Open the root directory in parent */
	}
	else
	{
		return -EINVAL;
		/* Dup (open?) the current directory in parent */
	}
	
	bname = leanfs_basename(file_name);
	
	/* Descend the path */
	while (*file_name)
	{
		int real_flags = O_RDONLY | O_DIRECTORY;

		/* Get the next file name component */
		while (*file_name == PATHSEP) file_name++;
		if (file_name == bname) real_flags = flags;
		for (pcomp = comp; *file_name && (*file_name != PATHSEP); *pcomp++ = *file_name++);
		*pcomp = 0;
		if (*file_name == PATHSEP) real_flags |= O_DIRECTORY;

		res = leanfs_open(parent, &child, comp, real_flags);
		if (res < 0) return res;
		leanfs_close(parent);
		parent = child;
	}
	*f = parent;
	return 0;
}
