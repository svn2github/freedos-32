/* INT 21h handler for the FreeDOS-32 DPMI Driver
 * Copyright (C) 2001-2005  Salvatore ISAJA
 *
 * This file "int21.c" is part of the FreeDOS-32 DPMI Driver (the Program).
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

/* TODO: We should provide some mechanism for installing new fake
         interrupt handlers (like this INT 21h handler), with the   
         possibility of recalling the old one.
*/

#include <ll/i386/hw-data.h>
#include <ll/i386/string.h>
#include <kernel.h>
#include <filesys.h>
#include <errno.h>
#include <logger.h>
#include <fd32time.h>
#include "rmint.h"
#include "dos_exec.h"

#define DOS_API_INPUT_IS_UTF8 1
#if !DOS_API_INPUT_IS_UTF8
 #include <unicode/unicode.h>
 #include <nls/nls.h>
 struct nls_operations *current_cp;
#endif

#if defined(__GNUC__) && !defined(restrict)
 #define restrict __restrict__
#endif

/* Define the __DEBUG__ symbol in order to activate log output */
/* #define __DEBUG__ */
#ifdef __DEBUG__
 #define LOG_PRINTF(s) fd32_log_printf s
#else
 #define LOG_PRINTF(s)
#endif

#define FAR2ADDR(seg, off) (((DWORD) seg << 4) + (DWORD) off)


/* The current PSP is needed for the pointer to the DTA */
extern struct psp *current_psp;

int use_lfn;


/* Parameter block for the dos_exec call, see INT 21h AH=4Bh */
typedef struct
{
  WORD  Env;
  WORD arg_offs;
  WORD arg_seg;
  DWORD Fcb1;
  DWORD Fcb2;
  DWORD Res1;
  DWORD Res2;
}
__attribute__ ((packed)) ExecParams;
/* DOS return code of the last executed program */
static WORD dos_return_code;


/* Data structure for INT 21 AX=7303h                   */
/* Windows95 - FAT32 - Get extended free space on drive */
typedef struct
{
  WORD  Size;            /* Size of the returned structure (this one)  */
  WORD  Version;         /* Version of this structure (desired/actual) */
  /* The following are *with* adjustment for compression */
  DWORD SecPerClus;      /* Sectors per cluster               */
  DWORD BytesPerSec;     /* Bytes per sector                  */
  DWORD AvailClus;       /* Number of available clusters      */
  DWORD TotalClus;       /* Total number of clusters on drive */
  /* The following are *without* adjustment for compression */
  DWORD RealSecPerClus;  /* Number of sectors per cluster     */
  DWORD RealBytesPerSec; /* Bytes per sector                  */
  DWORD RealAvailClus;   /* Number of available clusters      */
  DWORD RealTotalClus;   /* Total number of clusters on drive */
  BYTE  Reserved[8];
}
__attribute__ ((packed)) ExtDiskFree;


/* DOS error codes (see RBIL table 01680) */
#define DOS_ENOTSUP   0x01 /* function number invalid */
#define DOS_ENOENT    0x02 /* file not found */
#define DOS_ENOTDIR   0x03 /* path not found */
#define DOS_EMFILE    0x04 /* too many open files */
#define DOS_EACCES    0x05 /* access denied */
#define DOS_EBADF     0x06 /* invalid handle */
#define DOS_ENOMEM    0x08 /* insufficient memory */
#define DOS_EMEMBLK   0x09 /* memory block address invalid */
#define DOS_EFTYPE    0x0B /* format invalid */
#define DOS_EDATA     0x0D /* data invalid */
#define DOS_ENOTBLK   0x0F /* invalid drive */
#define DOS_EXDEV     0x11 /* not same device */
#define DOS_ENMFILE   0x12 /* no more files */
#define DOS_EROFS     0x13 /* disk write-protected */
#define DOS_EUNIT     0x14 /* unknown unit */
#define DOS_ENOMEDIUM 0x15 /* drive not ready */
#define DOS_EIO       0x1F /* general failure */
#define DOS_ENOSPC    0x27 /* insufficient disk space */
#define DOS_EEXIST    0x50 /* file exists */
#define DOS_EUNKNOWN  0x64 /* unknown error */
#define DOS_EBUFSMALL 0x7A /* buffer too small to hold return data */
#define DOS_EINVAL    0xA0 /* bad arguments */
#define DOS_ENOLCK    0xB4 /* lock count has been exceeded */
#define DOS_ENOEXEC   0xC1 /* bad EXE format */

/* Lookup table to convert FD32 errno codes to DOS error codes */
/* TODO: Fine tune the mapping using the DJGPP one as a reference */
static const BYTE errno2dos[] = {
/*   0 */ 0x00,         DOS_EUNKNOWN, DOS_ENOENT,   DOS_EUNKNOWN,
/*   4 */ DOS_EUNKNOWN, DOS_EIO,      DOS_EUNIT,    DOS_EUNKNOWN,
/*   8 */ DOS_ENOEXEC,  DOS_EBADF,    DOS_EUNKNOWN, DOS_EUNKNOWN,
/*  12 */ DOS_ENOMEM,   DOS_EACCES,   DOS_EMEMBLK,  DOS_EUNIT,
/*  16 */ DOS_EUNKNOWN, DOS_EEXIST,   DOS_EXDEV,    DOS_EUNIT,
/*  20 */ DOS_ENOTDIR,  DOS_EACCES,   DOS_EINVAL,   DOS_EMFILE,
/*  24 */ DOS_EMFILE,   DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EACCES,
/*  28 */ DOS_ENOSPC,   DOS_EUNKNOWN, DOS_EROFS,    DOS_EUNKNOWN,
/*  32 */ DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN,
/*  36 */ DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN,
/*  40 */ DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN,
/*  44 */ DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_ENOLCK,   DOS_EUNKNOWN,
/*  48 */ DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN,
/*  52 */ DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_ENOTSUP,  DOS_EUNKNOWN,
/*  56 */ DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN,
/*  60 */ DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN,
/*  64 */ DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN,
/*  68 */ DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN,
/*  72 */ DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN,
/*  76 */ DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EFTYPE,
/*  80 */ DOS_EUNKNOWN, DOS_EBADF,    DOS_EUNKNOWN, DOS_EUNKNOWN,
/*  84 */ DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN,
/*  88 */ DOS_ENOTSUP,  DOS_ENMFILE,  DOS_EACCES,   DOS_EBUFSMALL,
/*  92 */ DOS_EACCES,   DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_ENOTSUP,
/*  96 */ DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN,
/* 100 */ DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN,
/* 104 */ DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN,
/* 108 */ DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN,
/* 112 */ DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN,
/* 116 */ DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN,
/* 120 */ DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN,
/* 124 */ DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN,
/* 128 */ DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EUNKNOWN,
/* 132 */ DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_ENOTSUP,  DOS_ENOMEDIUM,
/* 136 */ DOS_EUNKNOWN, DOS_EUNKNOWN, DOS_EDATA
};


/* Converts a FD32 return code (< 0 meaning error)
 * to a DOS return status in the specified register set.
 * - on error: carry flag set, error (positive) in AX
 * - on success: carry flag clear
 */
static void res2dos(int res, union rmregs *r)
{
	//LOG_PRINTF(("INT 21h - res=%i\n", res));
	RMREGS_CLEAR_CARRY;
	if (res < 0)
	{
		res = -res;
		RMREGS_SET_CARRY;
		r->x.ax = DOS_EUNKNOWN;
		if (res < sizeof(errno2dos)) r->x.ax = errno2dos[res];
	}
}


/* Converts a DOS open action (see RBIL table 01770) to a libc mode. */
static int parse_open_action(int dos_action)
{
	int action = 0;
	if (dos_action & 0x10) action |= O_CREAT;
	switch (dos_action & ~0x10)
	{
		case 0x00: action |= O_EXCL; break;
		case 0x01: break;
		case 0x02: action |= O_TRUNC; break;
		default: action = -EINVAL;
	}
	return action;
}


/* Helper for the DOS-style "open" system call.
 * Forbids opening directories even for reading only.
 */
static int dos_open(const char *fn, int flags, int attr, int hint, int *action)
{
	fd32_fs_attr_t a;
	int res;
	int fd = fd32_open((char *) fn, flags, attr, hint, action);
	if (fd < 0) return fd;
	res = fd32_get_attributes(fd, &a);
	if (res >= 0)
	{
		if (!(a.Attr & FD32_ADIR)) return fd;
		res = -EACCES;
	}
	fd32_close(fd);
	return res;
}


/* Converts a path name from DOS API to UTF-8 using no more than "size" bytes.
 * Forward slashes '/' are converted to back slashes '\'.
 * Returns 0 on success, or a negative error.
 */
static int fix_path(char *restrict dest, const char *restrict source, size_t size)
{
	#if DOS_API_INPUT_IS_UTF8
	char c;
	while (*source)
	{
		if (!size) return -ENAMETOOLONG;
		c = *(source++);
		if (c == '/') c = '\\';
		*(dest++) = c;
	}
	#else
	int res = 0;
	wchar_t wc;
	while (*source)
	{
		res = current_cp->mbtowc(&wc, source, NLS_MB_MAX_LEN);
		if (res < 0) return res;
		source += res;
		if (wc == (wchar_t) '/') wc = (wchar_t) '\\';
		res = unicode_wctoutf8(dest, wc, size);
		if (res < 0) return res;
		dest += res;
		size -= res;
		
	}
	#endif
	if (!size) return -ENAMETOOLONG;
	*dest = 0;
	return 0;
}


/* Sub-handler for DOS services "Get/set file's time stamps"
 * INT 21h AH=57h (BX=file handle, AL=action).
 */
static inline void get_and_set_time_stamps(union rmregs *r)
{
	fd32_fs_attr_t a;
	int res;
	LOG_PRINTF(("[DPMI] INT 21h: get_and_set_time_stamps, al=%02xh\n", r->h.al));
	a.Size = sizeof(fd32_fs_attr_t);
	switch (r->h.al)
	{
		/* Get last modification date and time */
		case 0x00:
			/* TODO: MS-DOS returns date and time of opening for char devices */
			res = fd32_get_attributes(r->x.bx, &a);
			if (res >= 0)
			{
				r->x.cx = a.MTime;
				r->x.dx = a.MDate;
			}
			break;
		/* Set last modification date and time */
		case 0x01:
			res = fd32_get_attributes(r->x.bx, &a);
			if (res >= 0)
			{
				a.MTime = r->x.cx;
				a.MDate = r->x.dx;
				res = fd32_set_attributes(r->x.bx, &a);
			}
			break;
		/* Get last access date and time */
		case 0x04:
			res = fd32_get_attributes(r->x.bx, &a);
			if (res >= 0)
			{
				r->x.dx = a.ADate;
				r->x.cx = 0x0000; /* FAT has not the last access time */
			}
			break;
		/* Set last access date and time */
		case 0x05:
			if (r->x.cx != 0x0000)
			{
				/* FAT has not the last access time */
				res = -EINVAL;
				break;
			}
			res = fd32_get_attributes(r->x.bx, &a);
			if (res >= 0)
			{
				a.ADate = r->x.dx;
				res = fd32_set_attributes(r->x.bx, &a);
			}
			break;
		/* Get creation date and time */
		case 0x06:
			res = fd32_get_attributes(r->x.bx, &a);
			if (res >= 0)
			{
				r->x.dx = a.CDate;
				r->x.cx = a.CTime;
				r->x.si = a.CHund;
			}
			break;
		/* Set creation date and time */
		case 0x07:
			res = fd32_get_attributes(r->x.bx, &a);
			if (res >= 0)
			{
				a.CDate = r->x.dx;
				a.CTime = r->x.cx;
				a.CHund = r->x.si;
				res = fd32_set_attributes(r->x.bx, &a);
			}
			break;
		/* Invalid subfunctions */
		default:
			r->h.al = 0xFF; /* Yet another convention */
			RMREGS_SET_CARRY;
			return;
	}
	res2dos(res, r);
}


/* Sub-handler for DOS service "Get/set file's attributes"
 * INT 21h AH=43h (DS:DX->ASCIZ file name, AL=action).
 */
static inline void dos_get_and_set_attributes(union rmregs *r)
{
	char fn[FD32_LFNPMAX];
	fd32_fs_attr_t a;
	int fd;
	int res = fix_path(fn, (const char *) FAR2ADDR(r->x.ds, r->x.dx), sizeof(fn));
	LOG_PRINTF(("[DPMI] INT 21h: get_and_set_attributes, fn=%s, al=%02xh\n", fn, r->h.al));
	if (res < 0)
	{
		res2dos(res, r);
		return;
	}
	fd = fd32_open(fn, O_RDONLY, FD32_ANONE, 0, NULL);
	if (fd < 0) /* It could be a directory... */
		fd = fd32_open(fn, O_RDONLY | O_DIRECTORY, FD32_ANONE, 0, NULL);
	if (fd < 0)
	{
		res2dos(fd, r);
		return;
	}
	a.Size = sizeof(fd32_fs_attr_t);
	switch (r->h.al)
	{
		/* Get file attributes */
		case 0x00:
			res = fd32_get_attributes(fd, &a);
			if (res >= 0)
			{
				r->x.cx = a.Attr;
				r->x.ax = r->x.cx; /* DR-DOS 5.0 */
			}
			break;
		/* Set file attributes */
		case 0x01:
			/* FIXME: Should close the file if currently open in sharing-compat
			 *        mode, or generate a sharing violation critical error in
			 *        the file is currently open.
			 * Mah! Would that be true with the new things? Mah...
			 */
			res = fd32_get_attributes(fd, &a);
			if (res < 0) break;
			/* Volume label and directory attributes cannot be changed.
			 * To change the other attributes bits of a directory the directory
			 * flag must be clear, though it will be set after this call.
			 */
			if ((r->x.cx & (FD32_AVOLID | FD32_ADIR)) || (a.Attr == FD32_AVOLID))
			{
				res = -EACCES;
				break;
			}
			if (a.Attr & FD32_ADIR) a.Attr = r->x.cx | FD32_ADIR;
			                   else a.Attr = r->x.cx;
			res = fd32_set_attributes(fd, &a);
			break;
		/* Invalid subfunctions */
		default:
			fd32_close(fd);
			r->h.al = 0xFF; /* Yet another convention */
			RMREGS_SET_CARRY;
			return;
	}
	fd32_close(fd);
	res2dos(res, r);
}


/* Sub-handler for LFN service "Extended get/set attributes"
 * INT 21h AX=7143h (DS:DX->ASCIZ file name, BL=action).
 */
static inline void lfn_get_and_set_attributes(union rmregs *r)
{
	char fn[FD32_LFNPMAX];
	fd32_fs_attr_t a;
	int fd;
	int res = fix_path(fn, (const char *) FAR2ADDR(r->x.ds, r->x.dx), sizeof(fn));
	LOG_PRINTF(("[DPMI] INT 21h: lfn_get_and_set_attributes, fn=%s, bl=%02xh\n", fn, r->h.bl));
	if (res < 0)
	{
		res2dos(res, r);
		return;
	}
	fd = fd32_open(fn, O_RDWR, FD32_ANONE, 0, NULL);
	if (fd < 0) /* It could be a directory... */
		fd = fd32_open(fn, O_RDWR | O_DIRECTORY, FD32_ANONE, 0, NULL);
	if (fd < 0)
	{
		res2dos(res, r);
		return;
	}
	a.Size = sizeof(fd32_fs_attr_t);
	switch (r->h.bl)
	{
		/* Get file attributes */
		case 0x00:
			res = fd32_get_attributes(fd, &a);
			if (res >= 0) r->x.cx = a.Attr;
			break;
		/* Set file attributes */
		case 0x01:
			res = fd32_get_attributes(fd, &a);
			if (res >= 0)
			{
				a.Attr = r->x.cx;
				res = fd32_set_attributes(fd, &a);
			}
			break;
		/* Get physical size of compressed file */
		/* case 0x02: Not implemented */

		/* Set last modification date and time */
		case 0x03:
			res = fd32_get_attributes(fd, &a);
			if (res >= 0)
			{
				a.MDate = r->x.di;
				a.MTime = r->x.cx;
				res = fd32_set_attributes(fd, &a);
			}
			break;
		/* Get last modification date and time */
		case 0x04:
			res = fd32_get_attributes(fd, &a);
			if (res >= 0)
			{
				r->x.di = a.MDate;
				r->x.cx = a.MTime;
			}
			break;
		/* Set last access date */
		case 0x05:
			/* DI new date in DOS format */
			res = fd32_get_attributes(fd, &a);
			if (res >= 0)
			{
				a.ADate = r->x.di;
				res = fd32_set_attributes(fd, &a);
			}
			break;
		/* Get last access date */
		case 0x06:
			res = fd32_get_attributes(fd, &a);
			if (res >= 0) r->x.di = a.ADate;
			break;
		/* Set creation date and time */
		case 0x07:
			res = fd32_get_attributes(fd, &a);
			if (res >= 0)
			{
				a.CDate = r->x.di;
				a.CTime = r->x.cx;
				a.CHund = r->x.si;
				res = fd32_set_attributes(fd, &a);
			}
			break;
		/* Get creation date and time */
		case 0x08:
			res = fd32_get_attributes(fd, &a);
			if (res >= 0)
			{
				r->x.di = a.CDate;
				r->x.cx = a.CTime;
				r->x.si = a.CHund;
			}
			break;
		/* Unsupported or invalid actions */
		default:
			fd32_close(fd);
			r->x.ax = 0x7100; /* Yet another convention */
			RMREGS_SET_CARRY;
			return;
	}
	fd32_close(fd);
	res2dos(res, r);
}


/* Sub-handler for Long File Names services
 * INT 21h AH=71h (AL=function)
 */
static inline void lfn_functions(union rmregs *r)
{
	char fn[FD32_LFNPMAX];
	int res;
	LOG_PRINTF(("[DPMI] INT 21h: lfn_functions, al=%02xh\n", r->h.al));
	switch (r->h.al)
	{
		/* Make directory (DS:DX->ASCIZ directory name) */
		case 0x39:
			res = fix_path(fn, (const char *) FAR2ADDR(r->x.ds, r->x.dx), sizeof(fn));
			if (res >= 0) res = fd32_mkdir(fn);
			break;
		/* Remove directory (DS:DX->ASCIZ directory name) */
		case 0x3A:
			res = fix_path(fn, (const char *) FAR2ADDR(r->x.ds, r->x.dx), sizeof(fn));
			if (res >= 0) res = fd32_rmdir(fn);
			break;
		/* Change directory (DS:DX->ASCIZ directory name) */
		case 0x3B:
			res = fix_path(fn, (const char *) FAR2ADDR(r->x.ds, r->x.dx), sizeof(fn));
			if (res >= 0) res = fd32_chdir(fn);
			break;
		/* Delete file (DS:DX->ASCIZ file name, CL=search attributes,
		 * CL=must-match attributes, SI=use wildcards and attributes [0=no,1=yes]).
		 * FIXME: Parameter in SI not used.
		 */
		case 0x41:
			res = fix_path(fn, (const char *) FAR2ADDR(r->x.ds, r->x.dx), sizeof(fn));
			if (res >= 0) res = fd32_unlink(fn, ((DWORD) r->h.ch << 8) + (DWORD) r->h.cl);
			break;
		/* Extended get/set file attributes */
		case 0x43:
			lfn_get_and_set_attributes(r);
			return;
		/* Get current directory (DL=drive number [0=default, 1=A, etc.],
		 * DS:SI->260-bytes buffer for ASCIZ pathname).
		 * Same as INT 21h AH=47h, but we don't convert to 8.3 the resulting path
		 */
		case 0x47:
		{
			char  drive[2] = "A";
			char *dest = (char *) FAR2ADDR(r->x.ds, r->x.si);
			drive[0] = fd32_get_default_drive();
			if (r->h.dl) drive[0] = r->h.dl + 'A' - 1;
			res = fd32_getcwd(drive, fn);
			if (res >= 0)
			{
				strcpy(dest, fn + 1); /* Skip leading backslash */
				/* FIXME: Convert from UTF-8 to OEM */
			}
			break;
		}
		/* Find first matching file (DS:DX->ASCIZ file spec, ES:DI->find data record,
		 * CL=allowable attributes, CH=required attributes, SI=date/time format
		 * [0=Windows time, 1=DOS time]).
		 * FIXME: Parameter in SI not used.
		 */
		case 0x4E:
			res = fix_path(fn, (const char *) FAR2ADDR(r->x.ds, r->x.dx), sizeof(fn));
			if (res < 0) break;
			res = fd32_lfn_findfirst(fn, ((DWORD) r->h.ch << 8) + (DWORD) r->h.cl,
			                         (fd32_fs_lfnfind_t *) FAR2ADDR(r->x.es, r->x.di));
			res2dos(res, r);
			if (res >= 0)
			{
				r->x.ax = (WORD) res; /* The directory handle to continue the search */
				/* FIXME: Unicode conversion flags in CX not implemented
				 * bit 0 set if the returned long name contains
				 *  underscores for unconvertable Unicode characters
				 * bit 1 set if the returned short name contains
				 *  underscores for unconvertable Unicode characters
				 */
				r->x.cx = 0; 
			}
			return;
		/* Find next matching file (ES:DI->find data record, BX=directory handle,
		 * SI=date/time format [0=Windows time, 1=DOS time]).
		 * FIXME: Same FIXMEs as case 0x4E.
		 */
		case 0x4F:
			res = fd32_lfn_findnext(r->x.bx, 0, (fd32_fs_lfnfind_t *) FAR2ADDR(r->x.es, r->x.di));
			res2dos(res, r);
			if (res >= 0) r->x.cx = 0;
			return;
		/* Rename of move file (DS:DX->ASCIZ old name, ES:DI->ASCIZ new name) */
		case 0x56:
		{
			char newfn[FD32_LFNPMAX];
			res = fix_path(fn, (const char *) FAR2ADDR(r->x.ds, r->x.dx), sizeof(fn));
			if (res < 0) break;
			res = fix_path(newfn, (const char *) FAR2ADDR(r->x.es, r->x.di), sizeof(newfn));
			if (res >= 0) res = fd32_rename(fn, newfn);
			break;
		}
		/* Truename (CL=subfunction) */
		case 0x60:
			/* FIXME: Only subservice 01h "Get short (8.3) filename for file" is implemented */
			if (r->h.cl != 0x01)
			{
				res = -ENOTSUP;
				break;
			}
			/* CH=SUBST expansion flag (ignored), 80h to not resolve SUBST
			 * DS:SI->ASCIZ long file name or path
			 * ES:DI->64-bytes buffer for short path name
			 */
			res = fix_path(fn, (const char *) FAR2ADDR(r->x.ds, r->x.si), sizeof(fn));
			if (res >= 0) res = fd32_sfn_truename((char *) FAR2ADDR(r->x.es, r->x.di), fn);
			break;
		/* Create or open file (BX=access mode and sharing flags, CX=attributes
		 * for creation, DX=action, DS:SI->ASCIZ file name, DI=numeric tail hint).
		 */
		case 0x6C:
		{
			int action;
			res = parse_open_action(r->x.dx);
			if (res < 0) break;
			action = res;
			res = fix_path(fn, (const char *) FAR2ADDR(r->x.ds, r->x.si), sizeof(fn));
			if (res < 0) break;
			res = dos_open(fn, action | (DWORD) r->x.bx, r->x.cx, r->x.di, &action);
			res2dos(res, r);
			if (res >= 0)
			{
				r->x.ax = (WORD) res; /* Returned handle */
				r->x.cx = (WORD) action; /* Action taken */
			}
			return;
		}
		/* Get volume informations (DS:DX->ASCIZ root directory name,
		 * ES:DI->buffer to store the file system name, CX=size of the
		 * buffer pointed by ES:DI).
		 */
		case 0xA0:
		{
			fd32_fs_info_t fsi;
			res = fix_path(fn, (const char *) FAR2ADDR(r->x.ds, r->x.dx), sizeof(fn));
			if (res < 0) break;
			fsi.Size       = sizeof(fd32_fs_info_t);
			fsi.FSNameSize = r->x.cx;
			fsi.FSName     = (char *) FAR2ADDR(r->x.es, r->x.di);
			res = fd32_get_fsinfo(fn, &fsi);
			if (res >= 0)
			{
				r->x.bx = fsi.Flags;
				r->x.cx = fsi.NameMax;
				r->x.dx = fsi.PathMax;
			}
			break;
		}
		/* "FindClose" - Terminate directory search (BX=directory handle) */
		case 0xA1:
			res = fd32_lfn_findclose(r->x.bx);
			break;
		/* Unsupported or invalid actions */
		default:
			r->x.ax = 0x7100; /* Yet another convention */
			RMREGS_SET_CARRY;
			return;
	}
	res2dos(res, r);
}


/* INT 21h handler for DOS file system services (AH=function) */
void int21_handler(union rmregs *r)
{
	int res;
	long long int res64;
	char fn[FD32_LFNPMAX];
	LOG_PRINTF(("[DPMI] INT 21h AX=%04xh BX=%04xh CX=%04xh DX=%04xh\n", r->x.ax, r->x.bx, r->x.cx, r->x.dx));
	switch (r->h.ah)
	{
		case 0x02:
		{
			static char last;
			cputc(r->h.dl);
			r->h.al = last;
			last = r->h.dl;
			return;
		}
		/* DOS 1+ - Direct character input, without echo */
		case 0x07:
		{
			int tmp;
			LOG_PRINTF(("[DPMI] INT 21h: Direct char input, no echo\n"));
			/* Perform 1-byte read from the stdin, file handle 0.
			   1. Save the old mode of stdin
			   2. Set the stdin to the raw mode without echo
			   3. Read 1 byte from stdin and restore the old mode
			   This procedure is very alike when doing on the Win32 platform
			 */
			/* This call doesn't check for Ctrl-C or Ctrl-Break.  */
			fd32_get_attributes(0, &tmp);
			res = 0; /* Res used to set the attributes only once */
			fd32_set_attributes(0, &res);
			res = fd32_read(0, &(r->h.al), 1);
			fd32_set_attributes(0, &tmp);
			/* Return the character in AL */
			/* TODO:
			   1. Get rid of the warning: passing arg 2 of `fd32_get_attributes'
			   from incompatible pointer type, maybe using another system call?
			   2. from RBIL: if the interim console flag is set (see AX=6301h),
			   partially-formed double-byte characters may be returned
			*/
			return;
		}
		/* DOS 1+ - Set default drive (DL=drive, 0=A, etc.) */
		case 0x0E:
			LOG_PRINTF(("[DPMI] INT 21h: Set default drive to %02xh\n", r->h.dl));
			res = fd32_set_default_drive(r->h.dl + 'A');
			if (res >= 0) return; 
			{
				/* Return the number of potentially valid drive letters in AL */
				r->h.al = (BYTE) res;
				RMREGS_CLEAR_CARRY;
			}
			/* No error code is documented. Check this. */
			return;
		/* DOS 1+ - Get default drive (in AL, 0=A, etc.) */
		case 0x19:
			r->h.al = fd32_get_default_drive() - 'A';
			LOG_PRINTF(("[DPMI] INT 21h: Get default drive: %02xh\n", r->h.al));
			RMREGS_CLEAR_CARRY;
			return;
		/* DOS 1+ - Set Disk Transfer Area address (DS:DX->new DTA) */
		case 0x1A:
			LOG_PRINTF(("[DPMI] INT 21h: Set Disk Transfer Area Address: %04xh:%04xh\n", r->x.ds, r->x.dx));
			current_psp->dta = (void *) FAR2ADDR(r->x.ds, r->x.dx);
			return;
		/* DOS 1+ - GET SYSTEM DATE */
		case 0x2A:
		{
			fd32_date_t d;
			LOG_PRINTF(("[DPMI] INT 21h: Get system date\n"));
			fd32_get_date(&d);
			r->x.cx = d.Year;
			r->h.dh = d.Mon;
			r->h.dl = d.Day;
			r->h.al = d.weekday;
			RMREGS_CLEAR_CARRY;
			return;
		}
		/* DOS 1+ - GET SYSTEM TIME */
		case 0x2C:
		{
			fd32_time_t t;
			LOG_PRINTF(("[DPMI] INT 21h: Get system time\n"));
			fd32_get_time(&t);
			r->h.ch = t.Hour;
			r->h.cl = t.Min;
			r->h.dh = t.Sec;
			r->h.dl = t.Hund;
			RMREGS_CLEAR_CARRY;
			return;
		}
		/* DOS 2+ - Get Disk Transfer Area address (ES:BX <- current DTA) */
		case 0x2F:
			r->x.es = (DWORD) current_psp->dta >> 4;
			r->x.bx = (DWORD) current_psp->dta & 0xF;
			LOG_PRINTF(("[DPMI] INT 21h: Get Disk Transfer Area Address: %04xh:%04xh\n", r->x.es, r->x.bx));
			return;

		/* DOS 2+ - Get DOS version, subject to modification by SETVER
		 * (AL=what to return in BH: 0=OEM number, 1=version flag).
		 */
		case 0x30:
			LOG_PRINTF(("[DPMI] INT 21h: Get DOS version, al=%02xh\n", r->h.al));
			if (r->h.al) r->h.bh = 0x00; /* DOS is not in ROM (bit 3) */
			        else r->h.bh = 0xFD; /* We claim to be FreeDOS */
			/* Set the 24-bit User Serial Number to zero as we don't use it */
			r->h.bl = 0;
			r->x.cx = 0;
			r->x.ax = 0x0A07; /* We claim to be 7.10 */
			RMREGS_CLEAR_CARRY;
			return;
		/* DOS 2+ AL=06h - Get ture DOS version */
		case 0x33:
			LOG_PRINTF(("[DPMI] INT 21h: Get true DOS version, al=%02xh\n", r->h.al));
			if (r->h.al == 0x06)
			{
				r->x.bx = 0x0A07; /* We claim to be 7.10 */
				r->x.dx = 0; /* Revision 0, no flags... */
				RMREGS_CLEAR_CARRY;
				return;
			}
			r->h.al = 0xFF; /* Invalid subfunction */
			RMREGS_SET_CARRY;
			return;
		/* DOS 2+ - Get free disk space (DL=drive number, 0=default, 1=A, etc.) */
		case 0x36:
		{
			fd32_getfsfree_t fsfree;
			char drive[4] = " :\\";
			drive[0] = fd32_get_default_drive();
			if (r->h.dl) drive[0] = r->h.dl + 'A' - 1;
			fsfree.Size = sizeof(fd32_getfsfree_t);
			res = fd32_get_fsfree(drive, &fsfree);
			LOG_PRINTF(("[DPMI] INT 21h: Get free disk space for \"%s\", returns %08x\n", drive, res));
			LOG_PRINTF(("                Got: %lu secperclus, %lu availclus, %lu bytspersec, %lu totclus\n",
			            fsfree.SecPerClus, fsfree.AvailClus, fsfree.BytesPerSec, fsfree.TotalClus));
			if (res < 0)
			{
				r->x.ax = 0xFFFF; /* Unusual way to report error condition */
				return;
			}
			r->x.ax = fsfree.SecPerClus;  /* AX = Sectors per cluster     */
			r->x.bx = fsfree.AvailClus;   /* BX = Number of free clusters */
			r->x.cx = fsfree.BytesPerSec; /* CX = Bytes per sector        */
			r->x.dx = fsfree.TotalClus;   /* DX = Total clusters on drive */
			return;
		}
		/* DOS 2+ - "MKDIR" - Create subdirectory (DS:DX->ASCIZ directory name) */
		case 0x39:
			res = fix_path(fn, (const char *) FAR2ADDR(r->x.ds, r->x.dx), sizeof(fn));
			LOG_PRINTF(("[DPMI] INT 21h: Make directory \"%s\"\n", fn));
			if (res >= 0) res = fd32_mkdir(fn);
			break;
		/* DOS 2+ - "RMDIR" - Remove subdirectory (DS:DX->ASCIZ directory name) */
		case 0x3A:
			res = fix_path(fn, (const char *) FAR2ADDR(r->x.ds, r->x.dx), sizeof(fn));
			LOG_PRINTF(("[DPMI] INT 21h: Remove directory \"%s\"\n", fn));
			if (res >= 0) res = fd32_rmdir(fn);
			break;
		/* DOS 2+ - "CHDIR" - Set current directory (DS:DX->ASCIZ directory name) */
		case 0x3B:
			res = fix_path(fn, (const char *) FAR2ADDR(r->x.ds, r->x.dx), sizeof(fn));
			LOG_PRINTF(("[DPMI] INT 21h: Change directory to \"%s\"\n", fn));
			if (res >= 0) res = fd32_chdir(fn);
			break;
		/* DOS 2+ - "CREAT" - Create or truncate file (DS:DX->ASCIZ file name,
		 * CX=attributes, volume label and directory not allowed).
		 */
		case 0x3C:
			res = fix_path(fn, (const char *) FAR2ADDR(r->x.ds, r->x.dx), sizeof(fn));
			LOG_PRINTF(("[DPMI] INT 21h: Creat \"%s\" with attr %04x\n", fn, r->x.cx));
			if (res < 0) break;
			res = dos_open(fn, O_RDWR | O_CREAT | O_TRUNC, r->x.cx, 0, NULL);
			res2dos(res, r);
			if (res >= 0) r->x.ax = (WORD) res; /* The new handle */
			return;
		/* DOS 2+ - "OPEN" - Open existing file (DS:DX->ASCIZ file name,
		 * AL=opening mode, CL=search attributes for server call).
		 */
		case 0x3D:
			res = fix_path(fn, (const char *) FAR2ADDR(r->x.ds, r->x.dx), sizeof(fn));
			LOG_PRINTF(("[DPMI] INT 21h %x %x : Open \"%s\" with mode %02x\n", r->x.ds, r->x.dx, fn, r->h.al));
			if (res < 0) break;
			res = dos_open(fn, r->h.al, FD32_ANONE, 0, NULL);
			res2dos(res, r);
			if (res >= 0) r->x.ax = (WORD) res; /* The new handle */
			return;
		/* DOS 2+ - "CLOSE" - Close file (BX=file handle) */
		case 0x3E:
			LOG_PRINTF(("[DPMI] INT 21h: Close handle %04x\n", r->x.bx));
			res = fd32_close(r->x.bx);
			break; /* Recent DOSes preserve AH, we are OK with this */
		/* DOS 2+ - "READ" - Read from file or device (BX=file handle,
		 * CX=bytes to read, DS:DX->transfer buffer).
		 */
		case 0x3F:
			res = fd32_read(r->x.bx, (void *) FAR2ADDR(r->x.ds, r->x.dx), r->x.cx);
			LOG_PRINTF(("[DPMI] INT 21h: Read (AH=3Fh) from handle %04x (%d bytes). Res=%08xh\n", r->x.bx, r->x.cx, res));
			if (res < 0) break;
			#if 0 /* FIXME: Can this be removed now? */
			/* This is a quick'n'dirty hack to return \n after \r from stdin.  */
			/* It works only if console input is on handle 0 and if the number */
			/* of bytes to read is greater than the first carriage return.     */
			/* TODO: Make this \r\n stuff functional according to the console. */
			if (r->x.bx == 0)
			{
				BYTE *buf = (BYTE *) FAR2ADDR(r->x.ds, r->x.dx);
				if (res < r->x.cx) buf[res++] = '\n';
			}
			#endif
			RMREGS_CLEAR_CARRY;
			r->x.ax = (WORD) res; /* bytes read */
			return;
		/* DOS 2+ - "WRITE" - Write to file or device (BX=file handle,
		 * CX=bytes to write, DS:DX->transfer buffer).
		 */
		case 0x40:
			res = fd32_write(r->x.bx, (/*const*/ void *) FAR2ADDR(r->x.ds, r->x.dx), r->x.cx);
			//LOG_PRINTF(("[DPMI] INT 21h: Write (AH=40h) to handle %04x (%d bytes). Res=%08xh\n", r->x.bx, r->x.cx, res));
			res2dos(res, r);
			if (res >= 0) r->x.ax = (WORD) res; /* bytes written */
			return;
		/* DOS 2+ - "UNLINK" - Delete file (DS:DX->ASCIZ file name,
		 * CL=search attributes for server call).
		 */
		case 0x41:
			res = fix_path(fn, (const char *) FAR2ADDR(r->x.ds, r->x.dx), sizeof(fn));
			LOG_PRINTF(("[DPMI] INT 21h: Delete file \"%s\"\n", fn));
			if (res >= 0) res = fd32_unlink(fn, FD32_FAALL | FD32_FRNONE);
			break;
		/* DOS 2+ - "LSEEK" - Set current file position (BX=file handle,
		 * CX:DX=offset, AL=origin).
		 * FIXME: Replace with off_t
		 */
		case 0x42:
			LOG_PRINTF(("[DPMI] INT 21h: lseek handle %04xh to %04xh:%04xh from %02xh\n", r->x.bx, r->x.cx, r->x.dx, r->h.al));
			res = fd32_lseek(r->x.bx, ((long long) r->x.cx << 16) + (long long) r->x.dx, r->h.al, &res64);
			res2dos(res, r);
			if (res >= 0)
			{
				r->x.ax = (WORD) res64;
				r->x.dx = (WORD) (res64 >> 16);
			}
			return;
		/* DOS 2+ - Get/set file attributes */
		case 0x43:
			dos_get_and_set_attributes(r);
			return;
		/* IOCTL functions
		 * TODO: They should be done... one day
		 */
		case 0x44:
			LOG_PRINTF(("[DPMI] INT 21h: IOCTL handle %04xh\n", r->x.bx));
			res = fd32_get_dev_info(r->x.bx);
			if (res < 0)
			{
				r->x.ax = DOS_EBADF;
				RMREGS_SET_CARRY;
			}
			else
			{
				r->x.ax = res;
				r->x.dx = r->x.ax;
				RMREGS_CLEAR_CARRY;
			}
			return;
		/* DOS 2+ - "DUP" - Duplicate file handle (BX=file handle) */
		case 0x45:
			LOG_PRINTF(("[DPMI] INT 21h: dup handle %04xh\n", r->x.bx));
			res = fd32_dup(r->x.bx);
			res2dos(res, r);
			if (res >= 0) r->x.ax = (WORD) res; /* new handle */
			return;
		/* DOS 2+ - "DUP2", "FORCEDUP" - Force duplicate file handle
		 * (BX=file handle, CX=new handle).
		 */
		case 0x46:
			LOG_PRINTF(("[DPMI] INT 21h: dup2, handle %04xh to %04xh\n", r->x.bx, r->x.cx));
			res = fd32_forcedup(r->x.bx, r->x.cx);
			break;
		/* DOS 2+ - "CWD" - Get current directory (DL=drive number, 0=default
		 * 1=A, etc., DS:DI->64 bytes buffer for ASCIZ path name).
		 */
		case 0x47:
		{
			char  drive[2] = "A";
			char *c;
			char *dest = (char *) FAR2ADDR(r->x.ds, r->x.si);
			LOG_PRINTF(("[DPMI] INT 21h: Get current dir of drive %02x\n", r->h.dl));
			drive[0] = fd32_get_default_drive();
			if (r->h.dl) drive[0] = r->h.dl + 'A' - 1;
			res = fd32_getcwd(drive, fn);
			if (res < 0) break;
			//LOG_PRINTF(("After getcwd:\"%s\"\n", fn));
			/* Convert all component to 8.3 */
			res = fd32_sfn_truename(fn, fn);
			if (res < 0) break;
			//LOG_PRINTF(("After sfn_truename:\"%s\"\n", fn));
			/* Skip drive specification and initial backslash as required from DOS */
			for (c = fn; *c != '\\'; c++);
			strcpy(dest, c + 1);
			//LOG_PRINTF(("Returned CWD:\"%s\"\n", dest));
			r->x.ax = 0x0100; /* Undocumented success return value */
			RMREGS_CLEAR_CARRY;
			/* TODO: Convert from UTF-8 to OEM */
			return;
		}
		case 0x4A:
		{
			LOG_PRINTF(("[DPMI] INT 21h: Resize Memory Block, BX=0x%x, ES=0x%x\n", r->x.bx, r->x.es));
			return;
		}
		/* DOS 2+ - "EXEC" - Load and/or Execute program */
		case 0x4B:
		{
			ExecParams *pb;
			char *args, *p, c[128];
			int cnt, i;
			LOG_PRINTF(("[DPMI] INT 21h: Load and Execute, al=%02xh\n", r->h.al));
			pb = (ExecParams *) FAR2ADDR(r->x.es, r->x.bx);
			/* Only AL = 0x00 - Load and Execute - is currently supported */
			if (r->h.al != 0)
			{
				r->h.al = 0xFF; /* Invalid subfunction */
				RMREGS_SET_CARRY;
				return;
			}
			p = (char *) FAR2ADDR(pb->arg_seg, pb->arg_offs);
			cnt = *p++;
			if (cnt == 0) {
				args = NULL;
			} else {
				for (i = 0; i < cnt; i++) {
					c[i] = *p++;
				}
				c[i] = 0;
				args = c;
			}
			res = fix_path(fn, (const char *) FAR2ADDR(r->x.ds, r->x.dx), sizeof(fn));
			LOG_PRINTF(("                dos_exec: \"%s\"\n", fn));
			if (res >= 0) res = dos_exec(fn, pb->Env, args, pb->Fcb1, pb->Fcb2, &dos_return_code);
			break;
		}
		/* DOS 2+ - Get return code */
		case 0x4D:
			LOG_PRINTF(("[DPMI] INT 21h: Get return code: %02xh\n", dos_return_code));
			RMREGS_CLEAR_CARRY;
			r->h.ah = 0; /* Only normal termination, for now... */
			r->h.al = dos_return_code;
			dos_return_code = 0; /* DOS clears it after reading it */
			return;
		/* DOS 2+ - "FINDFIRST" - Find first matching file (DS:DX->ASCIZ file spec,
		 * CX=allowable attributes (archive and read only ignored),
		 * AL=special flag for append (ignored).
		 */
		case 0x4E:
			res = fix_path(fn, (const char *) FAR2ADDR(r->x.ds, r->x.dx), sizeof(fn));
			LOG_PRINTF(("[DPMI] INT 21h: Findfirst \"%s\" with attr %04xh\n", fn, r->x.cx));
			if (res < 0) break;
			res = fd32_dos_findfirst(fn, r->x.cx, current_psp->dta);
			res2dos(res, r);
			return;
		/* DOS 2+ - "FINDNEXT" - Find next matching file */
		case 0x4F:
			LOG_PRINTF(("[DPMI] INT 21h: Findnext\n"));
			res = fd32_dos_findnext(current_psp->dta);
			res2dos(res, r);
			return;
		/* DOS 2+ - "RENAME" - Rename or move file (DS:DX->old name,
		 * ES:DI->new name, CL=attributes for server call).
		 */
		case 0x56:
		{
			char newfn[FD32_LFNPMAX];
			res = fix_path(fn, (const char *) FAR2ADDR(r->x.ds, r->x.dx), sizeof(fn));
			if (res < 0) break;
			res = fix_path(newfn, (const char *) FAR2ADDR(r->x.es, r->x.di), sizeof(newfn));
			if (res < 0) break;
			LOG_PRINTF(("[DPMI] INT 21h: Renaming \"%s\" to \"%s\"\n", fn, newfn));
			res = fd32_rename(fn, newfn);
			break;
		}
		/* DOS 2+ - Get/set file's time stamps */
		case 0x57:
			get_and_set_time_stamps(r);
			return;
		/* DOS 2+ - Get and set memory allocation strategy & UMB link state */
		case 0x58:
		{
			LOG_PRINTF(("[DPMI] INT 21h: Set and get mem stategy AL=0x%x\n", r->h.al));
			return;
		}
		/* DOS 3.0+ - Create new file (DS:DX->ASCIZ file name, CX=attributes) */
		case 0x5B:
			res = fix_path(fn, (const char *) FAR2ADDR(r->x.ds, r->x.dx), sizeof(fn));
			LOG_PRINTF(("[DPMI] INT 21h: Creating new file (AH=5Bh) \"%s\" with attr %04x\n", fn, r->x.cx));
			if (res < 0) break;
			res = dos_open(fn, O_RDWR | O_CREAT | O_EXCL, r->x.cx, 0, NULL);
			res2dos(res, r);
			if (res >= 0) r->x.ax = (WORD) res; /* The new handle */
			return;
		/* DOS 3.0+ - Get current PSP address */
		case 0x62:
			LOG_PRINTF(("[DPMI] INT 21h: Get current PSP address\n"));
			r->x.bx = (DWORD) current_psp >> 4;
			return;
		/* DOS 3.3+ - "FFLUSH" - Commit file
		 * and DOS 4.0+ Undocumented - Commit file (BX=file handle).
		 */
		case 0x68:
		case 0x6A:
			LOG_PRINTF(("[DPMI] INT 21h: fflush\n"));
			/* BX file handle */
			res = fd32_fflush(r->x.bx);
			break;
		/* DOS 5+ Undocumented - Null function */
		case 0x6B:
			LOG_PRINTF(("[DPMI] INT 21h: Null function. Hey, somebody calls it!\n"));
			r->h.al = 0;
			RMREGS_CLEAR_CARRY;
			return;
		/* DOS 4.0+ - Extended open/create file (AL=00h, BL=access and sharing mode,
		 * BH=flags, CX=attributes for creation, DL=action, DH=00h, DS:DI->ASCIZ file name).
		 */
		case 0x6C:
		{
			int action;
			res = parse_open_action(r->x.dx);
			if (res < 0) break;
			action = res;
			res = fix_path(fn, (const char *) FAR2ADDR(r->x.ds, r->x.si), sizeof(fn));
			if (res < 0) break;
			res = dos_open(fn, action | (DWORD) r->x.bx, r->x.cx, 0, &action);
			LOG_PRINTF(("[DPMI] INT 21h: Extended open/create (AH=6Ch) \"%s\" res=%08xh\n", fn, res));
			res2dos(res, r);
			if (res >= 0)
			{
				r->x.ax = (WORD) res; /* The new handle */
				r->x.cx = (WORD) action; /* Action taken */
			}
			return;
		}
		/* Long File Name functions */
		case 0x71:
			if (use_lfn) lfn_functions(r);
			else
			{
				r->x.ax = 0x7100; /* Yet another convention */
				RMREGS_SET_CARRY;
			}
			return;
		/* Windows 95 beta - "FindClose" - Terminate directory search
		* _Guessing_ that BX is the directory handle.
		*/
		case 0x72:
			LOG_PRINTF(("[DPMI] INT 21h: Win95 beta FindClose\n"));
			res = fd32_lfn_findclose(r->x.bx);
			break;
		/* Windows 95 FAT32 services.
		 * Only AH=03h "Get extended free space on drive" is currently supported.
		 */
		case 0x73:
		{
			ExtDiskFree *edf;
			fd32_getfsfree_t fsfree;
			LOG_PRINTF(("[DPMI] INT 21h: FAT32 service %02x issued\n", r->h.al));
			if (r->h.al != 0x03)
			{
				r->h.al = 0xFF; /* Unimplemented subfunction */
				RMREGS_SET_CARRY;
				return;
			}
			/* DS:DX->ASCIZ string for drive ("C:\" or "\\SERVER\Share")
			 * ES:DI->Buffer for extended free space structure (ExtFreSpace)
			 * CX=size of ES:DI buffer
			 */
			if (r->x.cx < sizeof(ExtDiskFree))
			{
				r->x.ax = DOS_EBUFSMALL;
				RMREGS_SET_CARRY;
				return;
			}
			edf = (ExtDiskFree *) FAR2ADDR(r->x.es, r->x.di);
			if (edf->Version != 0x0000)
			{
				r->x.ax = DOS_EINVAL;
				RMREGS_SET_CARRY;
				return;
			}
			fsfree.Size = sizeof(fd32_getfsfree_t);
			res = fix_path(fn, (const char *) FAR2ADDR(r->x.ds, r->x.dx), sizeof(fn));
			if (res < 0) break;
			res = fd32_get_fsfree(fn, &fsfree);
			if (res >= 0)
			{
				edf->Size = sizeof(ExtDiskFree);
				/* Currently we don't support volume compression */
				edf->SecPerClus  = fsfree.SecPerClus;
				edf->BytesPerSec = fsfree.BytesPerSec;
				edf->AvailClus   = fsfree.AvailClus;
				edf->TotalClus   = fsfree.TotalClus;
				edf->RealSecPerClus  = fsfree.SecPerClus;
				edf->RealBytesPerSec = fsfree.BytesPerSec;
				edf->RealAvailClus   = fsfree.AvailClus;
				edf->RealTotalClus   = fsfree.TotalClus;
			}
			break;
		}
		/* Unsupported or invalid functions */
		default:
			r->x.ax = DOS_ENOTSUP;
			RMREGS_SET_CARRY;
			return;
	}
	res2dos(res, r);
}
