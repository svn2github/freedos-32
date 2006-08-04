/* The FreeDOS-32 FAT Driver version 2.0
 * Copyright (C) 2001-2005  Salvatore ISAJA
 *
 * This file "open.c" is part of the FreeDOS-32 FAT Driver (the Program).
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
 * \brief Facilities to open and close files.
 */
/**
 * \addtogroup fat
 * @{
 */
#include "fat.h"


static void file_init(File *f, Volume *v, const struct fat_direntry *de, Sector de_sector, unsigned de_secofs)
{
	memcpy(&f->de, de, sizeof(struct fat_direntry));
	f->de_changed    = false;
	f->de_sector     = de_sector;
	f->de_secofs     = de_secofs;
	f->v             = v;
	f->first_cluster = ((Cluster) de->first_cluster_hi << 16) | (Cluster) de->first_cluster_lo;
	f->references    = 0;
	list_push_front(&v->files_open, (ListItem *) f);
}


/* Gets and initializes a state structure for a file.
 * If the specified File is already open, increase its reference count.
 * Fot the latter task, comparing the first cluster is not enough, since
 * all zero length files have the first cluster equal to 0.
 */
static File *file_get(Volume *v, const struct fat_direntry *de, Sector de_sector, unsigned de_secofs)
{
	File *f;
	Cluster first = ((Cluster) de->first_cluster_hi << 16) | (Cluster) de->first_cluster_lo;
	for (f = (File *) v->files_open.begin; f; f = f->next)
	{
		if (first && (f->first_cluster == first)) break;
		if (!first && (f->de_sector == de_sector) && (f->de_secofs == de_secofs)) break;
	}
	if (!f)
	{
		f = slabmem_alloc(&v->files_slab);
		file_init(f, v, de, de_sector, de_secofs);
	}
	f->references++;
	return f;
}


/* Releases a state structure for a file. */
static void file_put(File *f)
{
	assert (f->references);
	if (--f->references == 0)
	{
		list_erase(&f->v->files_open, (ListItem *) f);
		slabmem_free(&f->v->files_slab, f);
	}
}


/**
 * \brief Increases the reference count of a cached directory node.
 * \param d the cached directory node; the behavior is undefined if it is not valid.
 */
void fat_dget(Dentry *d)
{
	d->references++;
	d->v->num_dentries++;
}


#if FAT_CONFIG_LFN
static void dentry_init(Dentry *d, Dentry *parent, unsigned de_entcnt, Sector de_sector, unsigned attr, unsigned lfn_entries)
#else
static void dentry_init(Dentry *d, Dentry *parent, unsigned de_entcnt, Sector de_sector, unsigned attr)
#endif
{
	d->prev        = NULL;
	d->next        = NULL;
	d->parent      = parent;
	list_init(&d->children);
	d->references  = 0;
	d->v           = parent->v;
	d->de_sector   = de_sector;
	d->attr        = attr;
	d->de_entcnt   = (uint16_t) de_entcnt;
	#if FAT_CONFIG_LFN
	d->lfn_entries = (uint8_t) lfn_entries;
	#endif
	fat_dget(parent);
	list_push_front(&parent->children, (ListItem *) d);
}


/* Gets a cached directory node matching the specified parameters,
 * allocating a new one of there is not already one.
 */
#if FAT_CONFIG_LFN
static Dentry *dentry_get(Dentry *parent, unsigned de_entcnt, Sector de_sector, unsigned attr, unsigned lfn_entries)
#else
static Dentry *dentry_get(Dentry *parent, unsigned de_entcnt, Sector de_sector, unsigned attr)
#endif
{
	Dentry *d;
	for (d = (Dentry *) parent->children.begin; d; d = d->next)
		if (de_entcnt == d->de_entcnt) break;
	if (!d)
	{
		d = slabmem_alloc(&parent->v->dentries_slab);
		if (!d) return NULL;
		#if FAT_CONFIG_LFN
		dentry_init(d, parent, de_entcnt, de_sector, attr, lfn_entries);
		#else
		dentry_init(d, parent, de_entcnt, de_sector, attr);
		#endif
	}
	fat_dget(d);
	return d;
}


/**
 * \brief   Releases a cached directory node.
 * \param   d the cached directory node; the behavior is undefined if it is not valid.
 * \remarks The reference count for the cached directory node shall be
 *          decreased. If it reaches zero, the directory node itself shall
 *          be deallocated, and the procedure shall be repeated recursively
 *          for any parent cached directory node of the file system volume.
 */
void fat_dput(Dentry *d)
{
	Dentry *parent;
	Volume *v = d->v;
	while (d)
	{
		assert(d->v == v);
		assert(d->references);
		assert(v->num_dentries);
		d->references--;
		v->num_dentries--;
		if (d->references) break;
		assert(!d->children.size);
		parent = d->parent;
		if (parent)
		{
			list_erase(&parent->children, (ListItem *) d);
			slabmem_free(&v->dentries_slab, d);
		}
		d = parent;
	}
}


static void channel_init(Channel *c, File *f, Dentry *d, int flags)
{
	c->file_pointer   = 0;
	c->f              = f;
	c->magic          = FAT_CHANNEL_MAGIC;
	c->flags          = flags;
	c->references     = 1;
	c->cluster_index  = 0;
	c->cluster        = 0;
	c->dentry         = d;
	list_push_front(&f->v->channels_open, (ListItem *) c);
}


/* Allocates and initializes an open instance of a file, and increases
 * the reference count of the associated cached directory node.
 */
static Channel *channel_get(File *f, Dentry *d, int flags)
{
	Channel *c = slabmem_alloc(&f->v->channels_slab);
	if (c)
	{
		channel_init(c, f, d, flags);
		if (d) fat_dget(d);
	}
	return c;
}


/**
 * \brief  Opens an existing file.
 * \param  dentry  cached directory node of the file to open;
 * \param  flags   opening flags;
 * \param  channel to receive the pointer to the open file description.
 * \return 0 on success, or a negative error (\c channel unchanged).
 * \remarks On success, the cached directory node shall be associated to the
 *          open file description, thus its reference count shall be increased.
 */
int fat_open(Dentry *dentry, int flags, Channel **channel)
{
	struct fat_direntry de;
	Volume  *v = dentry->v;
	Buffer  *b = NULL;
	File    *f;
	Channel *c;
	int      res;
	unsigned de_secofs = ((off_t) dentry->de_entcnt * sizeof(struct fat_direntry))
	                   & (v->bytes_per_sector - 1);

	/* Check for permissions */
	if ((flags & O_WRONLY) || (flags & O_RDWR) || (flags & O_TRUNC))
	{
		if (dentry->attr & FAT_ARDONLY) return -EACCES;
		if (!(flags & O_DIRECTORY) && (dentry->attr & FAT_ADIR)) return -EISDIR;
	}
	if ((flags & O_DIRECTORY) && !(dentry->attr & FAT_ADIR)) return -ENOTDIR;
	/* Fetch the directory entry, or synthesize one for the root */
	if (dentry->parent)
	{
		res = fat_readbuf(v, dentry->de_sector, &b, false);
		if (res < 0) return res;
		memcpy(&de, b->data + res + de_secofs, sizeof(struct fat_direntry));
	}
	else
	{
		memset(&de, 0, sizeof(struct fat_direntry));
		de.attr = FAT_ADIR;
		de.first_cluster_hi = (uint16_t) (v->root_cluster >> 16);
		de.first_cluster_lo = (uint16_t) v->root_cluster;
	}
	/* Open a file description */
	f = file_get(v, &de, dentry->de_sector, de_secofs);
	if (!f) return -ENOMEM;
	c = channel_get(f, dentry, flags);
	if (!c)
	{
		file_put(f);
		return -ENOMEM;
	}
	assert(dentry->references >= 2);
	*channel = c;
	return 0;
}


#define mode_to_attributes(x) x
/**
 * \brief  Creates a new file in the specified directory.
 * \param  dparent cached node of the directory where to create the file in;
 * \param  fn      name of the file to create, in UTF-8 not null terminated;
 * \param  fnsize  length in bytes of the file name in \c fn;
 * \param  flags   opening flags (\c O_CREAT | \c O_EXCL assumed);
 * \param  mode    permissions for the new file;
 * \param  channel to receive the pointer to the open file description.
 * \return 0 on success, or a negative error.
 * \remarks The cached directory node of the parent directory shall be released.
 */
int fat_create(Dentry *dparent, const char *fn, size_t fnsize, int flags, mode_t mode, Channel **channel)
{
	struct fat_direntry *de;
	Volume  *v = dparent->v;
	File    *f = NULL;
	Dentry  *d = NULL;
	Channel *c = NULL;
	Channel *cparent = NULL;
	int      res;

	/* Validate the access mode and open the parent */
	mode = mode_to_attributes(mode);
	if (flags & O_DIRECTORY) mode |= FAT_ADIR;
	res = fat_open(dparent, O_RDWR | O_DIRECTORY, &cparent);
	if (res < 0) return res;
	/* Make sure there is room for the state structures for the new file */
	res = -ENOMEM;
	f = slabmem_alloc(&v->files_slab);
	d = slabmem_alloc(&v->dentries_slab);
	c = slabmem_alloc(&v->channels_slab);
	if (!f || !d || !c) goto error;
	/* Create a link to the new file */
	res = fat_link(cparent, fn, fnsize, mode, 1, &v->lud);
	if (res < 0) goto error;
	res = fat_close(cparent);
	if (res < 0) goto error;
	#if FAT_CONFIG_LFN
	de = &v->lud.cde[v->lud.cde_pos];
	#else
	de = &v->lud.cde;
	#endif
	/* Initialize the state structures */
	file_init(f, v, de, v->lud.de_sector, v->lud.de_secofs);
	f->references = 1;
	assert(v->lud.de_dirofs % sizeof(struct fat_direntry) == 0);
	#if FAT_CONFIG_LFN
	dentry_init(d, dparent, v->lud.de_dirofs / sizeof(struct fat_direntry), v->lud.de_sector, de->attr, v->lud.lfn_entries);
	#else
	dentry_init(d, dparent, v->lud.de_dirofs / sizeof(struct fat_direntry), v->lud.de_sector, de->attr);
	#endif
	fat_dget(d);
	channel_init(c, f, d, flags);
	*channel = c;
	return 0;
error:
	if (f) slabmem_free(&v->files_slab, f);
	if (d) slabmem_free(&v->dentries_slab, d);
	if (c) slabmem_free(&v->channels_slab, c);
	if (cparent) fat_close(cparent);
	return res;
}


/* Opens a directory knowing its first cluster for read, and seeks to
 * the offset corresponding to the specified 32-byte directory entry.
 * Only used for DOS-style FindFirst and FindNext services.
 */
int fat_reopen_dir(Volume *v, Cluster first_cluster, unsigned entry_count, Channel **channel)
{
	struct fat_direntry de;
	File *f;
	Channel *c;
	memset(&de, 0, sizeof(struct fat_direntry));
	de.attr = FAT_ADIR;
	de.first_cluster_hi = (uint16_t) (first_cluster >> 16);
	de.first_cluster_lo = (uint16_t) first_cluster;
	f = file_get(v, &de, 0, 0);
	if (!f) return -ENOMEM;
	c = channel_get(f, 0, O_RDONLY | O_DIRECTORY); //TODO: Missing Dentry
	if (!c)
	{
		file_put(f);
		return -ENOMEM;
	}
	c->file_pointer = (off_t) entry_count << 5;
	*channel = c;
	return FD32_OROPEN;
}


#if FAT_CONFIG_WRITE
/**
 * \brief  Backend for the "fsync" and "fdatasync" POSIX system calls.
 * \param  c        open instance of the file to commit;
 * \param  datasync if nonzero, do not commit the directory entry, only the buffered data.
 * \return 0 on success, or a negative error.
 */
int fat_fsync(Channel *c, bool datasync)
{
	File *f;
	if (c->magic != FAT_CHANNEL_MAGIC) return -EBADF;
	f = c->f;
	if (!datasync && f->de_changed && f->de_sector && (f->de_sector != FAT_UNLINKED))
	{
		Buffer *b = NULL;
		int res = fat_readbuf(f->v, f->de_sector, &b, false);
		if (res < 0) return res;
		memcpy(b->data + res + f->de_secofs, &f->de, sizeof(struct fat_direntry));
		f->de_changed = false;
		res = fat_dirtybuf(b, false);
		if (res < 0) return res;
	}
	return fat_sync(f->v);
}
#endif


/**
 * \brief   Backend for the "close" POSIX system call.
 * \param   c open instance of the file to close.
 * \return  0 on success, or a negative error.
 * \remarks If there are no other open instances, the file description and the
 *          associated cached directory node shall be released even on I/O error.
 */
int fat_close(Channel *c)
{
	File *f;
	Volume *v;
	int res = 0;
	if ((c->magic != FAT_CHANNEL_MAGIC) || !c->references) return -EBADF;
	f = c->f;
	v = f->v;
	c->references--;
	#if FAT_CONFIG_WRITE
	if (!c->references && (f->de_sector == FAT_UNLINKED))
		res = fat_delete_clusters(v, f->first_cluster);
	if (!res) res = fat_fsync(c, false);
	#endif
	if (!c->references)
	{
		if (c->dentry) fat_dput(c->dentry);
		file_put(f);
		c->magic = 0;
		list_erase(&v->channels_open, (ListItem *) c);
		slabmem_free(&v->channels_slab, c);
	}

	LOG_PRINTF(("[FAT2] fat_flose (res=%i). Volume buffers: %u hits, %u misses on %u\n",
		res, v->buf_hit, v->buf_miss, v->buf_access));
	LOG_PRINTF(("[FAT2] %u channels open, %u files open, %u dentries.\n",
		v->channels_open.size, v->files_open.size, v->num_dentries));
	return res;
}


/* Compares a UTF-8 file name against a wide character file name without
 * wildcards. The strings are not null terminated.
 * Return value: 0 match, >0 don't match, <0 error.
 */
static int compare_without_wildcards(const char *s1, size_t n1, const wchar_t *s2, size_t n2)
{
	wchar_t wc;
	int skip;
	while (n1 && n2)
	{
		skip = unicode_utf8towc(&wc, s1, n1);
		if (skip < 0) return skip;
		if (unicode_simple_fold(wc) != unicode_simple_fold(*s2)) return 1;
		n1 -= skip;
		s1 += skip;
		n2--;
		s2++;
	}
	if (n1 || n2) return 1;
	return 0;
}


/**
 * \brief Searches a directory for a specified file.
 * \param[in]  dentry cached node of the directory to search in;
 * \param[out] dentry cached directory node for the file found;
 * \param      fn     name of the file too lookup (not "."), not null terminated;
 * \param      fnsize length in bytes of the file name in \c fn;
 * \remarks On success, the output cached directory node reference count shall
 *          be increased (it shall be at least 1), and the input cached
 *          directory node shall be released. On error the input cached
 *          directory node shall be unchanged.
 * \return  0 on success, or a negative error.
 */
int fat_lookup(Dentry **dentry, const char *fn, size_t fnsize)
{
	LookupData *lud;
	Channel *c;
	Dentry *d;
	int res;
	assert((*dentry)->references);
	/* Do "." fast. Consider "" the same as "." */
	if (!fnsize || ((fnsize == 1) && (*fn == '.')))
	{
		res = -ENOTDIR;
		if ((*dentry)->attr & FAT_ADIR) res = 0;
		return res;
	}
	/* Do ".." fast */
	if ((fnsize == 2) && (!memcmp(fn, "..", 2)))
	{
		d = (*dentry)->parent;
		assert(d->attr & FAT_ADIR);
		if (d)
		{
			fat_dget(d);
			fat_dput(*dentry);
			*dentry = d;
		}
		return 0;
	}
	/* Not "..", lookup the file name. */
	res = fat_open(*dentry, O_RDONLY | O_DIRECTORY, &c);
	if (res < 0) return res;
	assert((*dentry)->references >= 2);
	lud = &c->f->v->lud;
	for (;;)
	{
		res = fat_do_readdir(c, lud);
		if (res < 0)
		{
			if (res == -ENMFILE) res = -ENOENT;
			goto quit;
		}
		#if FAT_CONFIG_LFN
		if (lud->cde[lud->cde_pos].attr & FAT_AVOLID) continue;
		res = compare_without_wildcards(fn, fnsize, lud->lfn, lud->lfn_length);
		if (!res) break;
		if (res < 0) goto quit;
		#else
		if (lud->cde.attr & FAT_AVOLID) continue;
		#endif
		res = compare_without_wildcards(fn, fnsize, lud->sfn, lud->sfn_length);
		if (!res) break;
		if (res < 0) goto quit;
	}
	res = -ENOMEM;
	assert(lud->de_dirofs % sizeof(struct fat_direntry) == 0);
	#if FAT_CONFIG_LFN
	d = dentry_get(*dentry, lud->de_dirofs / sizeof(struct fat_direntry), lud->de_sector, lud->cde[lud->cde_pos].attr, lud->lfn_entries);
	#else
	d = dentry_get(*dentry, lud->de_dirofs / sizeof(struct fat_direntry), lud->de_sector, lud->cde.attr);
	#endif
	if (d)
	{
		fat_dput(*dentry);
		*dentry = d;
		res = 0;
	}
quit:
	fat_close(c);
	return res;
}

/* @} */
