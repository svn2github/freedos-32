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
/* This file is "dir.c".
 * It contains the facilities to access directories.
 */
#include "leanfs.h"
//#define NDEBUG
#include <assert.h>


/* This functions does the actual work for readdir.
 * It reads the first non-deleted directory entry in the directory "dir",
 * using "de" as directory entry buffer. "de" must be big enough to store
 * a 256-byte name. A null terminator is appended to the name.
 */
static int do_readdir(struct File *dir, struct DirEntry *de)
{
	ssize_t res;
	do
	{
		/* Read the first fields (constant length) */
		res = leanfs_read(dir, de, sizeof(struct DirEntryHeader));
		if (res < 0) return res;
		if (res == 0) return -ENOENT; /* The EOF case */
		if (res != sizeof(struct DirEntryHeader))
			return -EIO; /* Malformed directory */
	
		/* Read the name (the remaining part) */
		res = leanfs_read(dir, (uint8_t *) de + sizeof(struct DirEntryHeader),
		                          de->rec_len - sizeof(struct DirEntryHeader));
		if (res < 0) return res;
		if (res != de->rec_len - sizeof(struct DirEntryHeader))
			return -EIO; /* Malformed directory */
	}
	while (!de->inode);
	de->name[de->name_len] = '\0';
	return 0;
}


/* Backend for the "readdir" or "readdir_r" system call. */
int leanfs_readdir(struct File *dir, void *dirent, readdir_callback_t cb)
{
	struct DirEntry de;
	int             res;

	if (!dir || !dirent || !cb) return -EINVAL;
	if ((dir->pf->inode.attributes & LEANFS_IFMT) != LEANFS_IFDIR) return -ENOTDIR;
	
	res = do_readdir(dir, &de);
	if (res < 0) return res;

	res = DT_REG;
	if (de.type == LEANFS_FT_DIR) res = DT_DIR;
	return cb(dirent, de.name, de.name_len, dir->file_pointer, de.inode, res);
}


/* The buffer pointed by 'de' must be big enough to store a 256-byte name */
int leanfs_find(struct File *dir, const char *name, struct DirEntry *de)
{
	int res;
	if ((dir->pf->inode.attributes & LEANFS_IFMT) != LEANFS_IFDIR) return -ENOTDIR;
	dir->file_pointer = 0;
	while ((res = do_readdir(dir, de)) == 0)
		if (strcmp(name, de->name) == 0)
			return 0;
	return res; /* Includes case -ENOENT, when readdir reaches EOF */
}


#if LEANFS_CONFIG_WRITE
/* Creates a new hard link to the file "pf", named "name", in the parent
 * directory "dir". On success, returns 0, and the parent shall update
 * the link counter of the file. On failure, returns a negative error.
 */
int leanfs_do_link(struct File *dir, uint32_t first_cluster, unsigned ftype, const char *name)
{
	struct DirEntryHeader deh;
	off_t    last_pos;
	ssize_t  res;
	uint32_t zero     = 0;
	unsigned found    = 0;
	unsigned name_len = strlen(name);
	unsigned rec_len  = (sizeof(struct DirEntryHeader) + name_len + 3) & ~3;

	if (name_len > 255) return -ENAMETOOLONG;
	if ((dir->pf->inode.attributes & LEANFS_IFMT) != LEANFS_IFDIR) return -ENOTDIR;

	/* Search for at least "rec_len" bytes of free space.                     */
	/* We may need more than one free entry: free entries are not coealesced. */
	for (;;)
	{
		last_pos = dir->file_pointer;
		res = leanfs_read(dir, &deh, sizeof(struct DirEntryHeader));
		if (res < 0) return res;
		if (res == 0) /* The EOF case */
		{
			found = rec_len;
			break;
		}
		if (res != sizeof(struct DirEntryHeader)) return -EIO; /* Malformed directory */
		if (!deh.inode) found += deh.rec_len;
		           else found = 0;
		if (found >= rec_len) break;
		dir->file_pointer = last_pos + deh.rec_len;
	}

	/* If we arrive here, either we have found at least "rec_len" bytes of */
	/* free space or we are at EOF. IAC we can write our new entry.        */
	dir->file_pointer = last_pos;
	deh.inode    = first_cluster;
	deh.rec_len  = rec_len;
	deh.name_len = (uint8_t) name_len;
	deh.type     = (uint8_t) ftype;
	//TODO: Add recovery on error, or set error bit
	if (found - rec_len < sizeof(struct DirEntryHeader) + 4) deh.rec_len = found;
	res = leanfs_write(dir, &deh, sizeof(struct DirEntryHeader));
	if (res < 0) return res;
	if (res < sizeof(struct DirEntryHeader)) return -ENOSPC;
	res = leanfs_write(dir, name, name_len);
	if (res < 0) return res;
	if ((unsigned) res < name_len) return -ENOSPC;
	res = leanfs_write(dir, &zero, rec_len - name_len - sizeof(struct DirEntryHeader));
	if (res < 0) return res;
	if ((unsigned) res < rec_len - name_len - sizeof(struct DirEntryHeader)) return -ENOSPC;

	/* If after our new entry there is sufficient space for another one, */
	/* let's put a directory entry header to mark it as a free entry.    */
	if (found - rec_len >= sizeof(struct DirEntryHeader) + 4)
	{
		deh.inode    = 0;
		deh.rec_len  = found - rec_len;
		deh.name_len = 0;
		deh.type     = 0;

		res = leanfs_write(dir, &deh, sizeof(struct DirEntryHeader));
		if (res < 0) return res;
		if (res < sizeof(struct DirEntryHeader)) return -ENOSPC;
	}
	dir->file_pointer = last_pos;
	return 0;
}


/* Backend for the "link" system call.
 * Adds to file "old_file" the hard link "new_name" in the directory "new_dir".
 * Returns 0 on success, or a negative error.
 */
int leanfs_link(struct File *old_file, struct File *new_dir, const char *new_name)
{
	int res = leanfs_do_link(new_dir, old_file->pf->first_cluster,
	                         LEANFS_IFMT_TO_FT(old_file->pf->inode.attributes), new_name);
	if (res < 0) return res;
	old_file->pf->inode.links_count++;
	return 0;
}


/* Backend for the "unlink" system call. */
int leanfs_unlink(struct File *parent, const char *name)
{
	struct DirEntry de;
	int res = leanfs_find(parent, name, &de);
	if (res < 0) return res;
	/* Fetch inode and decrease the links count */
	struct PFile *pf = leanfs_is_open(parent->pf->v, de.inode);
	if (!pf)
	{
		struct File *f;
		res = leanfs_open(parent, &f, name, O_WRONLY);
		if (!f->pf->inode.links_count) return -EIO;//TODO: Put this check in open
		f->pf->inode.links_count--;
		res = leanfs_close(f);
	}
	else
	{
		if (!pf->inode.links_count) return -EIO;
		pf->inode.links_count--;
	}
	assert(parent->file_pointer - de.rec_len >= 0);
	parent->file_pointer -= de.rec_len;
	de.inode = 0;
	res = leanfs_write(parent, &de, sizeof(struct DirEntryHeader));
	if (res < 0) return res;
	if (res != sizeof(struct DirEntryHeader)) return -EIO;
	return res;
}


/* Backend for the "mkdir" system call.
 * Creates the directory and writes the "." and ".." entries.
 * Returns a negative error code on failure.
 */
int leanfs_mkdir(struct File *parent, const char *name, mode_t mode)
{
	struct File *f;
	int          res;
	struct
	{
		struct DirEntryHeader dot_deh;
		char dot_name[4];
		struct DirEntryHeader dotdot_deh;
		char dotdot_name[4];
	} e;

	res = leanfs_open(parent, &f, name, O_WRONLY | O_CREAT | O_EXCL | O_DIRECTORY, mode);
	if (res < 0) return res;
	e.dot_deh.inode = f->pf->first_cluster;
	e.dot_deh.rec_len = sizeof(struct DirEntryHeader) + sizeof(e.dot_name);
	e.dot_deh.name_len = 1;
	e.dot_deh.type = LEANFS_FT_DIR;
	strncpy(e.dot_name, ".", sizeof(e.dot_name));
	e.dotdot_deh.inode = parent->pf->first_cluster;
	e.dotdot_deh.rec_len = sizeof(struct DirEntryHeader) + sizeof(e.dotdot_name);
	e.dotdot_deh.name_len = 2;
	e.dotdot_deh.type = LEANFS_FT_DIR;
	strncpy(e.dotdot_name, "..", sizeof(e.dotdot_name));
	res = leanfs_write(f, &e, sizeof(e));
	if (res < 0) return res;
	if (res < sizeof(e)) return -ENOSPC;
	f->pf->inode.links_count += 2;
	return leanfs_close(f);
}


/* Backend for the "rmdir" system call.
 * Removes the directory "name" in "parent". The directory must be empty,
 * that is it must contain only "." and ".." entries, in this order.
 * Returns 0 on success, or a negative error on failure.
 */
int leanfs_rmdir(struct File *parent, const char *name)
{
	struct DirEntry de;
	struct File *f;
	int res;

	parent->file_pointer = 0;
	res = leanfs_open(parent, &f, name, O_RDONLY | O_DIRECTORY);
	if (res < 0) return res;
	res = do_readdir(f, &de);
	if (res < 0) return res;
	if ((de.name_len != 1) || memcpy(de.name, ".", 1)) return -EIO;
	res = do_readdir(f, &de);
	if (res < 0) return res;
	if ((de.name_len != 2) || memcpy(de.name, "..", 2)) return -EIO;
	res = do_readdir(f, &de);
	if (res == 0) return -ENOTEMPTY;
	if (res != -ENOENT) return res;
	/* If we arrive here, the directory is empty, delete it */
	res = leanfs_close(f);
	if (res < 0) return res;
	return 0;//leanfs_do_unlink(parent, name);
}
#endif /* #if LEANFS_CONFIG_WRITE */
