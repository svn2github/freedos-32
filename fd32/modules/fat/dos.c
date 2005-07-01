/* The FreeDOS-32 FAT Driver version 2.0
 * Copyright (C) 2001-2005  Salvatore ISAJA
 *
 * This file "dos.c" is part of the FreeDOS-32 FAT Driver (the Program).
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
 * \brief Driver services only relevant to a DOS system.
 */
#include "fat.h"


/* Format of the private fields of the DOS find data block */
struct DosFindData
{
	uint8_t  search_drive;        /* A=1 etc., remote if bit 7 set */
	uint8_t  search_template[11]; /* Name to find in FCB format, '?' allowed */
	uint8_t  search_attr;         /* Search attributes */
	uint16_t entry_count;         /* Entry count position within the directory */
	uint32_t first_dir_cluster;   /* First cluster of the search directory */
	uint16_t reserved;
} PACKED;


/* Compares two file names in FCB format. "sw" may contain '?' wildcards.
 * Returns: zero=match, nonzero=not match.
 * Remarks: don't know how this is supposed to work with MBCS.
 */
static int compare_fcb_names(const uint8_t *s, const uint8_t *sw)
{
	unsigned k;
	for (k = 0; k < 11; k++)
		if ((sw[k] != '?') && (s[k] != sw[k])) return 1;
	return 0;
}


/* Converts a string of wide characters (that may be not null terminated) to
 * a null terminated UTF-8 string, using no more than "dest_size" bytes.
 * Returns the total length in bytes of the converted string.
 */
static int wchar_string_to_utf8(char *dest, size_t dest_size, const wchar_t *src, size_t src_size)
{
	char *dest_save = dest;
	while (*src && src_size)
	{
		int res = unicode_wctoutf8(dest, *src, dest_size);
		if (res < 0) return res;
		dest += res;
		dest_size -= res;
		src++;
		src_size--;
	}
	if (dest_size < 1) return -ENAMETOOLONG;
	*dest = 0;
	return (int) (dest - dest_save);
}


/* Backend for the DOS-style "findfirst" and "findnext" system calls.
 * Searches an open directory for the specified template, then closes the directory.
 * On success, returns 0 and fills the output fields of the find data buffer.
 */
static int dos_find(Channel *c, fd32_fs_dosfind_t *df)
{
	LookupData *lud = &c->f->v->lud;
	struct DosFindData *dfd = (struct DosFindData *) df;
	uint8_t unallowed_attr = ~(dfd->search_attr | FAT_AARCHIV | FAT_ARDONLY);
	int res;
	while ((res = fat_do_readdir(c, lud)) == 0)
	{
		#if FAT_CONFIG_LFN
		struct fat_direntry *de = &lud->cde[lud->cde_pos];
		#else
		struct fat_direntry *de = &lud->cde;
		#endif
		/* According to the RBIL, if search attributes are not 08h (volume
		 * label) all files with at most the specified attributes should be
		 * returned (archive and readonly are always returned), otherwise
		 * only the volume label should be returned.
		 */
		if (dfd->search_attr != FAT_AVOLID)
		{
			if (de->attr & unallowed_attr) continue;
		}
		else
		{
			if (de->attr != FAT_AVOLID) continue;
		}
		/* Now that attributes match, compare the file names */
		if (compare_fcb_names(de->name, dfd->search_template) == 0)
		{
			df->Attr  = de->attr;
			df->MTime = de->mod_time;
			df->MDate = de->mod_date;
			df->Size  = de->file_size;
			/* TODO: Should convert to the code page used by the DPMI driver */
			res = wchar_string_to_utf8(df->Name, sizeof(df->Name), lud->sfn, lud->sfn_length);
			if (res < 0) break;
			dfd->entry_count = c->file_pointer >> 5;
			dfd->first_dir_cluster = c->f->first_cluster;
			res = 0;
			break;
		}
	}
	fat_close(c);
	return res;
}


/* Same functionality as the GNU version of basename in glibc */
static const char *get_base_name(const char *file_name)
{
	const char *res = file_name;
	for (; *file_name; file_name++)
		if (*file_name == FAT_PATH_SEP) res = file_name + 1;
	return res;
}


/* The DOS-style "findfirst" system call */
int fat_findfirst(Volume *v, const char *fn, int attr, fd32_fs_dosfind_t *df)
{
	const char *bname = get_base_name(fn);
	int res;
	Channel *c;
	df->SearchAttr = attr;
	res = fat_build_fcb_name(v->nls, df->SearchTemplate, bname, true);
	if (res < 0) return res;
	if (res > 0) return -ENOENT; /* Can't exist in the short names namespace */
	res = fat_open(v, fn, bname, O_RDONLY | O_DIRECTORY, 0, &c);
	if (res < 0) return res;
	res = dos_find(c, df);
	if (res == -ENMFILE) res = -ENOENT; /* Yet another convention */
	return res;
}


/* The DOS-style "findnext" system call */
int fat_findnext(Volume *v, fd32_fs_dosfind_t *df)
{
	struct DosFindData *dfd = (struct DosFindData *) df;
	Channel *c;
	int res = fat_reopen_dir(v, dfd->first_dir_cluster, dfd->entry_count, &c);
	if (res < 0) return res;
	return dos_find(c, df);
}


/* Checks if "s1" (which may contain '?' wildcards) matches the beginning
 * of "s2". Both strings are not null terminated.
 * Returns the number of matched wide characters if the string match, zero
 * if the strings don't match, or a negative error.
 */
static int compare_pattern(const char *s1, size_t n1, const wchar_t *s2, size_t n2)
{
	int res = 0;
	wchar_t wc;
	int skip;
	while (n1 && n2)
	{
		skip = unicode_utf8towc(&wc, s1, n1);
		if (skip < 0) return skip;
		if ((wc != '?') && (unicode_simple_fold(wc) != unicode_simple_fold(*s2))) return 0;
		n1 -= skip;
		s1 += skip;
		n2--;
		s2++;
		res++;
	}
	if (n1) return 0;
	return res;
}


/* Compare "s1" (which may contain '*' and '?' with their standard meaning)
 * against "s2". Both strings are not null terminated.
 * Return value: 0 match, >0 don't match, <0 error.
 */
static int compare_with_wildcards(const char *s1, size_t n1, const wchar_t *s2, size_t n2)
{
	int res = 0; /* avoid warnings */
	bool shift = false;
	while (n1)
	{
		if (*s1 == '*')
		{
			shift = true;
			s1++;
			n1--;
			if (!n1) return 0;
			continue;
		}
		if (!n2) break;
		const char *asterisk = memchr(s1, '*', n1);
		size_t nsub = n1;
		if (asterisk) nsub = asterisk - s1;
		if (shift)
		{
			for (; n2; n2--, s2++)
			{
				res = compare_pattern(s1, nsub, s2, n2);
				if (res) break;
			}
		}
		else res = compare_pattern(s1, nsub, s2, n2);
		if (res < 0) return res;
		if (!res) return 1;
		n1 -= nsub;
		s1 += nsub;
		n2 -= res;
		s2 += res;
	}
	if (n1 || n2) return 1;
	return 0;
}


#if FAT_CONFIG_FD32
/* FIXME: memrchr not provided by OSLib! */
static void *memrchr(const void *s, int c, size_t n)
{
	void *res = NULL;
	const unsigned char *us = (const unsigned char *) s;
	unsigned char uc = (unsigned char) c;
	for (; n; us++, n--) if (*us == uc) res = (void *) us;
	return res;
}
#endif


static const wchar_t *wmemrchr(const wchar_t *s, wchar_t c, size_t n)
{
	const wchar_t *res = NULL;
	for (; n; s++, n--) if (*s == c) res = s;
	return res;
}


/* Compare "s1" (which may contain wildcards) against "s2".
 * The wildcard matching is DOS-compatible: if "s1" contains any dots,
 * the last dot is used as a conventional extension separator, that needs
 * not to be present in "s2". The name and extension parts are then compared
 * separately, so that "*.*" matches any file like "*" (even if not containing
 * a dot), and "*." matches files with no extension (even if not ending with a
 * dot). Both strings are not null terminated.
 * Return value: 0 match, >0 don't match, <0 error.
 */
static int compare_with_wildcards_compat(const char *s1, size_t n1, const wchar_t *s2, size_t n2)
{
	const char *s1dot = memrchr(s1, '.', n1);
	if (s1dot)
	{
		int res1, res2;
		size_t n1nam = s1dot - s1;
		size_t n1ext = n1 - n1nam - 1;
		size_t n2nam = n2;
		size_t n2ext = 0;
		const wchar_t *s2dot = wmemrchr(s2, '.', n2);
		if (s2dot)
		{
			n2nam = s2dot - s2;
			n2ext = n2 - n2nam - 1;
		}
		res1 = compare_with_wildcards(s1, n1nam, s2, n2nam);
		if (res1 < 0) return res1;
		res2 = compare_with_wildcards(s1dot + 1, n1ext, s2dot + 1, n2ext);
		if (res2 < 0) return res2;
		return res1 || res2;
	}
	return compare_with_wildcards(s1, n1, s2, n2);
}


/* Backend for the long file names find facilities */
int fat_findfile(Channel *c, const char *fn, size_t fnsize, int flags, fd32_fs_lfnfind_t *lfnfind)
{
	int res;
	int unallowed_attr = ~((flags & 0x00FF) | FD32_AARCHIV | FD32_ARDONLY);
	int required_attr  = (flags & 0xFF00) >> 16;
	LookupData *lud = &c->f->v->lud;
	while ((res = fat_do_readdir(c, lud)) == 0)
	{
		#if FAT_CONFIG_LFN
		struct fat_direntry *de = &lud->cde[lud->cde_pos];
		#else
		struct fat_direntry *de = &lud->cde;
		#endif
		if (!(de->attr & unallowed_attr) && ((de->attr & required_attr) == required_attr))
			if ((lud->lfn_length && !compare_with_wildcards_compat(fn, fnsize, lud->lfn, lud->lfn_length))
			 || !compare_with_wildcards_compat(fn, fnsize, lud->sfn, lud->sfn_length))
			{
				lfnfind->Attr   = (DWORD) de->attr;
				lfnfind->CTime  = (QWORD) de->cre_time | ((QWORD) de->cre_date << 16);
				lfnfind->ATime  = (QWORD) de->acc_date << 16;
				lfnfind->MTime  = (QWORD) de->mod_time | ((QWORD) de->mod_date << 16);
				lfnfind->SizeHi = 0;
				lfnfind->SizeLo = de->file_size;
				/* TODO: Should convert to the code page used by the DPMI driver */
				if (lud->lfn_length)
					res = wchar_string_to_utf8(lfnfind->LongName, sizeof(lfnfind->LongName), lud->lfn, lud->lfn_length);
				else
					res = wchar_string_to_utf8(lfnfind->LongName, sizeof(lfnfind->LongName), lud->sfn, lud->sfn_length);
				if (res < 0) return res;
				/* NOTE: the Windows documentation reports that the returned
				 * alternate name is empty if only the short name is present. */
				res = wchar_string_to_utf8(lfnfind->ShortName, sizeof(lfnfind->ShortName), lud->sfn, lud->sfn_length);
				if (res < 0) return res;
				return 0;
			}
	}
	return res;
}
