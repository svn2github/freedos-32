/* The FreeDOS-32 ISO 9660 Driver version 0.2
 * Copyright (C) 2005  Salvatore ISAJA
 *
 * This file "pathreso.c" is part of the FreeDOS-32 ISO 9660 Driver (the Program).
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

//TODO: Check trailing slash everywhere but open

#if ISO9660_CONFIG_FD32
 #ifdef LOG_PRINTF
  #undef LOG_PRINTF
 #endif
 #define LOG_PRINTF(x) fd32_log_printf x
#else
 #ifdef LOG_PRINTF
  #undef LOG_PRINTF
 #endif
 #define LOG_PRINTF(x) printf x
#endif


#define EMPTY_PATHNAME_ERROR EINVAL
#define PATH_SEP '\\'


/* Returns a pointer to the last component of a pathname, that is the part
 * after the last slash. The pathname is not null terminated, in UTF-8.
 */
static const char *get_base_name(const char *pn, size_t pnsize)
{
	const char *res = pn;
	for (; pnsize; pn++, pnsize--)
		if (*pn == PATH_SEP) res = pn + 1;
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
		while (pnsize && (*pn == PATH_SEP)) pn++, pnsize--;
		comp = pn;
		while (pnsize && (*pn != PATH_SEP)) pn++, pnsize--;
		#if 0
		/* Check if ".." jumps out of a mounted tree */
		if ((pn - comp == 2) && (!memcmp(comp, "..", 2))) ;
		#endif
		res = iso9660_lookup(dentry, comp, pn - comp);
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
int iso9660_open_pr(Dentry *dentry, const char *pn, size_t pnsize, int flags, mode_t mode, File **file)
{
	Volume *v = dentry->v;
	const char *bn;
	size_t bnsize;
	int res;

	if (!pnsize) return -EMPTY_PATHNAME_ERROR;
	bn = get_base_name(pn, pnsize);
	bnsize = pnsize - (bn - pn);
	iso9660_dget(dentry);
	res = resolve_path(&dentry, pn, bn - pn);
	if (res < 0) goto quit;
	res = resolve_path(&dentry, bn, bnsize); //FOLLOW
	if (!res)
	{
		res = -EEXIST;
		if ((flags & O_CREAT) && (flags & O_EXCL)) goto quit;
		res = iso9660_open(dentry, flags, file);
		if (res < 0) goto quit;
		res = FD32_OROPEN;
		if (flags & O_TRUNC)
		{
			res = EROFS;
			iso9660_close(*file);
			goto quit;
		}
	}
	else
	{
		assert(res < 0);
		assert(bnsize);
		if ((res == -ENOENT) && (flags & O_CREAT))
		{
			res = EROFS;
		}
	}
quit: /* catch-like as well as regular exit point */
	iso9660_dput(dentry);
	LOG_PRINTF(("[ISO 9660] OPEN COMPLETE. %lu files, %u dentries.\n",
		v->files_open.size, v->num_dentries));
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
int iso9660_findfirst_pr(Dentry *dentry, const char *pn, size_t pnsize, int attr, fd32_fs_dosfind_t *df)
{
	Volume *v = dentry->v;
	int res;
	const char *bn;
	size_t bnsize;

	if (!pnsize) return -EMPTY_PATHNAME_ERROR;
	bn = get_base_name(pn, pnsize);
	bnsize = pnsize - (bn - pn);
	iso9660_dget(dentry);
	res = resolve_path(&dentry, pn, bn - pn);
	if (!res) res = iso9660_findfirst(dentry, bn, bnsize, attr, df);
	iso9660_dput(dentry);
	LOG_PRINTF(("[ISO 9660] FINDFIRST COMPLETE. %lu files, %u dentries.\n",
		v->files_open.size, v->num_dentries));
	return res;
}
