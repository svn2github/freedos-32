/* The FreeDOS-32 ISO 9660 Driver version 0.2
 * Copyright (C) 2005  Salvatore ISAJA
 *
 * This file "dentry.c" is part of the FreeDOS-32 ISO 9660 Driver (the Program).
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


/**
 * \brief Increases the reference count of a cached directory node.
 * \param d the cached directory node; the behavior is undefined if it is not valid.
 */
void iso9660_dget(Dentry *d)
{
	d->references++;
	d->v->num_dentries++;
}


/**
 * \brief   Releases a cached directory node.
 * \param   d the cached directory node; the behavior is undefined if it is not valid.
 * \remarks The reference count for the cached directory node shall be
 *          decreased. If it reaches zero, the directory node itself shall
 *          be deallocated, and the procedure shall be repeated recursively
 *          for any parent cached directory node of the file system volume.
 */
void iso9660_dput(Dentry *d)
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


/* Gets a cached directory node matching the specified parameters,
 * allocating a new one of there is not already one.
 */
static Dentry *dentry_get(Dentry *parent, Block de_block, unsigned de_blkofs, unsigned flags)
{
	Dentry *d;
	for (d = (Dentry *) parent->children.begin; d; d = d->next)
		if ((de_block == d->de_block) && (de_blkofs == d->de_blkofs)) break;
	if (!d)
	{
		d = slabmem_alloc(&parent->v->dentries_slab);
		if (!d) return NULL;
		d->prev        = NULL;
		d->next        = NULL;
		d->parent      = parent;
		list_init(&d->children);
		d->references  = 0;
		d->v           = parent->v;
		d->de_block    = de_block;
		d->de_blkofs   = de_blkofs;
		d->flags       = flags;
		iso9660_dget(parent);
		list_push_front(&parent->children, (ListItem *) d);
	}
	iso9660_dget(d);
	return d;
}


static File *take_file(Volume *v)
{
	File *f = slabmem_alloc(&v->files_slab);
	if (f) list_push_front(&v->files_open, (ListItem *) f);
	return f;
}


int iso9660_open(Dentry *dentry, int flags, File **file)
{
	File *f;
	int res = flags & O_ACCMODE;

	if ((flags & O_CREAT) || (flags & O_TRUNC) || (res == O_WRONLY) || (res == O_RDWR)) return -EROFS;
	if ((flags & O_DIRECTORY) && !(dentry->flags & ISO9660_FL_DIR)) return -ENOTDIR;
	if (dentry->flags & (ISO9660_FL_ME | ISO9660_FL_REC)) return -ENOTSUP;

	/* Fetch the directory record, or synthesize one for the root */
	if (dentry->parent)
	{
		struct iso9660_dir_record *dr;
		Buffer *b = NULL;
		res = iso9660_readbuf(dentry->v, dentry->de_block, &b);
		if (res < 0) return res;
		dr = (struct iso9660_dir_record *) (b->data + res + dentry->de_blkofs);
		f = take_file(dentry->v);
		if (!f) return -ENOMEM;
		f->extent         = ME(dr->extent);
		f->data_length    = ME(dr->data_length);
		f->len_ear        = dr->len_ear;
		//f->recording_time = 0;
		memcpy(&f->timestamp, &dr->recording_time, sizeof(struct iso9660_dir_record_timestamp));
		f->file_flags     = dr->flags;
	}
	else
	{
		f = take_file(dentry->v);
		if (!f) return -ENOMEM;
		f->extent         = dentry->v->root_extent;
		f->data_length    = dentry->v->root_data_length;
		f->len_ear        = dentry->v->root_len_ear;
		//f->recording_time = 0;
		memcpy(&f->timestamp, &dentry->v->root_recording_time, sizeof(struct iso9660_dir_record_timestamp));
		f->file_flags     = ISO9660_FL_DIR;
	}
	f->magic          = ISO9660_FILE_MAGIC;
	f->file_pointer   = 0;
	f->open_flags     = flags;
	f->references     = 1;
	f->dentry         = dentry;
	iso9660_dget(dentry);
	assert(dentry->references >= 2);
	*file = f;
	return 0;
}


int iso9660_lookup(Dentry **dentry, const char *fn, size_t fnsize)
{
	#if ISO9660_CONFIG_DOS
	char fnfcb[11];
	char rnfcb[11];
	#endif
	struct iso9660_dir_record *dr;
	Buffer  *buffer = NULL;
	Block    de_block;
	unsigned de_blkofs;
	File    *f;
	Dentry  *d;
	int      res;
	assert((*dentry)->references);
	/* Do "." fast. Consider "" the same as "." */
	if (!fnsize || ((fnsize == 1) && (*fn == '.')))
	{
		res = -ENOTDIR;
		if ((*dentry)->flags & ISO9660_FL_DIR) res = 0;
		return res;
	}
	/* Do ".." fast */
	if ((fnsize == 2) && (!memcmp(fn, "..", 2)))
	{
		d = (*dentry)->parent;
		assert(d->flags & ISO9660_FL_DIR);
		if (d)
		{
			iso9660_dget(d);
			iso9660_dput(*dentry);
			*dentry = d;
		}
		return 0;
	}
	/* Not "..", lookup the file name. */
	res = iso9660_open(*dentry, O_RDONLY | O_DIRECTORY, &f);
	if (res < 0) return res;
	assert((*dentry)->references >= 2);
	#if ISO9660_CONFIG_DOS
	res = iso9660_build_fcb_name(fnfcb, fn, fnsize, false);
	if (res < 0) goto quit;
	#endif
	for (;;)
	{
		unsigned len_fi;
		#if ISO9660_CONFIG_STRIPVERSION
		char *sep2;
		#endif
		res = iso9660_do_readdir(f, &buffer, &de_block, &de_blkofs);
		if (res < 0)
		{
			if (res == -ENMFILE) res = -ENOENT;
			goto quit;
		}
		dr = (struct iso9660_dir_record *) (buffer->data + res + de_blkofs);
		len_fi = dr->len_fi;
		#if ISO9660_CONFIG_STRIPVERSION
		sep2 = memchr(dr->file_id, ';', dr->len_fi);
		if (sep2) len_fi = sep2 - dr->file_id;
		#endif
		if ((fnsize == len_fi) && !memcmp(dr->file_id, fn, fnsize)) break;
		#if ISO9660_CONFIG_DOS
		res = iso9660_build_fcb_name(rnfcb, dr->file_id, len_fi, false);
		if (res < 0) continue; /* Ignore invalid any file identifier */
		if (!memcmp(fnfcb, rnfcb, 11)) break;
		#endif
	}
	res = -ENOMEM;
	d = dentry_get(*dentry, de_block, de_blkofs, dr->flags);
	if (d)
	{
		iso9660_dput(*dentry);
		*dentry = d;
		res = 0;
	}
quit:
	iso9660_close(f);
	return res;
}
