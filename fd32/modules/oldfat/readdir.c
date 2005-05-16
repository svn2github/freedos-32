/**************************************************************************
 * FreeDOS 32 FAT Driver                                                  *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2005, Salvatore Isaja                               *
 *                                                                        *
 * This is "readdir.c" - Services to find a specified directory entry     *
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
#include <dr-env.h>

#include <unicode/unicode.h>
#include "fat.h"

/* Define the DEBUG symbol in order to activate driver's log output */
#ifdef DEBUG
 #define LOG_PRINTF(s) fd32_log_printf s
#else
 #define LOG_PRINTF(s)
#endif

/* To fetch a LFN we use a circular buffer of directory entries.       */
/* A long file name is up to 255 characters long, so 20 entries are    */
/* needed, plus one additional entry for the short name alias. We scan */
/* the directory and place the entry that we read into the Buffer at   */
/* the offset BufferPos. When we find a valid short name, we backtrace */
/* the Buffer with LfnSlot to extract the long file name, if present.  */
#define LFN_FETCH_SLOTS 21

/* Fetches a long file name before a short name entry, if present, from the */
/* specified Buffer of directory entries to a 16-bit character string.      */
/* Returns the number of directory entries used by the long name, or 0 if   */
/* no long name is present before the short name entry.                     */
/* Called by fat_readdir and find.                                          */
static int fetch_lfn(tDirEntry *Buffer, int BufferPos, WORD *Name)
{
  tLfnEntry *LfnSlot  = (tLfnEntry *) &Buffer[BufferPos];
  BYTE       Checksum = lfn_checksum(&Buffer[BufferPos]);
  int        Order    = 0;
  int        NamePos  = 0;
  int        k;

  do
  {
    if (--LfnSlot < (tLfnEntry *) Buffer) LfnSlot += LFN_FETCH_SLOTS;
    if (LfnSlot->Attr != FD32_ALNGNAM) return 0;
    if (++Order != (LfnSlot->Order & 0x1F)) return 0;
    if (Checksum != LfnSlot->Checksum) return 0;
    /* Ok, the LFN slot is valid, attach it to the long name */
    for (k = 0; k < 5; k++) Name[NamePos++] = LfnSlot->Name0_4[k];
    for (k = 0; k < 6; k++) Name[NamePos++] = LfnSlot->Name5_10[k];
    for (k = 0; k < 2; k++) Name[NamePos++] = LfnSlot->Name11_12[k];
  }
  while (!(LfnSlot->Order & 0x40));
  if (Name[NamePos - 1] != 0x0000) Name[NamePos] = 0x0000;
  return Order;
}

/* Temporary function */
static int utf16to8(const WORD *restrict source, char *restrict dest)
{
	wchar_t wc;
	int res;
	while (*source)
	{
		res = unicode_utf16towc(&wc, source, 2);
		if (res < 0) return res;
		source += res;
		res = unicode_wctoutf8(dest, wc, 6);
		if (res < 0) return res;
		dest += res;
	}
	return 0;
}

/* Searches an open directory for files matching the file specification */
/* and the search flags.                                                */
/* On success, returns 0, fills the FindData structure.                 */
/* Returns a negative error code on failure.                            */
static int readdir(tFile *Dir, tFatFind *Ff)
{
  tDirEntry  Buffer[LFN_FETCH_SLOTS];
  int        BufferPos = 0;
  int        NumRead;
  WORD       LongNameUtf16[FD32_LFNMAX];

  if (!(Dir->DirEntry.Attr & FD32_ADIR)) return -EBADF;

  NumRead = fat_read(Dir, &Buffer[0], 32);
  if (NumRead < 0) return NumRead;
  while (NumRead > 0)
  {
    /* If the directory name starts with FAT_DIR_ENFOFDIR */
    /* we don't need to scan further.                     */
    if (Buffer[BufferPos].Name[0] == ENDOFDIR) return -ENOENT;

    /* If the entry is marked as deleded we skip to the next entry */
    if (Buffer[BufferPos].Name[0] != FREEENT)
    {
      /* If we find a file without the Long Name attribute, it is a valid */
      /* short name. We then try to extract a long name before it.        */
      if (Buffer[BufferPos].Attr != FD32_ALNGNAM)
      {
        Ff->SfnEntry    = Buffer[BufferPos];
        Ff->EntryOffset = Dir->TargetPos - NumRead;
        fat_expand_fcb_name(Dir->V->nls, Ff->ShortName, Buffer[BufferPos].Name, sizeof(Ff->ShortName)); /* was from the FS layer */
        #ifdef FATLFN
        Ff->LfnEntries = fetch_lfn(Buffer, BufferPos, LongNameUtf16);
        if (Ff->LfnEntries)
        {
          int res = utf16to8(LongNameUtf16, Ff->LongName);
          if (res < 0) return res;
        }
        else strcpy(Ff->LongName, Ff->ShortName);
        #else
        Ff->LfnEntries  = 0;
        strcpy(Ff->LongName, Ff->ShortName);
        #endif
        LOG_PRINTF(("Found: %-14s%s\n", Ff->ShortName, Ff->LongName));
        return 0;
      }
    }
    if (++BufferPos == LFN_FETCH_SLOTS) BufferPos = 0;
    NumRead = fat_read(Dir, &Buffer[BufferPos], 32);
    if (NumRead < 0) return NumRead;
  } /* while (NumRead > 0) */
  return -ENOENT;
}


/* The READDIR system call.                                    */
/* Reads the next directory entry of the open directory Dir in */
/* the passed LFN finddata structure.                          */
/* Returns 0 on success, or a negative error code on failure.  */
int fat_readdir(tFile *Dir, fd32_fs_lfnfind_t *Entry)
{
  tFatFind F;
  int      Res;

  if (!Entry) return -EINVAL;
  if ((Res = readdir(Dir, &F)) < 0) return Res;

  Entry->Attr   = F.SfnEntry.Attr;
  Entry->CTime  = F.SfnEntry.CrtTime + (F.SfnEntry.CrtDate << 16);
  Entry->ATime  = F.SfnEntry.LstAccDate << 16;
  Entry->MTime  = F.SfnEntry.WrtTime + (F.SfnEntry.WrtDate << 16);
  Entry->SizeHi = 0;
  Entry->SizeLo = F.SfnEntry.FileSize;
  ((tFindRes *) Entry->Reserved)->EntryCount = Dir->TargetPos >> 5;
  ((tFindRes *) Entry->Reserved)->FirstDirCluster = FIRSTCLUSTER(Dir->DirEntry);
  strcpy(Entry->LongName, F.LongName);
  strcpy(Entry->ShortName, F.ShortName);
  return 0;
}

/* Compares two UTF-8 strings, disregarding case.
 * Returns 0 if the strings match, a positive value if they don't match,
 * or a -EILSEQ if an invalid UTF-8 sequence is found.
 */
static int utf8_stricmp(const char *s1, const char *s2)
{
	wchar_t wc1, wc2;
	int res;
	for (;;)
	{
		if (!(*s2 & 0x80))
		{
			if (toupper(*s1) != toupper(*s2)) return 1;
			if (*s1 == 0) return 0;
			s1++;
			s2++;
		}
		else
		{
			res = unicode_utf8towc(&wc1, s1, 6);
			if (res < 0) return res;
			s1 += res;
			res = unicode_utf8towc(&wc2, s2, 6);
			if (res < 0) return res;
			s2 += res;
			if (unicode_simple_fold(wc1) != unicode_simple_fold(wc2)) return 1;
		}
	}
}

/* Searches an open directory for files matching the file specification and   */
/* the search flags.                                                          */
/* On success, returns 0 and fills the passed find data structure (optional). */
/* Returns a negative error code on failure.                                  */
int fat_find(tFile *Dir, char *FileSpec, DWORD Flags, tFatFind *Ff)
{
  int      Res;
  tFatFind TempFf;
  BYTE     AllowableAttr = (BYTE) Flags;
  BYTE     RequiredAttr  = (BYTE) (Flags >> 8);

  while ((Res = readdir(Dir, &TempFf)) == 0)
    if (((AllowableAttr | TempFf.SfnEntry.Attr) == AllowableAttr)
     && ((RequiredAttr & TempFf.SfnEntry.Attr) == RequiredAttr))
    {
      #ifdef FATLFN
      if (utf8_stricmp(TempFf.LongName, FileSpec) == 0)
      {
        if (Ff) memcpy(Ff, &TempFf, sizeof(tFatFind));
        return 0;
      }
      #endif
      if (utf8_stricmp(TempFf.ShortName, FileSpec) == 0)
      {
        if (Ff) memcpy(Ff, &TempFf, sizeof(tFatFind));
        return 0;
      }
    }
  return Res;
}


/* Backend for the DOS-style FINDFIRST and FINDNEXT system calls.   */
/* Searches for the search template specified in the buffer in      */
/* an open directory "f", then closes the directory.                */
/* On success, returns 0 and fills the output fields of the buffer. */
/* On failure, returns a negative error code.                       */
static int dos_find(tFile *f, fd32_fs_dosfind_t *find_data)
{
  tFatFind ff;
  int res;

  while ((res = readdir(f, &ff)) == 0)
  {
    /* According to the RBIL, if search attributes are not 08h (volume  */
    /* label) all files with at most the specified attributes should be */
    /* returned (archive and readonly are always returned), otherwise   */
    /* only the volume label should be returned.                        */
    if (find_data->SearchAttr != FD32_AVOLID)
    {
      BYTE a = find_data->SearchAttr | FD32_AARCHIV | FD32_ARDONLY;
      if ((a | ff.SfnEntry.Attr) != a) continue;
    }
    else
    {
      if (ff.SfnEntry.Attr != FD32_AVOLID) continue;
    }
    /* Now that attributes match, compare the file names */
    if (fat_compare_fcb_names(ff.SfnEntry.Name, find_data->SearchTemplate)) continue;
    /* Ok, we have found the file, fill the output fields of "find_data" */
    find_data->Attr  = ff.SfnEntry.Attr;
    find_data->MTime = ff.SfnEntry.WrtTime;
    find_data->MDate = ff.SfnEntry.WrtDate;
    find_data->Size  = ff.SfnEntry.FileSize;
    strcpy(find_data->Name, ff.ShortName);
    ((tFindRes *) find_data->Reserved)->EntryCount = f->TargetPos >> 5;
    ((tFindRes *) find_data->Reserved)->FirstDirCluster = FIRSTCLUSTER(f->DirEntry);
    return fat_close(f);
  }
  fat_close(f);
  return res;
}


/* The DOS-style FINDFIRST system call */
/* TODO: Remove useless string copy */
int fat_findfirst(tVolume *v, const char *path, int attributes, fd32_fs_dosfind_t *find_data)
{
  char dir[FD32_LFNPMAX];
  char name[FD32_LFNPMAX];
  int res;
  tFile *f;
  find_data->SearchAttr = attributes;
  fat_split_path(path, dir, name);
  res = fat_build_fcb_name(v->nls, find_data->SearchTemplate, name);
  if (res < 0) return res;
  res = fat_open(v, dir, O_RDONLY | O_DIRECTORY, FD32_ANONE, 0, &f);
  if (res < 0) return res;
  return dos_find(f, find_data);
}


/* The DOS-style FINDNEXT system call */
int fat_findnext(tVolume *v, fd32_fs_dosfind_t *find_data)
{
  tFile *f;
  int res = fat_reopendir(v, (tFindRes *) find_data->Reserved, &f);
  if (res < 0) return res;
  return dos_find(f, find_data);
}


/* TODO: enable search attributes/flags  */
int fat_findfile(tFile *f, const char *name, fd32_fs_lfnfind_t *find_data)
{
  int res;
  while ((res = fat_readdir(f, find_data)) == 0)
//    if (((AllowableAttr | FindData->Attr) == AllowableAttr)
//     && ((RequiredAttr & FindData->Attr) == RequiredAttr))
      if ((fat_fnameicmp(find_data->LongName, name) == 0)
       || (fat_fnameicmp(find_data->ShortName, name) == 0))
        return 0;
  return res;
}
