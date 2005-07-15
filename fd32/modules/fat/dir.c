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


/* Converts a part (the name or the extension) of a file name in UTF-8 to
 * the FCB format, allowing wildcards if required.
 * The source string may be not null terminated, the converted string is not.
 * Returns: 0 success; >0 success, some inconvertable characters replaced with '_'
 *          or name truncated due to "dest_size"; <0 error.
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
		if (skip == -EINVAL) skip = nls->wctomb(dest, '_', dest_size);
		if (skip == -ENAMETOOLONG) return 1;
		else
		{
			const char *c;
			assert(skip > 0);
			if (*dest < 0x20) return -EINVAL;
			if (!wildcards && (*dest == '?')) return -EINVAL;
			for (c = invalid_sfn_characters; *c; c++)
				if (*dest == *c) return -EINVAL;
			*dest = nls->toupper(*dest);
			dest_size -= skip;
			dest += skip;
		}
	}
	return res;
}


/* Validates an UTF-8 null terminated short file name and converts it to FCB format.
 * Returns: 0 success; >0 success, some inconvertable characters replaced with '_'
 *          or name truncated fit in 11 bytes; <0 error.
 * TODO: add src_size
 */
int fat_build_fcb_name(const struct nls_operations *nls, uint8_t *dest, const char *source, bool wildcards)
{
	int res1, res2 = 0;
	const char *dot;
	memset(dest, 0x20, 11);
	/* If source is "." or ".." build ".          " or "..         " */
	if (*source == '.')
	{
		if (*(source + 1) == 0)
		{
			*dest = '.';
			return 0;
		}
		if ((*(source + 1) == '.') && (*(source + 2) == 0))
		{
			*dest++ = '.';
			*dest = '.';
			return 0;
		}
	}
	/* Not "." nor ".." */
	dot = strchr(source, '.');
	res1 = build_fcb_name_part(nls, dest, 8, source, dot ? dot - source : (size_t) -1, wildcards);
	if (res1 < 0) return res1;
	if (dot)
	{
		res2 = build_fcb_name_part(nls, dest + 8, 3, dot + 1, (size_t) -1, wildcards);
		if (res2 < 0) return res2;
	}
	if (*dest == ' ') return -EINVAL;
	if (*dest == FAT_FREEENT) *dest = 0x05;
	return res1 || res2;
}


/* Gets a wide character short file name from an FCB file name.
 * The converted name is not null terminated, its length is returned.
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


/* Computes and returns the 8-bit checksum for a LFN from the
 * corresponding short name directory entry.
 */
static uint8_t lfn_checksum(const struct fat_direntry *de)
{
	uint8_t sum = 0;
	size_t i;
	for (i = 0; i < sizeof(de->name); i++)
		sum = (((sum & 1) << 7) | ((sum & 0xFE) >> 1)) + de->name[i];
	return sum;
}


/* Fetches a wide character long file name from the passed lookup data.
 * The fetched name is not null terminated, its length is stored too.
 * This function knows about the UTF-16 encoding.
 * On success, returns 0 and updates the "lud" structure.
 */
static int fetch_lfn(const Channel *c, LookupData *lud)
{
	struct fat_lfnentry *slot;
	wchar_t *lfn = lud->lfn;
	unsigned lfn_length = 0;
	unsigned k, j, order;
	bool semi = false;
	wchar_t wc = 0; /* avoid warning */
	uint8_t checksum = lfn_checksum(&lud->cde[lud->cde_pos]);
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
#endif /* if FAT_CONFIG_LFN */


/*****************************************************************************
 *
 * Facilities to read directory entries
 *
 *****************************************************************************/


/* Backend for the readdir and find services.
 * On success, returns 0 and fills the "lud" structure.
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
			lud->de_dirofs = c->file_pointer - num_read;
			lud->de_sector = lud->de_dirofs >> v->log_bytes_per_sector;
			if (!f->de_sector && !f->first_cluster)
				lud->de_sector += v->root_sector;
			else
				lud->de_sector = (lud->de_sector & (v->sectors_per_cluster - 1))
				               + ((c->cluster - 2) << v->log_sectors_per_cluster) + v->data_start;
			lud->de_secofs = lud->de_dirofs & (v->bytes_per_sector - 1);
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


/* Compare a UTF-8 string against a wide character string without wildcards.
 * The strings are not null terminated.
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


/* Searches an open directory for a file matching the specified name.
 * On success, returns 0 and fills the passed LookupData structure.
 */
int fat_lookup(Channel *c, const char *fn, size_t fnsize, LookupData *lud)
{
	int res;
	while ((res = fat_do_readdir(c, lud)) == 0)
	{
		if (lud->cde[lud->cde_pos].attr & FAT_AVOLID) continue;
		#if FAT_CONFIG_LFN
		if (!compare_without_wildcards(fn, fnsize, lud->lfn, lud->lfn_length))
			return 0;
		#endif
		if (!compare_without_wildcards(fn, fnsize, lud->sfn, lud->sfn_length))
			return 0;
	}
	if (res == -ENMFILE) res = -ENOENT;
	return res;
}
