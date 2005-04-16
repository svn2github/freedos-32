/**************************************************************************
 * FreeDOS 32 FAT Driver                                                  *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2005, Salvatore Isaja                               *
 *                                                                        *
 * This is "lfn.c" - Convert long file names in the standard DOS          *
 *                   directory entry format and generate short name       *
 *                   aliases from long file names                         *
 *                                                                        *
 *                                                                        *
 * This file is part of the FreeDOS 32 FAT Driver.                        *
 *                                                                        *
 * The FreeDOS 32 FAT Driver is free software; you can redistribute it    *
 * and/or modify it under the terms of the GNU General Public License     *
 * as published by the Free Software Foundation; either version 2 of the  *
 * License, or (at your option) any later version.                        *
 *                                                                        *
 * The FreeDOS 32 FAT Driver is distributed in the hope that it will be   *
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with the FreeDOS 32 FAT Driver; see the file COPYING;            *
 * if not, write to the Free Software Foundation, Inc.,                   *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA                *
 **************************************************************************/

#include "fat.h"
#include <unicode.h>

/* Define the DEBUG symbol in order to activate driver's log output */
#ifdef DEBUG
 #define LOG_PRINTF(s) fd32_log_printf s
#else
 #define LOG_PRINTF(s)
#endif


/* From NLS support */
int oemcp_to_utf8(char *Source, UTF8 *Dest);
int utf8_to_oemcp(UTF8 *Source, int SourceSize, char *Dest, int DestSize);
int oemcp_skipchar(char *Dest);


/* Calculate the 8-bit checksum for the long file name from its */
/* corresponding short name.                                    */
/* Called by split_lfn and find (find.c).                       */
BYTE lfn_checksum(tDirEntry *D)
{
  int Sum = 0, i;
  for (i = 0; i < 11; i++)
  {
    Sum = (((Sum & 1) << 7) | ((Sum & 0xFE) >> 1)) + D->Name[i];
  }
  return Sum;
}


#ifdef FATWRITE
#if 0
/* Returns nonzero if a UTF-8 string is a valid FAT long file name. */
/* Called by allocate_lfn_dir_entries (direntry.c).                 */
int lfn_is_valid(char *s)
{
  for (; *s; s++)
  {
    if (Ch < 0x20) return 0;
    switch (Ch)
    {
      /* + , ; = [ ] . are not valid for short names but are ok for LFN */
      case 0x22 : /* " */
      case 0x2A : /* * */
      case 0x2F : /* / */
      case 0x3A : /* : */
      case 0x3C : /* < */
      case 0x3E : /* > */
      case 0x3F : /* ? */
      case 0x5C : /* \ */
      case 0x7C : /* | */ return 0;
    }
  }
  return 1;
}
#endif


/* Generates a valid 8.3 file name from a valid long file name. */
/* The generated short name has not a numeric tail.             */
/* Returns 0 on success, or a negative error code on failure.   */
/* NOTE: Pasted from fd32/filesys/names.c (that has to be removed) */
static int gen_short_fname1(char *Dest, char *Source, DWORD Flags)
{
  BYTE  ShortName[11] = "           ";
  char  Purified[FD32_LFNPMAX]; /* A long name without invalid 8.3 characters */
  char *DotPos = NULL; /* Position of the last embedded dot in Source */
  char *s;
  int   Res = 0;

  /* Find the last embedded dot, if present */
  if (!(*Source)) return FD32_EFORMAT;
  for (s = Source + 1; *s; s++)
    if ((*s == '.') && (*(s-1) != '.') && (*(s+1) != '.') && *(s+1))
      DotPos = s;

  /* Convert all characters of Source that are invalid for short names */
  for (s = Purified; *Source;)
    if (!(*Source & 0x80))
    {
      if ((*Source >= 'a') && (*Source <= 'z'))
      {
        *s++ = *Source + 'A' - 'a';
        Res |= FD32_GENSFN_CASE_CHANGED;
      }
      else if (*Source < 0x20) return FD32_EFORMAT;
      else switch (*Source)
      {
        /* Spaces and periods must be removed */
        case ' ': break;
        case '.': if (Source == DotPos) DotPos = s, *s++ = '.';
                                   else Res |= FD32_GENSFN_WAS_INVALID;
                  break;
        /* + , ; = [ ] are valid for LFN but not for short names */
        case '+': case ',': case ';': case '=': case '[': case ']':
          *s++ = '_'; Res |= FD32_GENSFN_WAS_INVALID; break;
        /* Check for invalid LFN characters */
        case '"': case '*' : case '/': case ':': case '<': case '>':
        case '?': case '\\': case '|':
          return FD32_EFORMAT;
        /* Return any other single-byte character unchanged */
        default : *s++ = *Source;
      }
      Source++;
    }
    else /* Process extended characters */
    {
      UTF32 Ch, upCh;
      int   Skip;

      if ((Skip = fd32_utf8to32(Source, &Ch)) < 0) return FD32_EUTF8;
      Source += Skip;
      upCh = unicode_toupper(Ch);
      if (upCh != Ch) Res |= FD32_GENSFN_CASE_CHANGED;
      if (upCh < 0x80) Res |= FD32_GENSFN_WAS_INVALID;
      s += fd32_utf32to8(upCh, s);
    }
  *s = 0;

  /* Convert the Purified name to the OEM code page in FCB format */
  /* TODO: Must report WAS_INVALID if an extended char maps to ASCII! */
  if (utf8_to_oemcp(Purified, DotPos ? DotPos - Purified : -1, ShortName, 8))
    Res |= FD32_GENSFN_WAS_INVALID;
  if (DotPos) if (utf8_to_oemcp(DotPos + 1, -1, &ShortName[8], 3))
                Res |= FD32_GENSFN_WAS_INVALID;
  if (ShortName[0] == ' ') return FD32_EFORMAT;
  if (ShortName[0] == 0xE5) Dest[0] = (char) 0x05;

  /* Return the generated short name in the specified format */
  switch (Flags & FD32_GENSFN_FORMAT_MASK)
  {
    case FD32_GENSFN_FORMAT_FCB    : memcpy(Dest, ShortName, 11);
                                     return Res;
    case FD32_GENSFN_FORMAT_NORMAL : fat_expand_fcb_name(Dest, ShortName); /* was from the FS layer */
                                     return Res;
    default                        : return FD32_EINVAL;
  }
}


/* Compares two file names in FCB format. Name2 may contain '?' wildcards. */
/* Returns zero if the names match, or nonzero if they don't match.        */
/* TODO: Works only on single byte character sets!                       */
/* NOTE: Pasted from fd32/filesys/names.c (that has to be removed) */
int fat_compare_fcb_names(BYTE *Name1, BYTE *Name2)
{
  int k;
  for (k = 0; k < 11; k++)
  {
    if (Name2[k] == '?') continue;
    if (Name1[k] != Name2[k]) return -1;
  }
  return 0;
}


/* Generate a valid 8.3 file name for a long file name, and makes sure */
/* the generated name is unique in the specified directory appending a */
/* "~Counter" if necessary.                                            */
/* Returns 0 if the passed long name is already a valid 8.3 file name  */
/* (including upper case), thus no extra directory entry is required.  */
/* Returns a positive number on successful generation of a short name  */
/* alias for a long file name which is not a valid 8.3 name.           */
/* Returns a negative error code on failure.                           */
/* Called by allocate_lfn_dir_entries (direntry.c).                    */
int gen_short_fname(tFile *Dir, char *LongName, BYTE *ShortName, WORD Hint)
{
  BYTE       Aux[11];
  BYTE       szCounter[6];
  int        Counter, szCounterLen;
  int        k, Res;
  char      *s;
  tDirEntry  E;

  LOG_PRINTF(("Generating unique short file name for \"%s\"\n", LongName));
  Res = gen_short_fname1(ShortName, LongName, FD32_GENSFN_FORMAT_FCB); /* was from FS layer */
  LOG_PRINTF(("gen_short_fname1 returned %08x\n", Res));
  if (Res <= 0) return Res;
  /* TODO: Check case change! */
  if (!(Res & FD32_GENSFN_WAS_INVALID)) return 1;

  /* Now append "~Counter" to the short name, incrementing "Counter" */
  /* until the file name is unique in that Path                      */
  for (Counter = Hint; Counter < 65536; Counter++)
  {
    memcpy(Aux, ShortName, 11);
    /* TODO: it would be better: snprintf(szCounter, sizeof(szCounter), "%d", Counter); */
    ksprintf(szCounter, "%d", Counter);
    szCounterLen = strlen(szCounter);
    /* TODO: This may not work properly with multibyte charsets, but this is not
     *       a problem for now, since the whole NLS stuff has to be replaced. */
    for (k = 0; (k < 7 - szCounterLen) && (Aux[k] != ' '); k += oemcp_skipchar(&Aux[k]));
    Aux[k++] = '~';
    for (s = szCounter; *s; Aux[k++] = *s++);

    /* Search for the generated name */
    LOG_PRINTF(("Checking if ~%i makes the name unique\n", Counter));
    Dir->TargetPos = 0;
    for (;;)
    {
      if ((Res = fat_read(Dir, &E, sizeof(tDirEntry))) < 0) return Res;
      if ((Res == 0) || (E.Name[0] == 0))
      {
        memcpy(ShortName, Aux, 11);
        return 1;
      }
      if (fat_compare_fcb_names(E.Name, Aux) == 0) break; /* was from the FS layer */
    }
  }
  return FD32_EACCES;
}
#endif /* #ifdef FATWRITE */


/* Gets a UTF-8 short file name from an FCB file name. */
/* NOTE: Pasted from fd32/filesys/names.c (that has to be removed) */
int fat_expand_fcb_name(char *Dest, BYTE *Source)
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
