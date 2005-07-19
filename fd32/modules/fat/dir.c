/* The FreeDOS-32 FAT Driver version 2.0
 * Copyright (C) 2001-2005  Salvatore ISAJA
 *
 * This file "dir.c" is part of the FreeDOS-32 FAT Driver (the Program).
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
 * \brief Facilities to access directories.
 */
#include "fat.h"


/*****************************************************************************
 *
 * Facilities to manage short file names
 *
 *****************************************************************************/

static const char *invalid_sfn_characters = "\"+,./:;<=>[\\]|*"; /* '?' handled separately */


/**
 * \brief Converts a part (the name or the extension) of a file name to the FCB format.
 * \param nls       NLS operations to use for character set conversion;
 * \param dest      array to hold the converted part in a national code page, not null terminated;
 * \param dest_size size in bytes of \c dest;
 * \param src       array holding the part to convert in UTF-8, not null terminated;
 * \param src_size  size in bytes of \c src;
 * \param wildcards \c true to allow \c '*' and \c '?' wildcards, false otherwise.
 * \retval 0  success;
 * \retval >0 success, some inconvertable characters replaced with \c '_' or name
 *            truncated due to \c dest_size;
 * \retval <0 error.
 */
static int build_fcb_name_part(const struct nls_operations *nls, uint8_t *dest, size_t dest_size,
                               const char *src, size_t src_size, bool wildcards)
{
	int res = 0;
	int skip;
	wchar_t wc;
	while (*src && src_size)
	{
		skip = unicode_utf8towc(&wc, src, src_size);
		if (skip < 0) return skip;
		src_size -= skip;
		src += skip;
		if (wildcards && (wc == (wchar_t) '*'))
		{
			memset(dest, '?', dest_size);
			break;
		}
		skip = nls->wctomb(dest, wc, dest_size);
		if (skip > 0)
		{
			const char *c;
			assert(skip > 0);
			if (*dest < 0x20) return -EINVAL;
			if (!wildcards && (*dest == '?')) return -EINVAL;
			for (c = invalid_sfn_characters; *c; c++)
				if (*dest == *c) return -EINVAL;
			*dest = nls->toupper(*dest);
		}
		if (skip == -EINVAL) skip = nls->wctomb(dest, '_', dest_size), res = 1;
		if (skip == -ENAMETOOLONG) return 1;
		if (skip < 0) return skip;
		dest_size -= skip;
		dest += skip;
	}
	return res;
}


/**
 * \brief Converts a file name to the FCB format.
 * \param nls       NLS operations to use for character set conversion;
 * \param dest      array to hold the converted name, in a national code page, in FCB format;
 * \param src       array holding the file name to convert, in UTF-8 not null terminated;
 * \param src_size  size in bytes of \c src;
 * \param wildcards \c true to allow \c '*' and \c '?' wildcards, false otherwise.
 * \retval  0 success;
 * \retval >0 success, some inconvertable characters replaced with \c '_' or name
 *            truncated to fit in 11 bytes;
 * \retval <0 error.
 */
int fat_build_fcb_name(const struct nls_operations *nls, uint8_t *dest, const char *src, size_t src_size, bool wildcards)
{
	int res1 = 0, res2 = 0;
	const char *dot;

	memset(dest, 0x20, 11);
	/* If source is "." or ".." build ".          " or "..         " */
	if ((src_size == 1) && (*src == '.')) *dest = '.';
	else if ((src_size == 2) && (*((uint16_t *) src) == 0x2E2E)) *((uint16_t *) dest) = 0x2E2E;
	else /* not "." nor ".." */
	{
		dot = memchr(src, '.', src_size);
		if (dot)
		{
			res1 = build_fcb_name_part(nls, dest, 8, src, dot - src, wildcards);
			if (res1 < 0) return res1;
			src = dot + 1;
			src_size -= dot - src + 1;
			res2 = build_fcb_name_part(nls, dest + 8, 3, src, src_size, wildcards);
			if (res2 < 0) return res2;
		}
		else
		{
			res1 = build_fcb_name_part(nls, dest, 8, src, src_size, wildcards);
			if (res1 < 0) return res1;
		}
		if (*dest == ' ') return -EINVAL;
		if (*dest == FAT_FREEENT) *dest = 0x05;
	}
	return res1 || res2;
}


/**
 * \brief Expands a file name in FCB format to a wide character string.
 * \param nls       NLS operations to use for character set conversion;
 * \param dest      array to hold the converted name, not null terminated;
 * \param dest_size max number of wide characters to write in \c dest;
 * \param source    11-bytes array holding the name in FCB format to convert.
 * \return The length in wide characters of the expanded name, or a negative error.
 */
static int expand_fcb_name(const struct nls_operations *nls, wchar_t *dest, size_t dest_size, const uint8_t *source)
{
	const uint8_t *name_end, *ext_end;
	const uint8_t *s = source;
	char aux[13], *a = aux;
	int res;

	/* Skip padding spaces at the end of the name and the extension */
	for (name_end = source + 7; *name_end == ' '; name_end--)
		if (name_end == source) return -EINVAL;
	for (ext_end = source + 10; *ext_end == ' '; ext_end--)
		if (ext_end == source) return -EINVAL;

	/* Copy name dot extension in aux */
	if (*s == 0x05) *a++ = (char) FAT_FREEENT, s++;
	for (; s <= name_end; *a++ = (char) *s++);
	if (source + 8 <= ext_end) *a++ = '.';
	for (s = source + 8; s <= ext_end; *a++ = (char) *s++);
	*a = 0;

	/* Convert aux to a wide character string */
	for (a = aux, res = 0; *a; dest++, dest_size--, res++)
	{
		int skip;
		if (!dest_size) return -ENAMETOOLONG;
		skip = nls->mbtowc(dest, a, aux + sizeof(aux) - a);
		if (skip < 0) return skip;
		a += skip;
	}
	return res;
}


#if FAT_CONFIG_LFN
/*****************************************************************************
 *
 * Facilities to manage long file names
 *
 *****************************************************************************/


/* To fetch a LFN we use a circular buffer of directory entries.
 * A long file name is up to 255 characters long, so 20 entries are
 * needed, plus one additional entry for the short name alias. When,
 * scanning a directory, we find a valid short name, we backtrace
 * the circular buffer to extract the long file name, if present.
 */
#define LFN_FETCH_SLOTS 21
/* Byte offsets of LFN 16-bit characters in a LFN entry */
#define LFN_CHARS_PER_SLOT 13
static const unsigned lfn_chars[LFN_CHARS_PER_SLOT] =
{ 1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30 };


/**
 * \brief  Computes the checksum of the short file name.
 * \param  name short file name in FCB format.
 * \return The 8-bit checksum.
 */
static int lfn_checksum(const uint8_t *name)
{
	int sum = 0;
	unsigned k = 11;
	while (k--) sum = (((sum & 1) << 7) | ((sum & 0xFE) >> 1)) + *name++;
	return sum;
}


/**
 * \brief   Fetches a long file name from LFN special directory entries.
 * \param   c    the open instance of the directory being read;
 * \param   lud  LookupData structure to fetch and store the long file name.
 * \return  0 on success, or a negative error.
 * \remarks On success, \c lud->lfn contains the fetched long file name,
 *          not null terminated, and \c lud->lfn_length contains the size
 *          of the long file name in wide characters.
 * \remarks This function knows about the UTF-16 encoding.
 */
static int fetch_lfn(const Channel *c, LookupData *lud)
{
	struct fat_lfnentry *slot;
	wchar_t *lfn = lud->lfn;
	unsigned lfn_length = 0;
	unsigned k, j, order;
	bool semi = false;
	wchar_t wc = 0; /* avoid warning */
	uint8_t checksum = lfn_checksum(lud->cde[lud->cde_pos].name);
	lud->lfn_entries = 0;
	lud->lfn_length = 0;
	for (order = 1, k = lud->cde_pos; ; order++)
	{
		if (!k)
		{
			if (c->file_pointer < LFN_FETCH_SLOTS * sizeof(struct fat_direntry)) break;
			k += LFN_FETCH_SLOTS;
		}
		k--;
		slot = (struct fat_lfnentry *) &lud->cde[k];
		if (slot->attr != FAT_ALFN) break;
		if (order != (slot->order & 0x1F)) break;
		if (checksum != slot->checksum) break;
		/* This loop extracts wide characters parsing UTF-16 */
		for (j = 0; j < LFN_CHARS_PER_SLOT; j++)
		{
			uint16_t c = *((uint16_t *) ((uint8_t *) slot + lfn_chars[j]));
			if (!semi)
			{
				wc = (wchar_t) c;
				if ((wc & 0xFC00) == 0xD800)
				{
					wc = ((wc & 0x03FF) << 10) + 0x010000;
					semi = true;
				}
			}
			else
			{
				if ((c & 0xFC00) != 0xDC00) return -EILSEQ;
				wc |= (wchar_t) c & 0x03FF;
				semi = false;
			}
			if (!semi)
			{
				if (!wc) break;
				if (++lfn_length == FAT_LFN_MAX) return -ENAMETOOLONG;
				*lfn++ = wc;
			}
		}
		if (slot->order & 0x40)
		{
			lud->lfn_entries = order;
			lud->lfn_length = lfn_length;
			break;
		}
	}
	return 0;
}


#if FAT_CONFIG_WRITE
/**
 * \brief   Splits a long file name in LFN special directory entries.
 * \param   fn       array holding the name to split in UTF-8, not null terminated;
 * \param   fnsize   size in bytes of \c fn;
 * \param   checksum 8-bit checksum of the short file name;
 * \param   lud      LookupData buffer to update with the splitted long file name;
 * \return  0 on success, or a negative error.
 * \remarks On success, the \c lud->cde array contains the splitted long file
 *          name in the last positions (the last element is not used to reserve
 *          space for the short name entry), the and \c lud->lfn_entries
 *          contains the number of LFN directory entries occupied by the
 *          splitted long file name.
 */
static int split_lfn(const char *fn, size_t fnsize, unsigned checksum, LookupData *lud)
{
	unsigned name_pos = 0;
	unsigned slot_pos = 0;
	unsigned order    = 0;
	uint16_t utf16[2];
	wchar_t  wc;
	int      res, k;
	struct fat_lfnentry *slot = (struct fat_lfnentry *) &lud->cde[LFN_FETCH_SLOTS - 2];

	while (fnsize)
	{
		res = unicode_utf8towc(&wc, fn, fnsize);
		if (res < 0) return res;
		fn += res;
		fnsize -= res;
		res = unicode_wctoutf16(utf16, wc, 2);
		if (res < 0) return res;
		for (k = 0; k < res; k++)
		{
			if (slot_pos == LFN_CHARS_PER_SLOT) slot_pos = 0, slot--;
			if (!slot_pos)
			{
				order++; /* 1-based numeration */
				slot->order = order;
				slot->attr = FAT_ALFN;
				slot->reserved = 0;
				slot->checksum = (uint8_t) checksum;
				slot->first_cluster = 0;
			}
			*((uint16_t *) ((uint8_t *) slot + lfn_chars[slot_pos++])) = utf16[k];
			if (++name_pos == FAT_LFN_MAX) return -ENAMETOOLONG;
		}
	}
	slot->order |= 0x40; /* last slot */
	/* Insert a 16-bit null terminator, only if it fits into the slot */
	if (slot_pos < LFN_CHARS_PER_SLOT)
		*((uint16_t *) ((uint8_t *) slot + lfn_chars[slot_pos++])) = 0x0000;
	/* Pad the remaining 16-bit characters of the slot with FFFFh */
	while (slot_pos < LFN_CHARS_PER_SLOT)
		*((uint16_t *) ((uint8_t *) slot + lfn_chars[slot_pos++])) = 0xFFFF;
	lud->lfn_entries = order;
	return 0;
}


/// Characters allowed in long file names but not in short names.
static const wchar_t valid_lfn_characters[] = { 0x2B, 0x2C, 0x3B, 0x3D, 0x5B, 0x5D, 0 }; /* +,;=[] */

/**
 * \brief   Generates a part of a short file name alias for a long file name.
 * \param   nls       NLS operations to use for character set conversion;
 * \param   dest      array to hold the converted part, in a national code page not null terminated;
 * \param   dest_size size in bytes of \c dest;
 * \param   src       array holding the part to convert, in UTF-8 not null terminated;
 * \param   src_size  size in bytes of \c src;
 * \retval   0 success, the name fitted in the short namespace;
 * \retval  >0 success, the name did not fit in the short namespace;
 * \retval  <0 error.
 * \remarks Spaces and dots are removed, inconvertable characters are replaced with
 *          \c '_', all characters are converted to upper case and the name may be
 *          truncated due to \c dest_size.
 * \sa      build_fcb_name_part()
 */
static int gen_short_fname_part(const struct nls_operations *nls, uint8_t *dest, size_t dest_size,
                                const char *src, size_t src_size)
{
	int res = 0;
	int skip;
	wchar_t wc;
	const wchar_t *wcp;
	while (src_size)
	{
		skip = unicode_utf8towc(&wc, src, src_size);
		if (skip < 0) return skip;
		src_size -= skip;
		src += skip;
		if ((wc == ' ') || (wc == '.'))
		{
			res = 1;
			continue;
		}
		for (wcp = valid_lfn_characters; *wcp; wcp++)
			if (wc == *wcp)
			{
				wc = '_';
				res = 1;
				break;
			}
		skip = nls->wctomb(dest, wc, dest_size);
		if (skip > 0)
		{
			int up = nls->toupper(*dest);
			if (up >= 0)
			{
				const char *c;
				*dest = up;
				if (*dest < 0x20) return -EINVAL;
				for (c = invalid_sfn_characters; *c; c++)
					if (*dest == *c) return -EINVAL;
			}
			else skip = -EINVAL;
		}
		if (skip == -EINVAL) skip = nls->wctomb(dest, '_', dest_size), res = 1;
		if (skip == -ENAMETOOLONG) return 1;
		if (skip < 0) return skip;
		dest_size -= skip;
		dest += skip;
	}
	return res;
}


/**
 * \brief  Scans memory for the last embedded dot.
 * \param  s the buffer to scan;
 * \param  n the size in bytes of the buffer.
 * \return A pointer to the last embedded dot if present, or NULL.
 */
static const char *last_embedded_dot(const char *s, size_t n)
{
	const char *dot = NULL;
	if (n > 2)
		for (s++, n--; n--; s++)
			if ((*s == '.') && (*(s - 1) != '.') && (n > 1) && (*(s + 1) != '.'))
				dot = s;
	return dot;
}


/**
 * \brief   Generates an 8.3 alias without numeric tail from a long file name.
 * \param   nls       NLS operations to use for character set conversion;
 * \param   dest      array to hold the converted part in a national code page, in FCB format;
 * \param   src       array holding the long file name to convert, in UTF-8 not null terminated;
 * \param   src_size  size in bytes of \c src;
 * \retval   0 success, the name fitted in the short namespace;
 * \retval  >0 success, the name did not fit in the short namespace;
 * \retval  <0 error.
 * \sa      gen_short_fname_part()
 */
static int gen_short_fname1(const struct nls_operations *nls, uint8_t *dest, const char *src, size_t src_size)
{
	int res1 = 0, res2 = 0;
	const char *dot;

	memset(dest, 0x20, 11);
	/* If source is "." or ".." build ".          " or "..         " */
	if ((src_size == 1) && (*src == '.')) *dest = '.';
	else if ((src_size == 2) && (*((uint16_t *) src) == 0x2E2E)) *((uint16_t *) dest) = 0x2E2E;
	else /* not "." nor ".." */
	{
		dot = last_embedded_dot(src, src_size);
		if (dot)
		{
			res1 = gen_short_fname_part(nls, dest, 8, src, dot - src);
			if (res1 < 0) return res1;
			src_size -= dot - src + 1;
			res2 = gen_short_fname_part(nls, dest + 8, 3, dot + 1, src_size);
			if (res2 < 0) return res2;
		}
		else
		{
			res1 = gen_short_fname_part(nls, dest, 8, src, src_size);
			if (res1 < 0) return res1;
		}
		if (*dest == ' ') return -EINVAL;
		if (*dest == FAT_FREEENT) *dest = 0x05;
	}
	return res1 || res2;
}


/**
 * \brief   Generates an 8.3 alias with numeric tail from a long file name.
 * \param   c         directory where to store the directory entry for file name collision;
 * \param   dest      array to hold the converted part in a national code page, in FCB format;
 * \param   src       array holding the long file name to convert, in UTF-8 not null terminated;
 * \param   src_size  size in bytes of \c src;
 * \retval   0 success, the name fitted in the short namespace;
 * \retval  >0 success, the name did not fit in the short namespace;
 * \retval  <0 error.
 */
static int gen_short_fname(Channel *c, uint8_t *dest, const char *src, size_t src_size, unsigned hint)
{
	struct fat_direntry de;
	char     strcounter[7]; /* "~65535" */
	uint8_t  aux[11];
	Volume  *v = c->f->v;
	unsigned counter;
	int      res;

	res = gen_short_fname1(v->nls, dest, src, src_size);
	if (res <= 0) return res;
	for (counter = hint; counter <= UINT16_MAX; counter++)
	{
		int k;
		uint8_t *b = aux;
		#if FAT_CONFIG_FD32
		int len = ksprintf(strcounter, "~%d", counter);
		#else
		int len = snprintf(strcounter, sizeof(strcounter), "~%d", counter);
		if (len >= sizeof(strcounter)) break;
		#endif
		memcpy(aux, dest, sizeof(aux));
		for (k = 8 - len; k && (*b != ' '); k -= res, b += res)
		{
			res = v->nls->mblen(b, k);
			if (res < 0) break;
		}
		memcpy(b, strcounter, len);
		/* Search for the generated name */
		c->file_pointer = 0;
		for (;;)
		{
			res = fat_read(c, &de, sizeof(struct fat_direntry));
			if (!res || (de.name[0] == FAT_ENDOFDIR))
			{
				memcpy(dest, aux, sizeof(aux));
				return 1;
			}
			if (res < 0) return res;
			if (res != sizeof(struct fat_direntry)) return -EIO; /* malformed directory */
			if ((de.name[0] != FAT_FREEENT) && !(de.attr & FAT_AVOLID)
			 && !memcmp(de.name, aux, sizeof(aux))) break; /* name collision */
		}
	}
	return -EEXIST;
}
#endif /* #if FAT_CONFIG_WRITE */
#endif /* #if FAT_CONFIG_LFN */


/*****************************************************************************
 *
 * Facilities to read directory entries
 *
 *****************************************************************************/


/**
 * \brief  Backend for the readdir and find services.
 * \param  c   the open instance of the directory to read;
 * \param  lud LookupData structure to fill with the read data.
 * \return 0 on success, or a negative error.
 */
int fat_do_readdir(Channel *c, LookupData *lud)
{
	File   *f = c->f;
	Volume *v = f->v;
	if (!(f->de.attr & FAT_ADIR)) return -EBADF;
	#if FAT_CONFIG_LFN
	lud->cde_pos = 0;
	#endif
	for (;;)
	{
		#if FAT_CONFIG_LFN
		struct fat_direntry *de = &lud->cde[lud->cde_pos];
		#else
		struct fat_direntry *de = &lud->cde;
		#endif
		int num_read = fat_read(c, de, sizeof(struct fat_direntry));
		if (num_read < 0) return num_read;
		if (!num_read) break; /* EOF */
		if (num_read != sizeof(struct fat_direntry)) return -EFTYPE; /* malformed directory */
		if (de->name[0] == FAT_ENDOFDIR) break;
		if ((de->name[0] != FAT_FREEENT) && (!(de->attr & FAT_AVOLID) || (de->attr == FAT_AVOLID)))
		{
			int res;
			#if FAT_CONFIG_LFN
			/* Try to extract a long name before the short name entry */
			res = fetch_lfn(c, lud);
			if (res < 0) return res;
			#endif
			res = expand_fcb_name(v->nls, lud->sfn, FAT_SFN_MAX, de->name);
			if (res < 0) return res;
			lud->sfn_length = res;
			fat_direntry_location(c, lud);
			return 0;
		}
		#if FAT_CONFIG_LFN
		if (++lud->cde_pos == LFN_FETCH_SLOTS) lud->cde_pos = 0;
		#endif
	}
	return -ENMFILE;
}


#if 0 //Not defined in the current file system interface
/* The READDIR system call.
 * Reads the next directory entry of the open directory "dir" in
 * the passed LFN finddata structure.
 */
int fat_readdir(File *f, fd32_fs_lfnfind_t *entry)
{
	FindData fd; /* TODO: remove from stack */
	int res;

	if (!entry) return -EFAULT;
	res = fat_do_readdir(f, &fd);
	if (res < 0) return res;

	entry->Attr   = fd.direntry.attr;
	entry->CTime  = fd.direntry.cre_time + (fd.direntry.cre_date << 16);
	entry->ATime  = fd.direntry.acc_date << 16;
	entry->MTime  = fd.direntry.mod_time + (fd.direntry.mod_date << 16);
	entry->SizeHi = 0;
	entry->SizeLo = fd.direntry.file_size;
	((struct FindReserved *) entry->Reserved)->entry_count = f->file_pointer >> 5;
	((struct FindReserved *) entry->Reserved)->first_dir_cluster = f->pf->first_cluster;
	((struct FindReserved *) entry->Reserved)->reserved = 0;
	#if FAT_CONFIG_LFN
	strcpy(entry->LongName, fd.long_name);
	#else
	strcpy(entry->LongName, fd.short_name);
	#endif
	return expand_fcb_name(entry->ShortName, fd.cde[fd.cde_pos].name, sizeof(entry->ShortName));
}
#endif


/**
 * \brief  Compare a UTF-8 string against a wide character string without wildcards.
 * \note   The strings are not null terminated.
 * \return 0 match, >0 don't match, <0 error.
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
 * \brief  Searches an open directory for a file matching the specified name.
 * \note   The file name string is not null terminated.
 * \return On success, returns 0 and fills the passed LookupData structure.
 */
int fat_lookup(Channel *c, const char *fn, size_t fnsize, LookupData *lud)
{
	int res;
	while ((res = fat_do_readdir(c, lud)) == 0)
	{
		#if FAT_CONFIG_LFN
		if (lud->cde[lud->cde_pos].attr & FAT_AVOLID) continue;
		if (!compare_without_wildcards(fn, fnsize, lud->lfn, lud->lfn_length))
			return 0;
		#else
		if (lud->cde.attr & FAT_AVOLID) continue;
		#endif
		if (!compare_without_wildcards(fn, fnsize, lud->sfn, lud->sfn_length))
			return 0;
	}
	if (res == -ENMFILE) res = -ENOENT;
	return res;
}


#if FAT_CONFIG_WRITE
/**
 * \brief  Marks as free the specified directory entries.
 * \param  c     the open directory to free entries in;
 * \param  from  byte offset of the first entry to delete;
 * \param  count number of adjacent entries to delete;
 * \return 0 on success, or a negative error.
 */
static int delete_entries(Channel *c, off_t from, size_t count)
{
	uint8_t b = FAT_FREEENT;
	ssize_t res;
	while (count--)
	{
		res = fat_do_write(c, &b, 1, from);
		if (res < 0) return res;
		if (res != 1) return -EIO;
		from += sizeof(struct fat_direntry);
	}
	return 0;
}


/**
 * \brief   Searches an open directory for free adjacent entries.
 * \param   c the directory to search into;
 * \param   num_entries number of free adjacent entries to find;
 * \return  The not negative byte offset of the first free entry, or a negative error.
 * \remarks The directory is extended, if possible, to find free entries.
 */
static int find_free_entries(Channel *c, unsigned num_entries)
{
	struct fat_direntry de;
	unsigned found = 0;
	int      entry_offset = 0;
	ssize_t  res;
	
	c->file_pointer = 0;
	for (;;)
	{
		res = fat_read(c, &de, sizeof(struct fat_direntry));
		if (res != sizeof(struct fat_direntry)) break;
		if ((de.name[0] == FAT_FREEENT) || (de.name[0] == FAT_ENDOFDIR))
		{
			if (found > 0) found++;
			else
			{
				found        = 1;
				entry_offset = c->file_pointer - res;
			}
		}
		else found = 0;
		if (found == num_entries) return entry_offset;
	}
	if (res)
	{
		if (res < 0) return res;
		if (res != sizeof(struct fat_direntry)) return -EIO; /* malformed directory */
	}
	/* Here the directory is at EOF, hence we try to extend it by writing
	 * a whole cluster filled with zeroes, finding free directory entries
	 * at the beginning of that cluster. */
	entry_offset = c->file_pointer;
	found = 1 << (c->f->v->log_bytes_per_sector + c->f->v->log_sectors_per_cluster); /* bytes per cluster */
	res = fat_do_write(c, NULL, found, c->file_pointer);
	if (res < 0) return res;
	if (res != found) return -EIO; /* malformed directory */
	return entry_offset;
}


/* Allocates 32-bytes directory entries of the specified open directory */
/* to store a long file name, using D as directory entry for the short  */
/* name alias.                                                          */
/* On success, returns the byte offset of the short name entry.         */
/* On failure, returns a negative error code.                           */
/* Called fat_creat and fat_rename.                                     */
int fat_link(Channel *c, const char *fn, size_t fnsize, int attr, unsigned hint, LookupData *lud)
{
	int res;
	#if FAT_CONFIG_LFN
	unsigned k;
	struct fat_direntry *de = &lud->cde[LFN_FETCH_SLOTS - 1];
	lud->cde_pos = LFN_FETCH_SLOTS - 1;
	lud->lfn_entries = 0;
	res = gen_short_fname(c, de->name, fn, fnsize, hint);
	if (res < 0) return res;
	if (res)
	{
		/* This fills lud->cde and lud->lfn_entries */
		res = split_lfn(fn, fnsize, lfn_checksum(de->name), lud);
		if (res < 0) return res;
	}
	res = find_free_entries(c, lud->lfn_entries + 1);
	#else
	struct fat_direntry *de = &lud->cde;
	res = fat_build_fcb_name(c->f->v->nls, de->name, fn, fnsize, false);
	if (res < 0) return res;
	res = find_free_entries(c, 1);
	#endif
	if (res < 0) return res;
	de->attr = (uint8_t) attr;
	de->nt_case = 0;
	fat_timestamps(&de->cre_time, &de->cre_date, &de->cre_time_hund);
	de->acc_date = de->cre_date;
	de->mod_time = de->cre_time;
	de->mod_date = de->mod_date;
	de->first_cluster_hi = 0;
	de->first_cluster_lo = 0;
	de->file_size = 0;
	/* Write the new directory entries into the free entries */
	c->file_pointer = res;
	#if FAT_CONFIG_LFN
	de -= lud->lfn_entries;
	for (k = lud->lfn_entries + 1; k--; de++)
	{
	#endif
		res = fat_do_write(c, de, sizeof(struct fat_direntry), c->file_pointer);
		if (res < 0) return res;
		if (res != sizeof(struct fat_direntry)) return -EIO; /* malformed directory */
		c->file_pointer += res;
	#if FAT_CONFIG_LFN
	}
	#endif
	fat_direntry_location(c, lud);
	return 0;
#if 0
	wchar_t  sfn[FAT_SFN_MAX];
	unsigned sfn_length;
	#if FAT_CONFIG_LFN
	wchar_t  lfn[FAT_LFN_MAX];
	#else
	#endif
#endif
}
#endif /* #if FAT_CONFIG_WRITE */
