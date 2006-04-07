/* The FreeDOS-32 ISO 9660 Driver version 0.2
 * Copyright (C) 2005-2006  Salvatore ISAJA
 *
 * This file "dos.c" is part of the FreeDOS-32 ISO 9660 Driver (the Program).
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
#include <filesys.h>

/* Format of the private fields of the DOS find data block */
typedef struct
{
	uint8_t  search_drive;        /* A=1 etc., remote if bit 7 set */
	char     search_template[11]; /* Name to find in FCB format, '?' allowed */
	uint8_t  search_attr;         /* Search attributes */
	uint16_t curr_secofs;         /* Byte position in the current sector */
	uint32_t curr_sector;         /* Current sector to search into */
	uint16_t sectors_left;        /* Count of sectors to search */
}
__attribute__ ((packed)) PrivateFindData;


#if ISO9660_CONFIG_FD32
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


/* Converts an ISO 9660 time stamp to a 16-bit DOS date */
static unsigned pack_dos_date(const struct iso9660_dir_record_timestamp *t)
{
	if ((t->year  >= 80) && (t->year  <= 207)
	 && (t->month >= 1)  && (t->month <= 12)
	 && (t->day   >= 1)  && (t->day   <= 31))
		return ((unsigned) t->day) | ((unsigned) t->month << 5) | (((unsigned) t->year - 80) << 9);
	return 0;
}


/* Converts an ISO 9660 time stamp to a 16-bit DOS time */
static unsigned pack_dos_time(const struct iso9660_dir_record_timestamp *t)
{
	if ((t->seconds <= 60) && (t->minutes <= 59) && (t->hour <= 23))
		return ((unsigned) t->seconds >> 1) | ((unsigned) t->minutes << 5) | ((unsigned) t->hour << 11);
	return 0;
}


/* In real life, it is actually common to find file identifiers including
 * non-d-characters. Let's consider as valid all characters allowed for
 * DOS file names.
 */
#if 0 
static bool is_dcharacter(int c)
{
	if (((c >= 'A') && (c <= 'Z')) || ((c >= '0') && (c <= '9')) || (c == '_')) return true;
	return false;
}
#else
static const char *invalid_characters = "\"+,./:;<=>[\\]|*?";
#endif


/* Returns the length in bytes of a string without trailing spaces */
static size_t rtrim(const char *s, size_t n)
{
	size_t len, k;
	for (k = 0, len = 0; k < n; k++) if (s[k] != ' ') len = k + 1;
	return len;
}


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
static int build_fcb_name_part(char *dest, size_t dest_size, const char *src, size_t src_size, bool wildcards)
{
	int res = 0;
	char c;
	while (*src && src_size)
	{
		c = *src++;
		src_size--;
		if (wildcards && (c == '*'))
		{
			memset(dest, '?', dest_size);
			break;
		}
		if (c < 0x20) return -EINVAL;
		#if 0 //working around illegal characters that appear to be used anyway...
		if (!(wildcards && (c == '?')))
		{
			const char *i;
			for (i = invalid_characters; *i; i++)
				if (c == *i) return -EINVAL;
		}
		#endif
		if (!dest_size) break;
		*dest++ = toupper(c);
		dest_size--;
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
int iso9660_build_fcb_name(char *dest, const char *src, size_t src_size, bool wildcards)
{
	int res1 = 0, res2 = 0;
	const char *sep1, *sep2;

	memset(dest, ' ', 11);
	/* If source is "." or ".." build ".          " or "..         " */
	if ((src_size == 1) && ((*src == '.') || (*src == '\0'))) *dest = '.';
	else if (((src_size == 2) && (*((uint16_t *) src) == 0x2E2E)) || ((src_size == 1) && (*src == '\1'))) *((uint16_t *) dest) = 0x2E2E;
	else /* not "." nor ".." */
	{
		sep1 = memrchr(src, '.', src_size);
		sep2 = memrchr(src, ';', src_size);
		#if 0 //seems that may actually happen
		if (sep2 < sep1) return -EINVAL;
		#endif
		if (sep2) src_size = sep2 - src;
		if (sep1)
		{
			res1 = build_fcb_name_part(dest, 8, src, sep1 - src, wildcards);
			if (res1 < 0) return res1;
			src_size -= sep1 - src + 1;
			src = sep1 + 1;
			res2 = build_fcb_name_part(dest + 8, 3, src, src_size, wildcards);
			if (res2 < 0) return res2;
		}
		else
		{
			res1 = build_fcb_name_part(dest, 8, src, src_size, wildcards);
			if (res1 < 0) return res1;
		}
		if (*dest == ' ') return -EINVAL;
	}
	return res1 || res2;
}


static int expand_fcb_name(char *dest, const char *src)
{
	unsigned len;
	len = rtrim(src, 8);
	memcpy(dest, src, len);
	src += 8;
	dest += len;
	len = rtrim(src, 3);
	if (len)
	{
		*dest++ = '.';
		memcpy(dest, src, len);
		dest += len;
	}
	*dest = '\0';
	return 0;
}


/* Compares two file names in FCB format. "sw" may contain '?' wildcards.
 * Returns: zero=match, nonzero=not match.
 */
int iso9660_compare_fcb_names(const char *s, const char *sw)
{
	unsigned k;
	for (k = 0; k < 11; k++)
		if ((sw[k] != '?') && (s[k] != sw[k])) return 1;
	return 0;
}


#if 0
/* Compare the pattern "p" against the string "s" according to
 * DOS file name matching conventions.
 */
int iso9660_dos_fnmatch(const char *p, size_t psize, const char *s, size_t ssize)
{
	unsigned np, ns;
	const char *psep1, *ssep1, *ssep2;
	/* Check for "." and ".." */
	if (ssize == 1)
	{
		if (*s == '\0')
		{
			if ((psize == 1) && (*p == '.')) return 0;
				else return 1;
		}
		if (*s == '\1')
		{
			if ((psize == 2) && !memcmp(p, "..", 2)) return 0;
				else return 1;
		}
	}
	/* Not "." or ".." */
	ssep1 = memchr(s, '.', ssize);
	ssep2 = memchr(s, ';', ssize);
	psep1 = memchr(p, '.', psize);
	if (ssep2) ssize = ssep2 - s;
	/* Compare the name */
	np = psize;
	if (psep1) np = psep1 - p;
	if (np > 8) np = 8;
	ns = ssize;
	if (ssep1) ns = ssep1 - s;
	if (ns > 8) ns = 8;
	if (np != ns) return 1;
	if (strncasecmp(p, s, np)) return 1;
	/* Compare the extension */
	p += np + 1;
	np = psize - np - 1;
	s += ns + 1;
	ns = ssize - ns - 1;
	if (np != ns) return 1;
	if (strncasecmp(p, s, np)) return 1;
	/* If we arrive here, the two names match according to DOS */
	return 0;
}
#endif


/* Backend for the DOS-style "findfirst" and "findnext" system calls.
 * Search a directory using raw sector access, without opening the directory.
 * On success, returns 0 and fills the output fields of the find data buffer.
 */
int iso9660_findnext(Volume *v, fd32_fs_dosfind_t *df)
{
	PrivateFindData *pfd = (PrivateFindData *) df;
	struct iso9660_dir_record *dr;
	Buffer *b;
	int     res;
	char    fcbname[11];
	uint8_t unallowed_attr = 0;

	if (!(pfd->search_attr & FD32_AHIDDEN)) unallowed_attr |= ISO9660_FL_EX;
	if (!(pfd->search_attr & FD32_ADIR))    unallowed_attr |= ISO9660_FL_DIR;
	for (;;)
	{
		if (!pfd->sectors_left) return -ENMFILE;
		res = iso9660_readbuf(v, pfd->curr_sector << v->log_blocks_per_sector, &b);
		if (res < 0) return res;
		dr = (struct iso9660_dir_record *) (b->data + res + pfd->curr_secofs);
		if (dr->len_dr)
		{
			#if 1
			char n[100];
			unsigned k;
			for (k = 0; k < dr->len_fi; k++) n[k] = dr->file_id[k];
			n[k] = 0;
			LOG_PRINTF(("[ISO 9660] dos_find: %3u %37s %12u %12u %3u %3u %3u\n",
				dr->len_dr, n, ME(dr->extent), ME(dr->data_length),
				dr->flags, dr->file_unit_size, dr->interleave_gap_size));
			#endif
			pfd->curr_secofs += dr->len_dr;
			if (!(dr->flags & unallowed_attr))
			{
				res = iso9660_build_fcb_name(fcbname, dr->file_id, dr->len_fi, false);
				if (res < 0) return res;
				if (iso9660_compare_fcb_names(fcbname, pfd->search_template) == 0)
				{
					df->Attr  = FD32_ARDONLY;
					if (dr->flags & ISO9660_FL_DIR) df->Attr |= FD32_ADIR;
					if (dr->flags & ISO9660_FL_EX)  df->Attr |= FD32_AHIDDEN;
					df->MTime = pack_dos_time(&dr->recording_time);
					df->MDate = pack_dos_date(&dr->recording_time);
					df->Size  = ME(dr->data_length);
					res = expand_fcb_name(df->Name, fcbname);
					if (res < 0) return res;
					return 0;
				}
			}
		}
		if ((pfd->curr_secofs >= v->bytes_per_sector) || !dr->len_dr)
		{
			pfd->curr_secofs = 0;
			pfd->curr_sector++;
			pfd->sectors_left--;
		}
	}
}


int iso9660_findfirst(Dentry *dparent, const char *fn, size_t fnsize, int attr, fd32_fs_dosfind_t *df)
{
	PrivateFindData *pfd = (PrivateFindData *) df;
	Volume *v = dparent->v;
	File *f;
	int res;
	if (attr == FD32_AVOLID)
	{
		df->Attr  = FD32_AVOLID;
		df->MTime = pack_dos_time(&v->root_recording_time);
		df->MDate = pack_dos_date(&v->root_recording_time);
		df->Size  = 0;
		memcpy(df->Name, v->vol_id, sizeof(df->Name));
		df->Name[12] = '\0';
		return 0;
	}
	res = iso9660_build_fcb_name(pfd->search_template, fn, fnsize, true);
	if (res < 0) return res;
	pfd->search_attr = attr;
	res = iso9660_open(dparent, O_RDONLY | O_DIRECTORY, &f);
	if (res < 0) return res;
	pfd->curr_secofs  = 0;
	pfd->curr_sector  = f->extent >> v->log_blocks_per_sector;
	pfd->sectors_left = (f->data_length + v->bytes_per_sector - 1) >> v->log_bytes_per_sector;
	iso9660_close(f);
	res = iso9660_findnext(v, df);
	if (res == -ENMFILE) res = -ENOENT; /* Yet another convention */
	return res;
}


/* Checks if "s1" (which may contain '?' wildcards) matches the beginning
 * of "s2". Both strings are not null terminated.
 * Returns the number of matched wide characters if the string match, zero
 * if the strings don't match, or a negative error.
 */
static int compare_pattern(const char *s1, size_t n1, const char *s2, size_t n2)
{
	int res = 0;
	//wchar_t wc;
	//int skip;
	while (n1 && n2)
	{
		//skip = unicode_utf8towc(&wc, s1, n1);
		//if (skip < 0) return skip;
		//if ((wc != '?') && (unicode_simple_fold(wc) != unicode_simple_fold(*s2))) return 0;
		//n1 -= skip;
		//s1 += skip;
		if ((*s1 != '?') && (toupper(*s1) != toupper(*s2))) return 0;
		n1--;
		s1++;
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
static int compare_with_wildcards(const char *s1, size_t n1, const char *s2, size_t n2)
{
	int res = 0; /* avoid warnings */
	const char *asterisk;
	size_t nsub;
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
		asterisk = memchr(s1, '*', n1);
		nsub = n1;
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


/* Compare "s1" (which may contain wildcards) against "s2".
 * The wildcard matching is DOS-compatible: if "s1" contains any dots,
 * the last dot is used as a conventional extension separator, that needs
 * not to be present in "s2". The name and extension parts are then compared
 * separately, so that "*.*" matches any file like "*" (even if not containing
 * a dot), and "*." matches files with no extension (even if not ending with a
 * dot). Both strings are not null terminated.
 * Return value: 0 match, >0 don't match, <0 error.
 */
static int dos_fnmatch(const char *s1, size_t s1size, const char *s2, size_t s2size, bool shorten)
{
	int res1, res2;
	size_t n1size = s1size;
	size_t e1size = 0;
	const char *e1 = memrchr(s1, '.', s1size);
	size_t n2size = s2size;
	size_t e2size = 0;
	const char *e2 = memrchr(s2, '.', s2size);
	if (e1)
	{
		n1size = e1 - s1;
		e1++;
		e1size = s1size - n1size - 1;
	}
	if (e2)
	{
		n2size = e2 - s2;
		e2++;
		e2size = s2size - n2size - 1;
	}
	if (shorten)
	{
		if (n1size > 8) n1size = 8;
		if (e1size > 3) e1size = 3;
		if (n2size > 8) n2size = 8;
		if (e2size > 3) e2size = 3;
		n2size = rtrim(s2, n2size);
		e2size = rtrim(e2, e2size);
	}
	res1 = compare_with_wildcards(s1, n1size, s2, n2size);
	if (res1 < 0) return res1;
	res2 = compare_with_wildcards(e1, e1size, e2, e2size);
	if (res2 < 0) return res2;
	return res1 || res2;
}


/* Backend for the "FindFirst" and "FindNext" Long File Names DOS system calls. */
int iso9660_findfile(File *f, const char *fn, size_t fnsize, int flags, fd32_fs_lfnfind_t *lfnfind)
{
	struct iso9660_dir_record *dr;
	const char *sep2;
	Buffer  *b;
	int      res;
	unsigned len;
	uint8_t  unallowed_attr = 0;
	uint8_t  required_attr = 0;

	if (!(flags & FD32_FAHIDDEN)) unallowed_attr |= ISO9660_FL_EX;
	if (!(flags & FD32_FADIR))    unallowed_attr |= ISO9660_FL_DIR;
	if (flags & FD32_FRHIDDEN)    required_attr  |= ISO9660_FL_EX;
	if (flags & FD32_FRDIR)       required_attr  |= ISO9660_FL_DIR;
	for (;;)
	{
		res = iso9660_do_readdir(f, &b, NULL, &len);
		if (res < 0) return res;
		dr = (struct iso9660_dir_record *) (b->data + res + len);
		if (dr->flags & unallowed_attr) continue;
		if ((dr->flags & required_attr) != required_attr) continue;
		len = dr->len_fi;
		#if ISO9660_CONFIG_STRIPVERSION
		sep2 = memrchr(dr->file_id, ';', dr->len_fi);
		if (sep2) len = sep2 - dr->file_id;
		#endif
		if (!dos_fnmatch(fn, fnsize, dr->file_id, len, false)
		 || !dos_fnmatch(fn, fnsize, dr->file_id, len, true))
		{
			lfnfind->Attr  = FD32_ARDONLY;
			if (dr->flags & ISO9660_FL_DIR) lfnfind->Attr |= FD32_ADIR;
			if (dr->flags & ISO9660_FL_EX)  lfnfind->Attr |= FD32_AHIDDEN;
			lfnfind->CTime  = (QWORD) pack_dos_time(&dr->recording_time) | ((QWORD) pack_dos_date(&dr->recording_time) << 16);
			lfnfind->ATime  = 0;
			lfnfind->MTime  = (QWORD) pack_dos_time(&dr->recording_time) | ((QWORD) pack_dos_date(&dr->recording_time) << 16);
			lfnfind->SizeHi = 0;
			lfnfind->SizeLo = ME(dr->data_length);
			if (len >= sizeof(lfnfind->LongName)) continue;//return -ENAMETOOLONG;
			memcpy(lfnfind->LongName, dr->file_id, len);
			lfnfind->LongName[len] = '\0';
			/* Generate an 8.3 name for lfnfind->ShortName */
			{
				size_t nsize = len;
				size_t esize = 0;
				const char *e = memrchr(dr->file_id, '.', len);
				if (e)
				{
					nsize = e - dr->file_id;
					e++;
					esize = len - nsize - 1;
				}
				if (nsize > 8) nsize = 8;
				if (esize > 3) esize = 3;
				nsize = rtrim(dr->file_id, nsize);
				memcpy(lfnfind->ShortName, dr->file_id, nsize);
				if (esize)
				{
					lfnfind->ShortName[nsize] = '.';
					esize = rtrim(e, esize);
					memcpy(lfnfind->ShortName + nsize + 1, e, esize);
					lfnfind->ShortName[nsize + 1 + esize] = '\0';
				}
				else lfnfind->ShortName[nsize] = '\0';
			}
			return 0;
		}
	}
}


#if ISO9660_CONFIG_FD32
int iso9660_get_attr(File *f, fd32_fs_attr_t *a)
{
	if (f->magic != ISO9660_FILE_MAGIC) return -EBADF;
	if (a->Size < sizeof(fd32_fs_attr_t)) return -EINVAL;
	a->Attr  = FD32_ARDONLY;
	if (f->file_flags & ISO9660_FL_EX) a->Attr |= FD32_AHIDDEN;
	if (f->file_flags & ISO9660_FL_DIR) a->Attr |= FD32_ADIR;
	a->MDate = pack_dos_date(&f->timestamp);
	a->MTime = pack_dos_time(&f->timestamp);
	a->ADate = 0;
	a->CDate = a->MDate;
	a->CTime = a->MTime;
	a->CHund = 0;
	return 0;
}


/* Gets allocation informations on a FAT volume. */
int iso9660_get_fsfree(fd32_getfsfree_t *fsfree)
{
	Volume *v;
	int res;
	if (fsfree->Size < sizeof(fd32_getfsfree_t)) return -EINVAL;
	v = (Volume *) fsfree->DeviceId;
	if (v->magic != ISO9660_VOL_MAGIC) return -ENODEV;
	res = iso9660_blockdev_test_unit_ready(v);
	if (res < 0) return res;
	fsfree->SecPerClus  = 1;
	fsfree->BytesPerSec = v->bytes_per_sector;
	fsfree->AvailClus   = 0;
	fsfree->TotalClus   = v->vol_space;
	return 0;
}
#endif
