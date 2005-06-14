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
/** \file
 * Facilities to open and close files.
 */
#include "fat.h"


/* Checks if the specified File is open.
 * The first_cluster alone may be not enough for file matching:
 * zero length files have first_cluster == 0.
 */
static File *file_is_open(const Volume *v, Cluster first_cluster, Sector de_sector, unsigned de_secoff)
{
	File *f;
	for (f = v->files_open; f; f = f->next)
	{
		if (first_cluster && (f->first_cluster == first_cluster)) return f;
		if (!first_cluster && (f->de_sector == de_sector) && (f->de_secoff == de_secoff)) return f;
	}
	return NULL;
}


/* Gets and initialize a struct File */
static File *file_get(Volume *v, const struct fat_direntry *de, Sector de_sector, unsigned de_secoff)
{
	File *f = v->files_free;
	if (!f)
	{
		unsigned k;
		FileTable *table = (FileTable *) malloc(4096);
		if (!table) return NULL;
		memset(table, 0, 4096);
		for (k = 0; k < FILES_PER_TABLE - 1; k++)
			table->item[k].next = &table->item[k + 1];
		v->files_free = &table->item[0];
		table->next   = v->files;
		v->files      = table;
		f = v->files_free;
	}
	list_erase((ListItem **) &v->files_free, (ListItem *) f);
	list_push_front((ListItem **) &v->files_open, (ListItem *) f);
	memcpy(&f->de, de, sizeof(struct fat_direntry));
	f->de_changed    = false;
	f->de_sector     = de_sector;
	f->de_secoff     = de_secoff;
	f->v             = v;
	f->first_cluster = ((Cluster) de->first_cluster_hi << 16) | (Cluster) de->first_cluster_lo;
	f->references    = 1;
	return f;
}


/* Releases a struct File.
 * If it has no more references, write the directory entry if needed.
 * Returns the updated reference count or a negative error.
 */
static int file_put(File *f)
{
	if (!f->references) return -EBADF;
	if (--f->references == 0)
	{
		Volume *v = f->v;
		#if FAT_CONFIG_WRITE
		if (f->de_changed && f->de_sector)
		{
			Buffer *b = NULL;
			int res = fat_readbuf(v, f->de_sector, &b);
			if (res < 0) return res;
			memcpy(b->data + res + f->de_secofs, &f->de, sizeof(struct fat_direntry));
			f->de_changed = false;
			res = fat_dirtybuf(b, false);
			if (res < 0) return res;
		}
		#endif
		list_erase((ListItem **) &v->files_open, (ListItem *) f);
		list_push_front((ListItem **) &v->files_free, (ListItem *) f);
	}
	return f->references;
}


/* Gets and initialize a struct Channel */
static Channel *channel_get(File *f, int flags)
{
	Volume  *v = f->v;
	Channel *c = v->channels_free;
	if (!c)
	{
		unsigned k;
		ChannelTable *table = (ChannelTable *) malloc(4096);
		if (!table) return NULL;
		memset(table, 0, 4096);
		for (k = 0; k < CHANNELS_PER_TABLE - 1; k++)
			table->item[k].next = &table->item[k + 1];
		v->channels_free = &table->item[0];
		table->next      = v->channels;
		v->channels      = table;
		c = v->channels_free;
	}
	list_erase((ListItem **) &v->channels_free, (ListItem *) c);
	list_push_front((ListItem **) &v->channels_open, (ListItem *) c);
	c->file_pointer   = 0;
	c->f              = f;
	c->magic          = FAT_CHANNEL_MAGIC;
	c->flags          = flags;
	c->references     = 1;
	c->cluster_index  = 0;
	c->cluster        = 0;
	return c;
}


#define fat_mode_to_attributes(x) x
/* Opens the specified file in the specified parent directory.
 * The opened instance is returned in "channel".
 */
static int fat_open1(Channel *restrict parent, Channel **restrict channel, const char *name, size_t name_size, int flags, mode_t mode)
{
	struct fat_direntry *de;
	Volume *v;
	File *f;
	Channel *c;
	int res;
	Cluster first;
	bool existing = true;

	if (!parent || !channel || !name) return -EFAULT;
	v = parent->f->v;
	res = fat_lookup(parent, name, name_size, &v->lud);
	#if FAT_CONFIG_WRITE
	if ((res == -ENOENT) && (flags & O_CREAT))
	{
		int attr = fat_mode_to_attributes(mode);
		res = fat_link(parent, name, attr, &v->lud);
		if (res < 0) return res;
		existing = false;
	}
	else if (res < 0) return res;
	/* Here the file exists */
	if (existing && (flags & O_CREAT) && (flags & O_EXCL)) return -EEXIST;
	#else
	if (res < 0) return res;
	#endif

	/* Check for permissions */
	#if FAT_CONFIG_LFN
	de = &v->lud.cde[v->lud.cde_pos];
	#else
	de = &v->lud.cde;
	#endif
	if ((flags & O_WRONLY) || (flags & O_RDWR) || (flags & O_TRUNC))
	{
		if ((de->attr & FAT_ARDONLY) && existing) return -EACCES;
		if ((de->attr & FAT_ADIR) && !(flags & O_DIRECTORY)) return -EISDIR;
	}
	if ((flags & O_DIRECTORY) && (!(de->attr & FAT_ADIR))) return -ENOTDIR;

	/* Check if we are going to open the root directory, identified as a dir
	 * starting at cluster 0, like in "..", even if the volume is FAT32. */
	first = ((Cluster) de->first_cluster_hi << 16) | (Cluster) de->first_cluster_lo;
	if (!first && (de->attr & FAT_ADIR))
	{
		v->lud.de_sector = 0;
		v->lud.de_secofs = 0;
		de->first_cluster_hi = (uint16_t) (v->root_cluster >> 16);
		de->first_cluster_lo = (uint16_t)  v->root_cluster;
	}
	/* Open the file structure */
	f = file_is_open(v, first, v->lud.de_sector, v->lud.de_secofs);
	if (!f)
	{
		f = file_get(v, de, v->lud.de_sector, v->lud.de_secofs);
		if (!f) return -ENOMEM;
	}
	else f->references++;	
	c = channel_get(f, flags);
	if (!c)
	{
		file_put(f);
		return -ENOMEM;
	}
	res = FD32_ORCREAT;
	if (existing) res = FD32_OROPEN;
	#if FAT_CONFIG_WRITE
	if (flags & O_TRUNC)
	{
		if (existing) res = FD32_ORTRUNC;
		ftruncate(c);
	}
	#endif
	*channel = c;
	return res;
}


static int fat_open_dot(Channel *restrict parent, Channel **restrict channel, int flags)
{
	File *f = parent->f;
	Channel *c;

	LOG_PRINTF(("[FAT2] fat_open_dot, flags=%08xh\n", flags));
	if ((flags & O_CREAT) && (flags & O_EXCL)) return -EEXIST;
	if ((flags & O_WRONLY) || (flags & O_RDWR) || (flags & O_TRUNC))
	{
		if (f->de.attr & FAT_ARDONLY) return -EACCES;
		if ((f->de.attr & FAT_ADIR) && !(flags & O_DIRECTORY)) return -EISDIR;
	}
	if ((flags & O_DIRECTORY) && (!(f->de.attr & FAT_ADIR))) return -ENOTDIR;

	f->references++;	
	c = channel_get(f, flags);
	if (!c)
	{
		file_put(f);
		return -ENOMEM;
	}
	*channel = c;
	#if FAT_CONFIG_WRITE
	if (flags & O_TRUNC)
	{
		ftruncate(c);
		return FD32_ORTRUNC;
	}
	#endif
	return FD32_OROPEN;
}


/* Opens a directory knowing its first cluster for read, and seeks to
 * the offset corresponding to the specified 32-byte directory entry.
 */
int fat_reopen_dir(Volume *v, Cluster first_cluster, unsigned entry_count, Channel **channel)
{
	File *f;
	Channel *c;
	f = file_is_open(v, first_cluster, 0, 0);
	if (!f)
	{
		struct fat_direntry de;
		memset(&de, 0, sizeof(struct fat_direntry));
		de.attr = FAT_ADIR;
		de.first_cluster_hi = (uint16_t) (first_cluster >> 16);
		de.first_cluster_lo = (uint16_t) first_cluster;
		f = file_get(v, &de, 0, 0);
		if (!f) return -ENOMEM;
	}
	else f->references++;	
	c = channel_get(f, O_RDONLY | O_DIRECTORY);
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
/* Backend for the fsync system call.
 * Commits the directory entry and volume buffers.
 */
int fat_fflush(Channel *c)
{
	File *f = c->f;
	/* If the file is not the root, write its directory entry */
	if (f->de_changed && f->de_sector)
	{
		Buffer *b = NULL;
		int res = fat_readbuf(f->v, f->de_sector, &b, false);
		if (res < 0) return res;
		memcpy(b->data + res + f->de_secoff, &f->de, sizeof(struct fat_direntry));
		f->de_changed = false;
		res = fat_dirtybuf(b, false);
		if (res < 0) return res;
	}
	return fat_sync(f->v);
}
#endif


/* Backend for the close system call */
int fat_close(Channel *c)
{
	Volume *v = c->f->v;
	if (!c->references) return -EBADF;
	if (--c->references == 0)
	{
		int res;
		#if FAT_CONFIG_WRITE
		res = fat_fflush(c);
		if (res < 0) return res;
		#endif
		res = file_put(c->f);
		if (res < 0) return res;
		c->magic = 0;
		list_erase((ListItem **) &v->channels_open, (ListItem *) c);
		list_push_front((ListItem **) &v->channels_free, (ListItem *) c);
	}
	#if FAT_CONFIG_DEBUG
	LOG_PRINTF(("[FAT2] fat_flose. Volume buffers: %u hits, %u misses on %u\n",
	            v->buf_hit, v->buf_miss, v->buf_access));
	unsigned k;
	Channel *d;
	File *e;
	for (k = 0, d = v->channels_open; d; d = d->next, k++);
	LOG_PRINTF(("[FAT2] %u channels open, ", k));
	for (k = 0, e = v->files_open; e; e = e->next, k++);
	LOG_PRINTF(("%u files open.\n", k));
	#endif
	return 0;
}


/******************************************************************************/
static const char *get_base_name(const char *file_name, const char *fn_stop)
{
	const char *res = file_name;
	for (; *file_name && (file_name != fn_stop); file_name++)
		if (*file_name == FAT_PATH_SEP) res = file_name + 1;
	return res;
}


/* Opens or creates a file in the specified volume.
 * The "file_name" string is parsed until first between the null terminator,
 * or the byte position "fn_stop".
 */
int fat_open(Volume *v, const char *file_name, const char *fn_stop, int flags, mode_t mode, Channel **channel)
{
	const char *bname;
	Channel *parent;
	Channel *child;
	int res;

	if (v->magic != FAT_VOL_MAGIC) return -ENODEV;
	res = fat_reopen_dir(v, v->root_cluster, 0, &parent);
	if (res < 0) return res;

	/* Descend the path */
	bname = get_base_name(file_name, fn_stop);
	while (*file_name && (file_name != fn_stop))
	{
		const char *comp;
		int real_flags = O_RDONLY | O_DIRECTORY;
		while ((*file_name == FAT_PATH_SEP) && (file_name != fn_stop)) file_name++;
		if (file_name == bname) real_flags = flags;
		for (comp = file_name; *file_name && (file_name != fn_stop) && (*file_name != FAT_PATH_SEP); file_name++);
		if ((comp == file_name) || ((file_name == comp + 1) && (*comp == '.')))
			res = fat_open_dot(parent, &child, flags | O_DIRECTORY);
		else
			res = fat_open1(parent, &child, comp, file_name - comp, real_flags, mode);
		LOG_PRINTF(("[FAT2]      res=%i\n", res));
		fat_close(parent);
		if (res < 0) return res;
		parent = child;
	}
	*channel = parent;
	return res;
}
