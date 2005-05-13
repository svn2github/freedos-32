/**************************************************************************
 * INT 21h handler for the FreeDOS32 DPMI Driver                          *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2005, Salvatore Isaja                               *
 *                                                                        *
 * This is "int21.c" - A wrapper to provide DOS application the standard  *
 *                     DOS INT 21h API they need to run under FD32.       *
 *                                                                        *
 *                                                                        *
 * This file is part of the FreeDOS32 DPMI Driver (the DRIVER).           *
 *                                                                        *
 * The DRIVER is free software; you can redistribute it and/or modify it  *
 * under the terms of the GNU General Public License as published by the  *
 * Free Software Foundation; either version 2 of the License, or (at your *
 * option) any later version.                                             *
 *                                                                        *
 * The DRIVER is distributed in the hope that it will be useful, but      *
 * WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with the DRIVER; see the file GPL.txt;                           *
 * if not, write to the Free Software Foundation, Inc.,                   *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA                *
 **************************************************************************/

/* TODO: We should provide some mechanism for installing new fake
         interrupt handlers (like this INT 21h handler), with the   
         possibility of recalling the old one.
*/

#include <ll/i386/hw-data.h>
#include <ll/i386/string.h>
#include <kernel.h>
#include <filesys.h>
#include <errors.h>
#include <stubinfo.h>
#include <logger.h>
#include <fd32time.h>
#include "rmint.h"

/* Define the __DEBUG__ symbol in order to activate log output */
//#define __DEBUG__
#ifdef __DEBUG__
 #define LOG_PRINTF(s) fd32_log_printf s
#else
 #define LOG_PRINTF(s)
#endif


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
__attribute__ ((packed)) tExecParams;
/* DOS return code of the last executed program */
static WORD DosReturnCode;


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
__attribute__ ((packed)) tExtDiskFree;


/* Lookup table to convert FD32 errno codes to DOS error
 * codes (see RBIL table 01680).
 * For unknown mappings, the DOS error 0x64 is used,
 * that is "(MSCDEX) unknown error".
 */
static const BYTE errno2dos[] = {
/*   0 */ 0x00, 0x64, 0x02, 0x64, 0x64, 0x17, 0x14, 0x64,
/*   8 */ 0xC1, 0x06, 0x64, 0x64, 0x08, 0x05, 0x09, 0x14,
/*  16 */ 0x64, 0x50, 0x11, 0x14, 0x03, 0x05, 0xA0, 0x04,
/*  24 */ 0x04, 0x64, 0x64, 0x05, 0x27, 0x64, 0x13, 0x64,
/*  32 */ 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64,
/*  40 */ 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0xB4, 0x64,
/*  48 */ 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x01, 0x64,
/*  56 */ 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64,
/*  64 */ 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64,
/*  72 */ 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x0B,
/*  80 */ 0x64, 0x06, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64,
/*  88 */ 0x01, 0x04, 0x05, 0x7A, 0x05, 0x64, 0x64, 0x01,
/*  96 */ 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64,
/* 104 */ 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64,
/* 112 */ 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64,
/* 120 */ 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64,
/* 128 */ 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x01, 0x15,
/* 136 */ 0x64, 0x64, 0x0D
};


#if 0
/* TODO: Cleanup path name shortening */
#include <unicode/unicode.h>
/* From NLS support */
int oemcp_to_utf8(char *Source, uint8_t *Dest);
int utf8_to_oemcp(uint8_t *Source, int SourceSize, char *Dest, int DestSize);
int oemcp_skipchar(char *Dest);

/* This function is temporary, while waiting for the new NLS support */
static int utf8_strupr(char *restrict dest, const char *restrict source)
{
	wchar_t wc;
	int res;
	while (*source)
	{
		res = unicode_utf8towc(&wc, source, 6);
		if (res < 0) return -1;
		source += res;
		res = unicode_wctoutf8(dest, toupper(wc), 6);
		if (res < 0) return -1;
		dest += res;
	}
	*dest = 0;
	return 0;
}

/* Given a file name Source in UTF-8, checks if it's valid */
/* and returns in Dest the file name in FCB format.        */
/* On success, returns 0 if no wildcards are present, or a */
/* positive number if '?' wildcards are present in Dest.   */
/* On failure, returns a negative error code.              */
/* NOTE: Pasted from fd32/filesys/names.c (that has to be removed) */
static int build_fcb_name(BYTE *Dest, char *Source)
{
  int   WildCards = 0;
  char  SourceU[FD32_LFNPMAX];
  int   Res;
  int   k;

  /* Name and extension have to be padded with spaces */
  memset(Dest, 0x20, 11);
  
  /* Build ".          " and "..         " if Source is "." or ".." */
  if ((strcmp(Source, ".") == 0) || (strcmp(Source, "..") == 0))
  {
    for (; *Source; Source++, Dest++) *Dest = *Source;
    return 0;
  }

  if ((Res = utf8_strupr(SourceU, Source)) < 0) return FD32_EUTF8;
  for (k = 0; (SourceU[k] != '.') && SourceU[k]; k++);
  utf8_to_oemcp(SourceU, SourceU[k] ? k : -1, Dest, 8);
  if (SourceU[k]) utf8_to_oemcp(&SourceU[k + 1], -1, &Dest[8], 3);

  if (Dest[0] == ' ') return FD32_EFORMAT;
  if (Dest[0] == 0xE5) Dest[0] = 0x05;
  for (k = 0; k < 11;)
  {
    if (Dest[k] < 0x20) return FD32_EFORMAT;
    switch (Dest[k])
    {
      case '"': case '+': case ',': case '.': case '/': case ':': case ';':
      case '<': case '=': case '>': case '[': case '\\': case ']':  case '|':
        return FD32_EFORMAT;
      case '?': WildCards = 1;
                k++;
                break;
      case '*': WildCards = 1;
                if (k < 8) for (; k < 8; k++) Dest[k] = '?';
                else for (; k < 11; k++) Dest[k] = '?';
                break;
      default : k += oemcp_skipchar(&Dest[k]);
    }
  }
  return WildCards;
}


/* Gets a UTF-8 short file name from an FCB file name. */
/* NOTE: Pasted from fd32/filesys/names.c (that has to be removed) */
static int expand_fcb_name(char *Dest, BYTE *Source)
{
  BYTE *NameEnd;
  BYTE *ExtEnd;
  char  Aux[13];
  BYTE *s = Source;
  char *a = Aux;

  /* Count padding spaces at the end of the name and the extension */
  for (NameEnd = Source + 7;  *NameEnd == ' '; NameEnd--);
  for (ExtEnd  = Source + 10; *ExtEnd  == ' '; ExtEnd--);

  /* Put the name, a dot and the extension in Aux */
  if (*s == 0x05) *a++ = (char) 0xE5, s++;
  for (; s <= NameEnd; *a++ = (char) *s++);
  if (Source + 8 <= ExtEnd) *a++ = '.';
  for (s = Source + 8; s <= ExtEnd; *a++ = (char) *s++);
  *a = 0;

  /* And finally convert Aux from the OEM code page to UTF-8 */
  return oemcp_to_utf8(Aux, Dest);
}


/* Converts the passed path name to a valid path of 8.3 names,  */
/* in the OEM code page, as required for short file name calls. */
/* Returns 0 on success, or a negative error code on failure.   */
/* TODO: It is currently assumed that Source is in UTF-8.
         This is not true for DOS calls since the charset is OEM. */
static int make_sfn_path(char *Dest, const char *Source)
{
  const char *s;
  char        Comp[FD32_LFNMAX];
  char       *c;
  BYTE        FcbName[11];
  int         Res;
  #ifdef __DEBUG__
  char       *Save = Dest;
  #endif

  /* Copy drive specification if present */
  for (s = Source, c = Dest; *s && (*s != ':'); *(c++) = *(s++));
  if (*s)
  {
    Source = s + 1;
    *c     = ':';
    Dest   = c + 1;
  }
  /* Truncate each path component to 8.3 */
  while (*Source)
  {
    if ((*Source == '\\') || (*Source == '/')) *(Dest++) = *(Source++);
    while ((*Source =='\\') || (*Source == '/')) Source++; /* ignore consecutive slashes */
    /* Extract the next partial from the path (before '\' or '/' or NULL) */
    for (c = Comp; (*Source != '\\') && (*Source != '/') && *Source; *(c++) = *(Source++));
    *c = 0;
    if (*Comp)
    {
      /* Comp is not empty, let's shrink it */
      if ((Res = build_fcb_name(FcbName, Comp)) < 0) return Res;
      if ((Res = expand_fcb_name(Comp, FcbName)) < 0) return Res;
      /* And append shrunk Comp to the Dest string, without trailing backslash */
      for (c = Comp; *c; *(Dest++) = *(c++));
    }
  }
  /* Finally add the null terminator */
  *Dest = 0;
  LOG_PRINTF(("INT 21h - Name shortened to '%s'\n", Save));
  return 0;
}


/* Fix '/' in the long file name into '\\' */
static char *fix_lfn_path(char *Dest)
{
  DWORD i;
  
  for(i = 0; Dest[i] != 0; i++)
    if(Dest[i] == '/')
      Dest[i] = '\\';
  return Dest;
}
#endif


#include <unicode/unicode.h>
#include <nls/nls.h>
struct nls_operations *current_cp;
/** \brief Converts a path name from DOS API to UTF-8.
 *  \param dest buffer to hold the path name converted to UTF-8;
 *  \param source source path name in some national code page;
 *  \param size size in bytes of the buffer pointed by \c dest.
 *  \remarks Forward slashes '/' are converted to back slashes '\'.
 *  \return 0 on success, or a negative error.
 */
static int fix_path(char *restrict dest, const char *restrict source, size_t size)
{
	#if 0 /* Input is in OEM code page: use Unicode and NLS support */
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
	#else /* Input is already in UTF-8: no conversion needed */
	char c;
	while (*source)
	{
		if (!size) return FD32_EFORMAT; //-ENAMETOOLONG
		c = *(source++);
		if (c == '/') c = '\\';
		*(dest++) = c;
	}
	#endif
	if (!size) return FD32_EFORMAT; //-ENAMETOOLONG
	*dest = 0;
	return 0;
}
#define make_sfn_path(dest, source) fix_path(dest, source, sizeof(dest))
#define FAR2ADDR(seg, off) (((DWORD) seg << 4) + (DWORD) off)


/* Given a FD32 return code, prepare the standard DOS return status.
 * - on error: carry flag set, error (positive) in r->x.ax
 * - on success: carry flag clear
 */
static void dos_return(int Res, union rmregs *r)
{
  LOG_PRINTF(("INT 21h - Result: %08x\n", Res));
  RMREGS_CLEAR_CARRY;
  if (Res < 0)
  {
    RMREGS_SET_CARRY;
    #if 0 /* Use new errno codes */
    Res = -Res;
    r->x.ax = 0x64;
    if (Res < sizeof(errno2dos)) r->x.ax = errno2dos[Res];
    #else /* Use old FD32_E* error codes */
    /* Keeping the low 16 bits of a negative FD32 error code */
    /* yelds a positive DOS error code.                      */
    r->x.ax = (WORD) Res; 
    #endif
  }
}


/* Sub-handler for DOS services "Get/set file's time stamps" */
/* INT 21h, AH=57h.                                          */
static inline void get_and_set_time_stamps(union rmregs *r)
{
  int            Res;
  fd32_fs_attr_t A;

  /* BX file handle */
  /* AL action      */
  A.Size = sizeof(fd32_fs_attr_t);
  switch (r->h.al)
  {
    /* Get last modification date and time */
    case 0x00:
      /* CX time in DOS format */
      /* DX date in DOS format */
      /* FIX ME: MS-DOS returns date and time of opening for char devices */
      Res = fd32_get_attributes(r->x.bx, &A);
      if (Res < 0) goto Error;
      r->x.cx = A.MTime;
      r->x.dx = A.MDate;
      goto Success;

    /* Set last modification date and time */
    case 0x01:
      /* CX new time in DOS format */
      /* DX new date in DOS format */
      Res = fd32_get_attributes(r->x.bx, &A);
      if (Res < 0) goto Error;
      A.MTime = r->x.cx;
      A.MDate = r->x.dx;
      Res = fd32_set_attributes(r->x.bx, &A);
      if (Res < 0) goto Error;
      goto Success;

    /* Get last access date and time */
    case 0x04:
      /* DX date in DOS format                          */
      /* CX time in DOS format (currently always 0000h) */
      Res = fd32_get_attributes(r->x.bx, &A);
      if (Res < 0) goto Error;
      r->x.dx = A.ADate;
      r->x.cx = 0x0000;
      goto Success;

    /* Set last access date and time */
    case 0x05:
      /* CX new time in DOS format (must be 0000h) */
      /* DX new date in DOS format                 */
      if (r->x.cx != 0x0000)
      {
        Res = FD32_EINVAL;
        goto Error;
      }
      Res = fd32_get_attributes(r->x.bx, &A);
      if (Res < 0) goto Error;
      A.ADate = r->x.dx;
      Res = fd32_set_attributes(r->x.bx, &A);
      if (Res < 0) goto Error;
      goto Success;

    /* Get creation date and time */
    case 0x06:
      /* DX date in DOS format                                 */
      /* CX time in DOS format                                 */
      /* SI hundredths of seconds past the time in CX (0..199) */
      Res = fd32_get_attributes(r->x.bx, &A);
      if (Res < 0) goto Error;
      r->x.dx = A.CDate;
      r->x.cx = A.CTime;
      r->x.si = A.CHund;
      goto Success;

    /* Set creation date and time */
    case 0x07:
      /* DX new date in DOS format                             */
      /* CX new time in DOS format                             */
      /* SI hundredths of seconds past the time in CX (0..199) */
      Res = fd32_get_attributes(r->x.bx, &A);
      if (Res < 0) goto Error;
      A.CDate = r->x.dx;
      A.CTime = r->x.cx;
      A.CHund = r->x.si;
      Res = fd32_set_attributes(r->x.bx, &A);
      if (Res < 0) goto Error;
      goto Success;

    /* Invalid actions */
    default:
      Res = FD32_EINVAL;
      goto Error;
  }

  Error:
    r->x.ax = (WORD) Res;
    RMREGS_SET_CARRY;
    return;

  Success:
    RMREGS_CLEAR_CARRY;
    return;
}


/* Sub-handler for DOS service "Get/set file's attributes" */
/* INT 21h, AH=43h.                                        */
static inline void dos_get_and_set_attributes(union rmregs *r)
{
  int            Handle, Res;
  fd32_fs_attr_t A;
  char           SfnPath[FD32_LFNPMAX];
  
  /* DS:DX pointer to the ASCIZ file name */
  Res = make_sfn_path(SfnPath, (char *) FAR2ADDR(r->x.ds, r->x.dx));
  dos_return(Res, r);
  if (Res < 0) return;
  Handle = fd32_open(SfnPath, FD32_OREAD | FD32_OEXIST, FD32_ANONE, 0, NULL);
  if (Handle < 0) /* It could be a directory... */
    Handle = fd32_open(SfnPath, FD32_OREAD | FD32_OEXIST | FD32_ODIR, FD32_ANONE, 0, NULL);
  dos_return(Handle, r);
  if (Handle < 0) return;
  /* AL contains the action */
  A.Size = sizeof(fd32_fs_attr_t);
  switch (r->h.al)
  {
    /* Get file attributes */
    case 0x00:
      LOG_PRINTF(("INT 21h - Getting attributes of \"%s\"\n", SfnPath));
      /* CX attributes        */
      /* AX = CX (DR-DOS 5.0) */
      Res = fd32_get_attributes(Handle, &A);
      if (Res < 0) goto Error;
      r->x.ax = r->x.cx = A.Attr;
      goto Success;

    /* Set file attributes */
    case 0x01:
      LOG_PRINTF(("INT 21h - Setting attributes of \"%s\" to %04x\n", SfnPath, r->x.cx));
      /* CX new attributes */
      /* Volume label and directory attributes cannot be changed. */
      if (r->x.cx & (FD32_AVOLID | FD32_ADIR))
      {
        Res = FD32_EACCES;
        goto Error;
      }
      Res = fd32_get_attributes(Handle, &A);
      /* FIX ME: Should close the file if currently open in sharing-compat */
      /*         mode, or generate a sharing violation critical error in   */
      /*         the file is currently open.                               */
      /* Mah! Would that be true with the new things? Mah...               */
      if (Res < 0) goto Error;
      /* Volume label and directory attributes cannot be changed.         */
      /* To change the other attributes bits of a directory the directory */
      /* flag must be clear, even if it will be set after this call.      */
      if (A.Attr == FD32_AVOLID)
      {
        Res = FD32_EACCES;
        goto Error;
      }
      if (A.Attr & FD32_ADIR) A.Attr = r->x.cx | FD32_ADIR;
                         else A.Attr = r->x.cx;
      Res = fd32_set_attributes(Handle, &A);
      if (Res < 0) goto Error;
      goto Success;

    default:
      Res = FD32_EINVAL;
      goto Error;
  }

  Error:
    r->x.ax = (WORD) Res;
    Res = fd32_close(Handle);
    if (Res < 0) r->x.ax = (WORD) Res;
    RMREGS_SET_CARRY;
    return;

  Success:
    Res = fd32_close(Handle);
    dos_return(Res, r);
    return;
}


/* Sub-handler for LFN service "Extended get/set attributes" */
/* INT 21h, AH=71h, AL=43h.                                  */
static inline void lfn_get_and_set_attributes(union rmregs *r)
{
  int            Handle, Res;
  fd32_fs_attr_t A;
  char fn[FD32_LFNPMAX];

  /* DS:DX pointer to the ASCIZ file name */
  Res = fix_path(fn, (char *) FAR2ADDR(r->x.ds, r->x.dx), sizeof(fn));
  dos_return(Res, r);
  Handle = fd32_open(fn, FD32_ORDWR | FD32_OEXIST, FD32_ANONE, 0, NULL);
  if (Handle < 0) /* It could be a directory... */
    Handle = fd32_open(fn, FD32_ORDWR | FD32_OEXIST | FD32_ODIR, FD32_ANONE, 0, NULL);
  dos_return(Handle, r);
  if (Handle < 0) return;
  /* BL contains the action */
  A.Size = sizeof(fd32_fs_attr_t);
  switch (r->h.bl)
  {
    /* Get file attributes */
    case 0x00:
      /* CX attributes */
      Res = fd32_get_attributes(Handle, &A);
      if (Res < 0) goto Error;
      r->x.cx = A.Attr;
      goto Success;

    /* Set file attributes */
    case 0x01:
      /* CX new attributes */
      Res = fd32_get_attributes(Handle, &A);
      if (Res < 0) goto Error;
      A.Attr = r->x.cx;
      Res = fd32_set_attributes(Handle, &A);
      if (Res < 0) goto Error;
      goto Success;

    /* Get physical size of compressed file */
    /* case 0x02: Not implemented           */

    /* Set last modification date and time */
    case 0x03:
      /* DI new date in DOS format */
      /* CX new time in DOS format */
      Res = fd32_get_attributes(Handle, &A);
      if (Res < 0) goto Error;
      A.MDate = r->x.di;
      A.MTime = r->x.cx;
      Res = fd32_set_attributes(Handle, &A);
      if (Res < 0) goto Error;
      goto Success;

    /* Get last modification date and time */
    case 0x04:
      /* DI date in DOS format */
      /* CX time in DOS format */
      Res = fd32_get_attributes(Handle, &A);
      if (Res < 0) goto Error;
      r->x.di = A.MDate;
      r->x.cx = A.MTime;
      goto Success;

    /* Set last access date */
    case 0x05:
      /* DI new date in DOS format */
      Res = fd32_get_attributes(Handle, &A);
      if (Res < 0) goto Error;
      A.ADate = r->x.di;
      Res = fd32_set_attributes(Handle, &A);
      if (Res < 0) goto Error;
      goto Success;

    /* Get last access date */
    case 0x06:
      /* DI date in DOS format */
      Res = fd32_get_attributes(Handle, &A);
      if (Res < 0) goto Error;
      r->x.di = A.ADate;
      goto Success;

    /* Set creation date and time */
    case 0x07:
      /* DI new date in DOS format                             */
      /* CX new time in DOS format                             */
      /* SI hundredths of seconds past the time in CX (0..199) */
      Res = fd32_get_attributes(Handle, &A);
      if (Res < 0) goto Error;
      A.CDate = r->x.di;
      A.CTime = r->x.cx;
      A.CHund = r->x.si;
      Res = fd32_set_attributes(Handle, &A);
      if (Res < 0) goto Error;
      goto Success;

    /* Get creation date and time */
    case 0x08:
      /* DI date in DOS format                                 */
      /* CX time in DOS format                                 */
      /* SI hundredths of seconds past the time in CX (0..199) */
      Res = fd32_get_attributes(Handle, &A);
      if (Res < 0) goto Error;
      r->x.di = A.CDate;
      r->x.cx = A.CTime;
      r->x.si = A.CHund;
      goto Success;

    /* Unsupported or invalid actions */
    default:
      r->x.ax = 0x7100; /* Function not supported */
      Res = fd32_close(Handle);
      if (Res < 0) r->x.ax = (WORD) Res;
      RMREGS_SET_CARRY;
      return;
  }

  Error:
    r->x.ax = (WORD) Res;
    Res = fd32_close(Handle);
    if (Res < 0) r->x.ax = (WORD) (-Res);
    RMREGS_SET_CARRY;
    return;

  Success:
    Res = fd32_close(Handle);
    dos_return(Res, r);
    return;
}


/* Sub-handler for Long File Names services (INT 21h AH=71h) */
static inline void lfn_functions(union rmregs *r)
{
  int Res;
  char fn[FD32_LFNPMAX];

  switch (r->h.al)
  {
    /* Make directory */
    case 0x39:
      /* DS:DX pointer to the ASCIZ directory name */
      Res = fix_path(fn, (char *) FAR2ADDR(r->x.ds, r->x.dx), sizeof(fn));
      dos_return(Res, r);
      Res = fd32_mkdir(fn);
      dos_return(Res, r);
      return;

    /* Remove directory */
    case 0x3A:
      /* DS:DX pointer to the ASCIZ directory name */
      Res = fix_path(fn, (char *) FAR2ADDR(r->x.ds, r->x.dx), sizeof(fn));
      dos_return(Res, r);
      Res = fd32_rmdir(fn);
      dos_return(Res, r);
      return;

    /* Change directory */
    case 0x3B:
      /* DS:DX pointer to the ASCIZ directory name */
      Res = fix_path(fn, (char *) FAR2ADDR(r->x.ds, r->x.dx), sizeof(fn));
      dos_return(Res, r);
      Res = fd32_chdir(fn);
      dos_return(Res, r);
      return;

    /* Delete file */
    case 0x41:
      /* DS:DX pointer to the ASCIZ file name                               */
      /* SI    use wildcard and attributes: 0000h not allowed, 0001 allowed */
      /* CL    search attributes                                            */
      /* CH    must-match attributes                                        */
      /* FIX ME: Parameter in SI is not yet used!                           */
      Res = fix_path(fn, (char *) FAR2ADDR(r->x.ds, r->x.dx), sizeof(fn));
      dos_return(Res, r);
      Res = fd32_unlink(fn, ((DWORD) r->h.ch << 8) + (DWORD) r->h.cl);
      dos_return(Res, r);
      return;

    /* Extended get/set file attributes */
    case 0x43:
      lfn_get_and_set_attributes(r);
      return;

    /* Get current directory */
    /* Same as INT 21 AH=47h, but we don't convert to 8.3 the resulting path */
    case 0x47:
    {
      /* DL    drive number (00h = default, 01h = A:, etc) */
      /* DS:SI 64-byte buffer for ASCIZ pathname           */
      char  Drive[2] = "\0\0";
      char  Cwd[FD32_LFNPMAX];
      char *Dest = (char *) FAR2ADDR(r->x.ds, r->x.si);

      Drive[0] = fd32_get_default_drive();
      if (r->h.dl != 0x00) Drive[0] = r->h.dl + 'A' - 1;
      fd32_getcwd(Drive, Cwd);
      strcpy(Dest, Cwd + 1); /* Skip leading backslash as required from DOS */
      /* FIXME: Convert from UTF-8 to OEM */
      return;
    }

    /* Find first matching file */
    case 0x4E:
      /* CL allowable attributes                                 */
      /* CH required attributes                                  */
      /* SI date/time format: 0000h Windows time, 0001h DOS time */
      /* DS:DX pointer to the ASCIZ file specification           */
      /* ES:DI pointer to the find data record                   */
      /* FIX ME: Parameter in SI is not yet used!                */
      Res = fix_path(fn, (char *) FAR2ADDR(r->x.ds, r->x.dx), sizeof(fn));
      dos_return(Res, r);
      Res = fd32_lfn_findfirst(fn, ((DWORD) r->h.ch << 8) + (DWORD) r->h.cl,
                               (fd32_fs_lfnfind_t *) FAR2ADDR(r->x.es, r->x.di));
      if (Res < 0)
      {
        RMREGS_SET_CARRY;
        r->x.ax = (WORD) Res;
        return;
      }
      RMREGS_CLEAR_CARRY;
      r->x.ax = (WORD) Res; /* The directory handle to continue the search */
      r->x.cx = 0; /* FIX ME: Not yet implemented: Unicode conversion flag */
                   /* bit 0 set if the returned long name contains         */
                   /*  underscores for unconvertable Unicode characters    */
                   /* bit 1 set if the returned short name contains        */
                   /*  underscores for unconvertable Unicode characters    */
      return;

    /* Find next matching file */
    case 0x4F:
      /* BX    directory handle                                     */
      /* SI    date/time format: 0000h Windows time, 0001h DOS time */
      /* ES:DI pointer to the find data record                      */
      /* FIX ME: Parameter in SI should be mixed with old search flags! */
      Res = fd32_lfn_findnext(r->x.bx, 0, (fd32_fs_lfnfind_t *) FAR2ADDR(r->x.es, r->x.di));
      dos_return(Res, r);
      if (Res == 0) r->x.cx = 0;
                   /* FIX ME: Not yet implemented: Unicode conversion flag */
                   /* bit 0 set if the returned long name contains         */
                   /*  underscores for unconvertable Unicode characters    */
                   /* bit 1 set if the returned short name contains        */
                   /*  underscores for unconvertable Unicode characters    */
      return;

    /* Rename of move file */
    case 0x56:
    {
      /* DS:DX pointer to the ASCIZ name of the file to be renamed */
      /* ES:DI pointer to the ASCIZ new name for the file          */
      char newn[FD32_LFNPMAX];
      Res = fix_path(fn, (char *) FAR2ADDR(r->x.ds, r->x.dx), sizeof(fn));
      dos_return(Res, r);
      Res = fix_path(newn, (char *) FAR2ADDR(r->x.es, r->x.di), sizeof(newn));
      dos_return(Res, r);
      Res = fd32_rename(fn, newn);
      dos_return(Res, r);
      return;
    }
      
    /* Truename */
    case 0x60:
      /* Only subservice 01h "Get short (8.3) filename for file" is implemented */
      if (r->h.cl != 0x01)
      {
        dos_return(FD32_EINVAL, r);
        return;
      }
      /* CH    SUBST expansion flag (ignored): 80h to not resolve SUBST */
      /* DS:SI pointer to the ASCIZ long filename or path               */
      /* ES:DI pointer to a buffer for short pathname                   */
      Res = fix_path(fn, (char *) FAR2ADDR(r->x.ds, r->x.si), sizeof(fn));
      dos_return(Res, r);
      Res = fd32_sfn_truename((char *) FAR2ADDR(r->x.es, r->x.di), fn);
      dos_return(Res, r);
      return;

    /* Create or open file */
    case 0x6C:
    {
      int Action;
      /* BX    access mode and sharing flags    */
      /* CX    attribute mask for file creation */
      /* DX    action                           */
      /* DS:SI pointer to the ASCIZ file name   */
      /* DI    numeric alias hint               */
      Res = fix_path(fn, (char *) FAR2ADDR(r->x.ds, r->x.si), sizeof(fn));
      dos_return(Res, r);
      Res = fd32_open(fn, ((DWORD) r->x.dx << 16) + (DWORD) r->x.bx,
                      r->x.cx, r->x.di, &Action);
      if (Res < 0)
      {
        RMREGS_SET_CARRY;
        r->x.ax = (WORD) Res;
        return;
      }
      RMREGS_CLEAR_CARRY;
      r->x.ax = (WORD) Res;    /* The new handle */
      r->x.cx = (WORD) Action; /* Action taken   */
      return;
    }

    /* Get volume informations */
    case 0xA0:
    {
      fd32_fs_info_t FSInfo;
      /* DS:DX pointer to the ASCIZ root directory name          */
      /* ES:DI pointer to a buffer to store the file system name */
      /* CX    size of the buffer pointed by ES:DI               */
      Res = fix_path(fn, (char *) FAR2ADDR(r->x.ds, r->x.dx), sizeof(fn));
      dos_return(Res, r);
      FSInfo.Size       = sizeof(fd32_fs_info_t);
      FSInfo.FSNameSize = r->x.cx;
      FSInfo.FSName     = fn;
      Res = fd32_get_fsinfo((char *) FAR2ADDR(r->x.ds, r->x.dx), &FSInfo);
      r->x.bx = FSInfo.Flags;
      r->x.cx = FSInfo.NameMax;
      r->x.dx = FSInfo.PathMax;
      dos_return(Res, r);
      return;
    }

    /* "FindClose" - Terminate directory search */
    case 0xA1:
      /* BX directory handle */
      Res = fd32_lfn_findclose(r->x.bx);
      dos_return(Res, r);
      return;

    /* Unsupported or invalid functions */
    default:
      RMREGS_SET_CARRY;
      r->x.ax = 0x7100;
      return;
  }
}


/* INT 21h handler for DOS file system services */
void int21_handler(union rmregs *r)
{
  int      Res;
  int      tmp;
  long long int llRes;
  char     SfnPath[FD32_LFNPMAX];

  switch (r->h.ah)
  {
    /* DOS 1+ - Direct character input, without echo */
    case 0x07:
      /* Perform 1-byte read from the stdin, file handle 0.
         1. Save the old mode of stdin
         2. Set the stdin to the raw mode without echo
         3. Read 1 byte from stdin and restore the old mode
         This procedure is very alike when doing on the Win32 platform
       */
      /* This call doesn't check for Ctrl-C or Ctrl-Break.  */
      fd32_get_attributes(0, &tmp);
      Res = 0; /* Res used to set the attributes only once */
      fd32_set_attributes(0, &Res);
      Res = fd32_read(0, &(r->h.al), 1);
      fd32_set_attributes(0, &tmp);
      /* Return the character in AL */
      /* TODO:
         1. Get rid of the warning: passing arg 2 of `fd32_get_attributes'
            from incompatible pointer type, maybe using another system call?
         2. from RBIL: if the interim console flag is set (see AX=6301h),
            partially-formed double-byte characters may be returned
       */
      return;

    /* DOS 1+ - Set default drive */
    case 0x0E:
      LOG_PRINTF(("INT 21h - Set default drive to %02x\n", r->h.dl));
      /* DL new default drive (0='A', etc.) */
      Res = fd32_set_default_drive(r->h.dl + 'A');
      if (Res < 0) return; /* No error code is documented. Check this. */
      /* Return the number of potentially valid drive letters in AL */
      r->h.al = (BYTE) Res;
      RMREGS_CLEAR_CARRY;
      return;

    /* DOS 1+ - Get default drive */
    case 0x19:
      /* Return the default drive in AL (0='A', etc.) */
      r->h.al = fd32_get_default_drive() - 'A';
      LOG_PRINTF(("INT 21h - Getting default drive: %02x\n", r->h.al));
      RMREGS_CLEAR_CARRY;
      return;

    /* DOS 1+ - Set Disk Transfer Area address */
    case 0x1A:
      LOG_PRINTF(("INT 21h - Set Disk Transfer Area Address: %04x:%04x\n", r->x.ds, r->x.dx));
      /* DS:DX pointer to the new DTA */
      current_psp->dta = (void *) FAR2ADDR(r->x.ds, r->x.dx);
      return;

    /* DOS 1+ - GET SYSTEM DATE */
    case 0x2A:
      {
      fd32_date_t D;
      fd32_get_date(&D);
      r->x.cx = D.Year;
      r->h.dh = D.Mon;
      r->h.dl = D.Day;
      r->h.al = D.weekday;
      }
      RMREGS_CLEAR_CARRY;
      return;

    /* DOS 1+ - GET SYSTEM TIME */
    case 0x2C:
      {
      fd32_time_t T;
      fd32_get_time(&T);
      r->h.ch = T.Hour;
      r->h.cl = T.Min;
      r->h.dh = T.Sec;
      r->h.dl = T.Hund;
      }
      RMREGS_CLEAR_CARRY;
      return;

    /* DOS 2+ - Get Disk Transfer Area address */
    case 0x2F:
      /* ES:BX pointer to the current DTA */
      /* FIX ME: I assumed low memory DTA, I'm probably wrong! */
      r->x.es = (DWORD) current_psp->dta >> 4;
      r->x.bx = (DWORD) current_psp->dta & 0xF;
      LOG_PRINTF(("INT 21h - Get Disk Transfer Area Address: %04x:%04x\n", r->x.es, r->x.bx));
      return;

    /* DOS 2+ - Get DOS version */
    /* FIX ME: This call is subject to modification by SETVER */
    case 0x30:
      LOG_PRINTF(("INT 21h - Get DOS version\n"));
      /* AL what to return in BH: 0=OEM number, 1=version flag */
      if (r->h.al) r->h.bh = 0x00; /* DOS is not in ROM (bit 3) */
              else r->h.bh = 0xFD; /* We claim to be FreeDOS    */
      /* Set the 24-bit User Serial Number to zero as we don't use it */
      r->h.bl = 0;
      r->x.cx = 0;
      /* We claim to be 7.10 */
      r->x.ax = 0x0A07;
      RMREGS_CLEAR_CARRY;
      return;

    /* DOS 2+ - AL = 06h - Get ture DOS version */
    case 0x33:
      LOG_PRINTF(("INT 21h - Get true DOS version\n"));
      if (r->h.al == 0x06)
      {
        r->x.bx = 0x0A07; /* We claim to be 7.10     */
        r->x.dx = 0;      /* Revision 0, no flags... */
        RMREGS_CLEAR_CARRY;
        return;
      }
      /* TODO: MS-DOS returns AL=FFh for invalid subfunctions.     */
      /*       Make this behaviour consistent with other services. */
      r->h.al = 0xFF;
      RMREGS_SET_CARRY;
      return;
      
    /* DOS 2+ - Get free disk space */
    case 0x36:
    {
      /* DL    drive number (00h = default, 01h = A:, etc) */
      char             DriveSpec[4] = " :\\";
      fd32_getfsfree_t FSFree;
      DriveSpec[0] = fd32_get_default_drive();
      if (r->h.dl != 0x00) DriveSpec[0] = r->h.dl + 'A' - 1;
      FSFree.Size = sizeof(fd32_getfsfree_t);
      Res = fd32_get_fsfree(DriveSpec, &FSFree);
      LOG_PRINTF(("INT 21h - Getting free disk space for \"%s\", returns %08x\n", DriveSpec, Res));
      LOG_PRINTF(("          Got: %lu secperclus, %lu availclus, %lu bytspersec, %lu totclus\n",
                  FSFree.SecPerClus, FSFree.AvailClus, FSFree.BytesPerSec, FSFree.TotalClus));
      if (Res < 0)
      {
        r->x.ax = 0xFFFF; /* Unusual way to report error condition */
        return;
      }
      r->x.ax = FSFree.SecPerClus;  /* AX = Sectors per clusters */
      r->x.bx = FSFree.AvailClus;   /* BX = Number of free clusters */
      r->x.cx = FSFree.BytesPerSec; /* CX = Bytes per sector        */
      r->x.dx = FSFree.TotalClus;   /* DX = Total clusters on drive */
      return;
    }

    /* DOS 2+ - "MKDIR" - Create subdirectory */
    case 0x39:
      /* DS:DX pointer to the ASCIZ directory name */
      Res = make_sfn_path(SfnPath, (char *) FAR2ADDR(r->x.ds, r->x.dx));
      LOG_PRINTF(("INT 21h - Make directory \"%s\"\n", SfnPath));
      dos_return(Res, r);
      if (Res < 0) return;
      Res = fd32_mkdir(SfnPath);
      dos_return(Res, r);
      return;

    /* DOS 2+ - "RMDIR" - Remove subdirectory */
    case 0x3A:
      /* DS:DX pointer to the ASCIZ directory name */
      Res = make_sfn_path(SfnPath, (char *) FAR2ADDR(r->x.ds, r->x.dx));
      LOG_PRINTF(("INT 21h - Remove directory \"%s\"\n", SfnPath));
      dos_return(Res, r);
      if (Res < 0) return;
      Res = fd32_rmdir(SfnPath);
      dos_return(Res, r);
      return;

    /* DOS 2+ - "CHDIR" - Set current directory */
    case 0x3B:
      /* DS:DX pointer to the ASCIZ directory name */
      Res = make_sfn_path(SfnPath, (char *) FAR2ADDR(r->x.ds, r->x.dx));
      LOG_PRINTF(("INT 21h - Change directory to \"%s\"\n", SfnPath));
      dos_return(Res, r);
      if (Res < 0) return;
      Res = fd32_chdir(SfnPath);
      dos_return(Res, r);
      return;

    /* DOS 2+ - "CREAT" - Create or truncate file */
    case 0x3C:
      /* DS:DX pointer to the ASCIZ file name               */
      /* CX    attribute mask for the new file              */
      /*       (volume label and directory are not allowed) */
      Res = make_sfn_path(SfnPath, (char *) FAR2ADDR(r->x.ds, r->x.dx));
      dos_return(Res, r);
      LOG_PRINTF(("INT 21h - Creat \"%s\" with attr %04x\n", SfnPath, r->x.cx));
      if (Res < 0) return;
      Res = fd32_open(SfnPath,
                      FD32_ORDWR | FD32_OCOMPAT | FD32_OTRUNC | FD32_OCREAT,
                      r->x.cx,
                      0,   /* alias hint, not used   */
                      NULL /* action taken, not used */);
      if (Res < 0)
      {
        RMREGS_SET_CARRY;
        r->x.ax = (WORD) Res;
        return;
      }
      RMREGS_CLEAR_CARRY;
      r->x.ax = (WORD) Res; /* The new handle */
      return;

    /* DOS 2+ - "OPEN" - Open existing file */
    case 0x3D:
      /* DS:DX pointer to the ASCIZ file name                 */
      /* AL    opening mode                                   */
      /* CL    attribute mask for to look for (?server call?) */
      Res = make_sfn_path(SfnPath, (char *) FAR2ADDR(r->x.ds, r->x.dx));
      dos_return(Res, r);
      LOG_PRINTF(("INT 21h - Open \"%s\" with mode %02x\n", SfnPath, r->h.al));
      if (Res < 0) return;
      Res = fd32_open(SfnPath, FD32_OEXIST | r->h.al, FD32_ANONE,
                      0,   /* alias hint, not used   */
                      NULL /* action taken, not used */);
      if (Res < 0)
      {
        RMREGS_SET_CARRY;
        r->x.ax = (WORD) Res;
        return;
      }
      RMREGS_CLEAR_CARRY;
      r->x.ax = (WORD) Res; /* The new handle */
      return;

    /* DOS 2+ - "CLOSE" - Close file */
    case 0x3E:
      /* BX file handle */
      LOG_PRINTF(("INT 21h - Close handle %04x\n", r->x.bx));
      Res = fd32_close(r->x.bx);
      dos_return(Res, r); /* Recent DOSes preserve AH, that's OK for us */
      return;

    /* DOS 2+ - "READ" - Read from file or device */
    case 0x3F:
      /* BX    file handle                    */
      /* CX    number of bytes to read        */
      /* DS:DX pointer to the transfer buffer */
      Res = fd32_read(r->x.bx, (void *) FAR2ADDR(r->x.ds, r->x.dx), r->x.cx);
      //LOG_PRINTF(("INT 21h - Read (AH=3Fh) from handle %04x (%d bytes). Res=%08xh\n", r->x.bx, r->x.cx, Res));
      if (Res < 0)
      {
        RMREGS_SET_CARRY;
        r->x.ax = (WORD) Res;
        return;
      }
      #if 0
      /* This is a quick'n'dirty hack to return \n after \r from stdin.  */
      /* It works only if console input is on handle 0 and if the number */
      /* of bytes to read is greater than the first carriage return.     */
      /* TODO: Make this \r\n stuff functional according to the console. */
      if (r->x.bx == 0)
      {
        BYTE *Buf = (BYTE *) (r->x.ds << 4) + r->x.dx;
        if (Res < r->x.cx) Buf[Res++] = '\n';
      }
      #endif
      RMREGS_CLEAR_CARRY;
      r->x.ax = (WORD) Res; /* Return the number of bytes read in AX */
      return;

    /* DOS 2+ - "WRITE" - Write to file or device */
    case 0x40:
      /* BX    file handle                    */
      /* CX    number of bytes to write       */
      /* DS:DX pointer to the transfer buffer */
      Res = fd32_write(r->x.bx, (void *) FAR2ADDR(r->x.ds, r->x.dx), r->x.cx);
      //LOG_PRINTF(("INT 21h - Write (AH=40h) to handle %04x (%d bytes). Res=%08xh\n", r->x.bx, r->x.cx, Res));
      if (Res < 0)
      {
        RMREGS_SET_CARRY;
        r->x.ax = (WORD) Res;
        return;
      }
      RMREGS_CLEAR_CARRY;
      r->x.ax = (WORD) Res; /* Return the number of bytes written in AX */
      return;

    /* DOS 2+ - "UNLINK" - Delete file */
    case 0x41:
      /* DS:DX pointer to the ASCIZ file name                  */
      /* CL    attribute mask for deletion (?server call?)     */
      /*       FIX ME: check if they are required or allowable */
      Res = make_sfn_path(SfnPath, (char *) FAR2ADDR(r->x.ds, r->x.dx));
      LOG_PRINTF(("INT 21h - Delete file \"%s\"\n", SfnPath));
      dos_return(Res, r);
      if (Res < 0) return;
      Res = fd32_unlink(SfnPath, FD32_FAALL | FD32_FRNONE);
      dos_return(Res, r); /* In DOS 3.3 AL seems to be the file's drive */
      return;

    /* DOS 2+ - "LSEEK" - Set current file position */
    case 0x42:
      /* BX    file handle */
      /* CX:DX offset      */
      /* AL    origin      */
      /* FIXME: Replace with off_t */
      Res = fd32_lseek(r->x.bx, ((long long) r->x.cx << 8) + (long long) r->x.dx,
                       r->h.al, &llRes);
      if (Res < 0)
      {
        RMREGS_SET_CARRY;
        r->x.ax = (WORD) Res;
        return;
      }
      RMREGS_CLEAR_CARRY;
      /* Return the new position from start of file in DX:AX */
      r->x.ax = (WORD) llRes;
      r->x.dx = (WORD) (llRes >> 16);
      return;

    /* DOS 2+ - Get/set file attributes */
    case 0x43:
      dos_get_and_set_attributes(r);
      return;

    /* IOCTL functions - FIX ME: THIS MUST BE COMPLETELY DONE! */
    case 0x44:
      Res = fd32_get_dev_info(r->x.bx);
      if (Res < 0)
      {
        r->x.ax = (WORD)FD32_EBADF;
        RMREGS_SET_CARRY;
      }
      else
      {
        r->x.ax = Res;
        r->x.dx = r->x.ax;
        RMREGS_CLEAR_CARRY;
      }
      return;
/*
 * Warning!!!
 * This is clearly an hack!!!
 * The correct thing to do would be:
 * 1) Call the kernel
 * 2) The kernel converts the handle (BX) in a file using the JFT of the
 *    current task
 * 3) The kernel checks if it is a device, and so on... And returns the
 *    correct value
 */
      /* This should be GET DEVICE INFORMATION... */
      /* Hrm... We should look at al... See the FD DosDevIO (ioctl.c) */
      if ((r->x.bx == 0) || (r->x.bx == 1) || (r->x.bx == 2)) {
        /* Stdout & Stderr are TTY devices... Hope this is the
         * right way to say it... 
         */
        r->x.ax = 0x83;
        /* Ok... Got it:
         * 0x80 = File is a Device
         * 0x02 = Device is Console Output
         * 0x01 = Device is Console Input
         */
        RMREGS_CLEAR_CARRY;
        return;
      }
      r->x.ax = (WORD) FD32_EBADF;
      RMREGS_SET_CARRY;
      return;

    /* DOS 2+ - "DUP" - Duplicate file handle */
    case 0x45:
      /* BX file handle */
      Res = fd32_dup(r->x.bx);
      if (Res < 0)
      {
        RMREGS_SET_CARRY;
        r->x.ax = (WORD) Res;
        return;
      }
      RMREGS_CLEAR_CARRY;
      r->x.ax = (WORD) Res; /* Return the new handle in AX */
      return;

    /* DOS 2+ - "DUP2", "FORCEDUP" - Force duplicate file handle */
    case 0x46:
      /* BX file handle */
      /* CX new handle  */
      Res = fd32_forcedup(r->x.bx, r->x.cx);
      dos_return(Res, r);
      return;

    /* DOS 2+ - "CWD" - Get current directory */
    case 0x47:
    {
      /* DL    drive number (00h = default, 01h = A:, etc) */
      /* DS:SI 64-byte buffer for ASCIZ pathname           */
      char  Drive[2] = "\0\0";
      char  Cwd[FD32_LFNPMAX];
      char *c;
      char *Dest = (char *) FAR2ADDR(r->x.ds, r->x.si);

      LOG_PRINTF(("INT 21h - Getting current dir of drive %02x\n", r->h.dl));
      Drive[0] = fd32_get_default_drive();
      if (r->h.dl != 0x00) Drive[0] = r->h.dl + 'A' - 1;
      fd32_getcwd(Drive, Cwd);
      LOG_PRINTF(("After getcwd:\"%s\"\n", Cwd));
      fd32_sfn_truename(Cwd, Cwd); /* Convert all component to 8.3 format */
      LOG_PRINTF(("After sfn_truename:\"%s\"\n", Cwd));
      /* Skip drive specification and initial backslash as required from DOS */
      for (c = Cwd; *c != '\\'; c++);
      strcpy(Dest, c + 1);
      LOG_PRINTF(("Returned CWD:\"%s\"\n", Dest));
      r->x.ax = 0x0100; /* Undocumented success return value, from RBIL */
      /* TODO: Convert from UTF-8 to OEM */
      return;
    }

    /* DOS 2+ - "EXEC" - Load and/or Execute program */
    case 0x4B:
    {
      tExecParams *pb;
      char *args, *p, c[128];
      int cnt, i;
      
      pb = (tExecParams *) FAR2ADDR(r->x.es, r->x.bx);
      /* Only AH = 0x00 - Load and Execute - is currently supported */
      if (r->h.al != 0)
      {
        dos_return(FD32_EINVAL, r);
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
      Res = make_sfn_path(SfnPath, (char *) FAR2ADDR(r->x.ds, r->x.dx));
      LOG_PRINTF(("INT 21h - Load and Execute \"%s\"\n", SfnPath));
      dos_return(Res, r);
      Res = dos_exec(SfnPath, pb->Env, args, pb->Fcb1, pb->Fcb2, &DosReturnCode);
    }
    dos_return(Res, r);
    return;
    
    /* DOS 2+ - Get return code */
    case 0x4D:
      RMREGS_CLEAR_CARRY;
      r->h.ah = 0; /* Only normal termination, for now...*/
      r->h.al = DosReturnCode;
      DosReturnCode = 0; /* DOS clears this after reading it */
      return;

    /* DOS 2+ - "FINDFIRST" - Find first matching file */
    case 0x4E:
      /* AL    special flag for APPEND (ignored)                   */
      /* CX    allowable attributes (archive and readonly ignored) */
      /* DS:DX pointer to the ASCIZ file specification             */
      Res = make_sfn_path(SfnPath, (char *) FAR2ADDR(r->x.ds, r->x.dx));
      LOG_PRINTF(("INT 21h - Findfirst \"%s\" with attr %04x\n", SfnPath, r->x.cx));
      dos_return(Res, r);
      Res = fd32_dos_findfirst(SfnPath, r->x.cx, current_psp->dta);
      dos_return(Res, r);
      return;

    /* DOS 2+ - "FINDNEXT" - Find next matching file */
    case 0x4F:
      LOG_PRINTF(("INT 21h - Findnext\n"));
      Res = fd32_dos_findnext(current_psp->dta);
      dos_return(Res, r);
      return;

    /* DOS 2+ - "RENAME" - Rename or move file */
    case 0x56:
    {
      char SfnPathNew[FD32_LFNPMAX];
      /* DS:DX pointer to the ASCIZ name of the file to be renamed */
      /* ES:DI pointer to the ASCIZ new name for the file          */
      /* CL    attribute mask for server call (ignored)            */
      Res = make_sfn_path(SfnPath, (char *) FAR2ADDR(r->x.ds, r->x.dx));
      dos_return(Res, r);
      if (Res < 0) return;
      Res = make_sfn_path(SfnPathNew, (char *) FAR2ADDR(r->x.es, r->x.di));
      dos_return(Res, r);
      LOG_PRINTF(("INT 21h - Renaming \"%s\" to \"%s\"\n", SfnPath, SfnPathNew));
      if (Res < 0) return;
      Res = fd32_rename(SfnPath, SfnPathNew);
      dos_return(Res, r);
      return;
    }

    /* DOS 2+ - Get/set file's time stamps */
    case 0x57:
      get_and_set_time_stamps(r);
      return;

    /* DOS 3.0+ - Create new file */
    case 0x5B:
      /* CX    attribute mask for the new file */
      /* DS:DX pointer to the ASCIZ file name  */
      Res = make_sfn_path(SfnPath, (char *) FAR2ADDR(r->x.ds, r->x.dx));
      LOG_PRINTF(("INT 21h - Creating new file (AH=5Bh) \"%s\" with attr %04x\n", SfnPath, r->x.cx));
      dos_return(Res, r);
      if (Res < 0) return;
      Res = fd32_open(SfnPath, FD32_ORDWR | FD32_OCOMPAT | FD32_OCREAT, r->x.cx, 0, NULL);
      if (Res < 0)
      {
        RMREGS_SET_CARRY;
        r->x.ax = (WORD) Res;
        return;
      }
      RMREGS_CLEAR_CARRY;
      r->x.ax = (WORD) Res; /* The new handle */
      return;

    case 0x62:
      LOG_PRINTF(("INT 21h - Get current PSP address\n"));
      r->x.bx = (DWORD) current_psp >> 4;
      return;

    /* DOS 3.3+ - "FFLUSH" - Commit file   */
    /* DOS 4.0+ Undocumented - Commit file */
    case 0x68:
    case 0x6A:
      LOG_PRINTF(("INT 21h - fflush\n"));
      /* BX file handle */
      Res = fd32_fflush(r->x.bx);
      dos_return(Res, r);
      return;

    /* DOS 5+ Undocumented - Null function */
    case 0x6B:
      LOG_PRINTF(("INT 21h - Null function. Hey, somebody calls it!\n"));
      r->h.al = 0;
      return;

    /* DOS 4.0+ - Extended open/create file */
    case 0x6C:
    {
      int Action;
      /* AL    00h                                                         */
      /* BL    access and sharing mode                                     */
      /* BH    flags: bit 6 = commit at every write                        */
      /*              bit 5 = return error rather than doing INT 24h       */
      /*              bit 4 = allow 4 GiB files on FAT32 volumes (ignored) */
      /* CX    attribute mask for file creation                            */
      /* DL    action: bit 4 = create file if does not exist               */
      /*               bit 1 = replace file if exists                      */
      /*               bit 0 = open file if exists                         */
      /* DH    reserved, must be 00h                                       */
      /* DS:SI pointer to the ASCIZ file name                              */
      Res = make_sfn_path(SfnPath, (char *) FAR2ADDR(r->x.ds, r->x.si));
      dos_return(Res, r);
      if (Res < 0) return;
      Res = fd32_open(SfnPath, ((DWORD) r->x.dx << 16) + (DWORD) r->x.bx, r->x.cx, 0, &Action);
      LOG_PRINTF(("INT 21h - Extended open/create (AH=6Ch) \"%s\" res=%08xh\n", SfnPath, Res));
      if (Res < 0)
      {
        RMREGS_SET_CARRY;
        r->x.ax = (WORD) Res;
        return;
      }
      RMREGS_CLEAR_CARRY;
      r->x.ax = (WORD) Res;    /* The new handle */
      r->x.cx = (WORD) Action; /* Action taken   */
      return;
    }

    /* Long File Name functions */
    case 0x71:
      if (use_lfn) lfn_functions(r);
      else
      {
        r->x.ax = 0x7100;
        RMREGS_SET_CARRY;
      }
      return;

    /* Windows 95 beta - "FindClose" - Terminate directory search */
    case 0x72:
      /* BX directory handle (FIX ME: This is not documented anywhere!) */
      Res = fd32_lfn_findclose(r->x.bx);
      dos_return(Res, r);
      return;
      
    /* Windows 95 FAT32 services */
    /* By now only AX=7303h "Get extended free space on drive" is supported */
    case 0x73:
    {
      tExtDiskFree     *Edf;
      fd32_getfsfree_t  FSFree;
      LOG_PRINTF(("INT 21h - FAT32 service %02x issued\n", r->h.al));
      if (r->h.al != 0x03)
      {
        r->h.al = 0xFF; /* Unimplemented subfunction */
        RMREGS_SET_CARRY;
      }
      /* DS:DX ASCIZ string for drive ("C:\" or "\\SERVER\Share")      */
      /* ES:DI Buffer for extended free space structure (tExtFreSpace) */
      /* CX    Length of buffer for extended free space                */
      if (r->x.cx < sizeof(tExtDiskFree))
      {
        dos_return(FD32_EFORMAT, r);
        return;
      }
      Edf = (tExtDiskFree *) FAR2ADDR(r->x.es, r->x.di);
      if (Edf->Version != 0x0000)
      {
        dos_return(FD32_EFORMAT, r);
        return;
      }
      FSFree.Size = sizeof(fd32_getfsfree_t);
      Res = fd32_get_fsfree((char *) FAR2ADDR(r->x.ds, r->x.dx), &FSFree);
      if (Res >= 0)
      {
        Edf->Size = sizeof(tExtDiskFree);
        /* Currently we don't support volume compression */
        Edf->SecPerClus  = FSFree.SecPerClus;
        Edf->BytesPerSec = FSFree.BytesPerSec;
        Edf->AvailClus   = FSFree.AvailClus;
        Edf->TotalClus   = FSFree.TotalClus;
        Edf->RealSecPerClus  = FSFree.SecPerClus;
        Edf->RealBytesPerSec = FSFree.BytesPerSec;
        Edf->RealAvailClus   = FSFree.AvailClus;
        Edf->RealTotalClus   = FSFree.TotalClus;
      }
      dos_return(Res, r);
      return;
    }

    /* Unsupported or invalid functions */
    default:
      RMREGS_SET_CARRY;
      r->x.ax = (WORD) FD32_EINVAL;
      return;
  }
}
