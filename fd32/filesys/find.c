/**************************************************************************
 * FreeDOS32 File System Layer                                            *
 * Wrappers for file system driver functions and JFT support              *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2002-2003, Salvatore Isaja                               *
 *                                                                        *
 * This is "find.c" - DOS and LFN style findfirst/findnext services.      *
 *                                                                        *
 *                                                                        *
 * This file is part of the FreeDOS32 File System Layer (the SOFTWARE).   *
 *                                                                        *
 * The SOFTWARE is free software; you can redistribute it and/or modify   *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation; either version 2 of the License, or (at  *
 * your option) any later version.                                        *
 *                                                                        *
 * The SOFTWARE is distributed in the hope that it will be useful, but    *
 * WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with the SOFTWARE; see the file GPL.txt;                         *
 * if not, write to the Free Software Foundation, Inc.,                   *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA                *
 **************************************************************************/

#include <dr-env.h>
#include <filesys.h>
#include <errors.h>
#include <unicode.h>
#include "fspriv.h"


/* Given a handle, checks if it's valid for directory searches (in    */
/* valid range, open and with a search name defined.                  */
/* Returns a pointer to the JFT entry on success, or NULL on failure. */
static inline tJft *validate_search_handle(int Handle)
{
  int   JftSize;
  tJft *Jft = (tJft *) fd32_get_jft(&JftSize);

  if ((Handle < 0) || (Handle >= JftSize)) return NULL;
  if (Jft[Handle].request == NULL) return NULL;
  if (Jft[Handle].SearchName == NULL) return NULL;
  return &Jft[Handle];
}


/* Splits a full valid path name (path + file name) into */
/* its path and file name components.                    */
/* Called by fd32_dos_findfirst, fd32_lfn_findfirst.     */
static void split_path(char *FullPath, char *Path, char *Name)
{
  char *NameStart;
  char *s;
  
  /* Move to the end of the FullPath string */
  for (s = FullPath; *s != 0; s++);
  /* Now move backward until '\' is found. The name starts next the '\' */
  for (; (*s != '\\') && (s != FullPath); s--);
  if (*s == '\\') s++;
  /* And copy the string from this point until the end into Name */
  for (NameStart = s; *s != 0; s++, Name++) *Name = *s;
  *Name = 0;
  /* Finally we copy from the beginning to the filename */
  /* into Path, with the trailing slash.                */
  for (s = FullPath; s < NameStart; s++, Path++) *Path = *s;
  *Path = 0;
}


/* Searches for the search template specified in the Dta buffer in */
/* an open directory Dir, then closes the directory.               */
/* On success, returns 0 and fills the output fields of the Dta.   */
/* On failure, returns a negative error code.                      */
/* Called by fd32_dos_findfirst, fd32_dos_findnext.                */
static int dos_find(fd32_request_t *request, void *DirId, fd32_fs_dosfind_t *Dta)
{
  int               Res;
  fd32_fs_lfnfind_t F;
  BYTE              FoundName[11];
  fd32_readdir_t    Rd;
  fd32_close_t      C;

  Rd.Size  = sizeof(fd32_readdir_t);
  Rd.DirId = DirId;
  Rd.Entry = &F;
  C.Size     = sizeof(fd32_close_t);
  C.DeviceId = DirId;
  while ((Res = request(FD32_READDIR, &Rd)) == 0)
  {
    /* According to the RBIL, if search attributes are not 08h (volume  */
    /* label) all files with at most the specified attributes should be */
    /* returned (archive and readonly are always returned), otherwise   */
    /* only the volume label should be returned.                        */
    if (Dta->SearchAttr != FD32_AVOLID)
    {
      BYTE a = Dta->SearchAttr | FD32_AARCHIV | FD32_ARDONLY;
      if ((a | F.Attr) != a) continue;
    }
    else
    {
      if (F.Attr != FD32_AVOLID) continue;
    }
    /* Now that attributes match, compare the file names */
    if ((Res = fd32_build_fcb_name(FoundName, F.ShortName)) < 0) break;
    if (fd32_compare_fcb_names(FoundName, Dta->SearchTemplate)) continue;
    /* Ok, we have found the file, fill the output fields of the Dta */
    Dta->Attr  = F.Attr;
    Dta->MTime = (WORD) F.MTime;
    Dta->MDate = (WORD) (F.MTime >> 16);
    Dta->Size  = F.SizeLo;
    strcpy(Dta->Name, F.ShortName);
    memcpy(Dta->Reserved, F.Reserved, 8);
    return request(FD32_CLOSE, &C);
  }
  request(FD32_CLOSE, &C);
  return Res;
}


/* The DOS style FINDFIRST system call.                                    */
/* Finds the first file or device that matches with the specified file     */
/* name. No handles are kept open by this call, and the search status for  */
/* subsequent FINXNEXT calls is stored in a user buffer, the Disk Transfer */
/* Area (DTA) in the real-mode DOS world. Thus, invoking another system    */
/* call that writes into the DTA overwrites the search status with         */
/* unpredictable results.                                                  */
/* Returns 0 on success, or a negative error code on failure.              */
int fd32_dos_findfirst(char *FileSpec, BYTE Attrib, fd32_fs_dosfind_t *Dta)
{
  char             AuxSpec[FD32_LFNPMAX];
  char             Path[FD32_LFNPMAX];
  char             Name[FD32_LFNPMAX];
  char            *FullPath;
  int              Res;
  fd32_request_t  *request;
  void            *DeviceId;
  fd32_openfile_t  Of;

  /* TODO: About character devices, the RBIL says:
           this call also returns successfully if given the name of a
           character device without wildcards.  DOS 2.x returns attribute
           00h, size 0, and the current date and time.  DOS 3.0+ returns
           attribute 40h and the current date and time. */
  if ((Res = fd32_truename(AuxSpec, FileSpec, FD32_TNSUBST)) < 0) return Res;
  for (;;)
  {
    if ((Res = fd32_get_drive(AuxSpec, &request, &DeviceId, &FullPath)) < 0) return Res;
    /* DOS find functions work only on drives identified by a drive letter */
    if (Res == 0) return FD32_ENODRV;
    Dta->SearchAttr  = Attrib;
    Dta->SearchDrive = Res - 'A' + 1;
    split_path(FullPath, Path, Name);
    if ((Res = fd32_build_fcb_name(Dta->SearchTemplate, Name)) < 0) return Res;
    /* Open the directory and search for the specified template */
    Of.Size     = sizeof(fd32_openfile_t);
    Of.DeviceId = DeviceId;
    Of.FileName = Path;
    Of.Mode     = FD32_OREAD | FD32_OEXIST | FD32_ODIR;
    if ((Res = request(FD32_OPENFILE, &Of)) < 0) /* Open file id in Of.FileId */
    {
      if (Res == FD32_ENMOUNT) continue;
      return Res;
    }
    return dos_find(request, Of.FileId, Dta);
  }
}


/* The DOS style FINDNEXT system call.                                    */
/* Finds the next file (or device?) that matches with file name specified */
/* by a previous DOS style FINDFIRST system call.                         */
int fd32_dos_findnext(fd32_fs_dosfind_t *Dta)
{
  int               Res;
  char              Drive[2] = " :";
  fd32_request_t   *request;
  void             *DeviceId;
  fd32_reopendir_t  Rod;

  for (;;)
  {
    Drive[0] = Dta->SearchDrive + 'A' - 1;
    if ((Res = fd32_get_drive(Drive, &request, &DeviceId, NULL)) < 0) return Res;
    /* Reopen the directory and search for the specified template */
    Rod.Size        = sizeof(fd32_reopendir_t);
    Rod.DeviceId    = DeviceId;
    Rod.DtaReserved = Dta->Reserved;
    if ((Res = request(FD32_REOPENDIR, &Rod)) < 0) /* Open dir id in Of.DirId */
    {
      if (Res == FD32_ENMOUNT) continue;
      return Res;
    }
    return dos_find(request, Rod.DirId, Dta);
  }
}


/* The Long File Name style FINDFIRST system call.                         */
/* Finds the first file (or device?) that matches with the specified name, */
/* returning a file handle to continue the search.                         */
/* On failure, returns a negative error code.                              */
int fd32_lfn_findfirst(char *FileSpec, DWORD Flags, fd32_fs_lfnfind_t *FindData)
{
  int            Handle, Res;
  char           Path[FD32_LFNPMAX];
  char           Name[FD32_LFNPMAX];
  tJft          *J = (tJft *) fd32_get_jft(NULL);
  BYTE           AllowableAttr = (BYTE) Flags;
  BYTE           RequiredAttr  = (BYTE) (Flags >> 8);
  fd32_readdir_t Rd;

  split_path(FileSpec, Path, Name);
  Handle = fd32_open(Path, FD32_OREAD | FD32_OEXIST | FD32_ODIR, FD32_ANONE, 0, NULL);
  if (Handle < 0) return Handle;
  J = &J[Handle];
  Rd.Size  = sizeof(fd32_readdir_t);
  Rd.DirId = J->DeviceId;
  Rd.Entry = (void *) FindData;
  while ((Res = J->request(FD32_READDIR, &Rd)) == 0)
    if (((AllowableAttr | FindData->Attr) == AllowableAttr)
     && ((RequiredAttr & FindData->Attr) == RequiredAttr))
      if ((utf8_fnameicmp(FindData->LongName, Name) == 0)
       || (utf8_fnameicmp(FindData->ShortName, Name) == 0))
      {
        J->SearchFlags = Flags;
        J->SearchName  = (char *) fd32_kmem_get(FD32_LFNPMAX);
        if (J->SearchName == NULL)
        {
          fd32_close(Handle);
          return FD32_ENOMEM;
        }
        strcpy(J->SearchName, Name);
        return Handle;
      }
  fd32_close(Handle);
  return Res;
}


/* The Long File Name style FINDNEXT system call.                        */
/* Finds the next file (or device?) that matches with the name specified */
/* by a previous LFN style FINDFIRST system call.                        */
/* Returns 0 on success, or a negative error code on failure.            */
int fd32_lfn_findnext(int Handle, DWORD Flags, fd32_fs_lfnfind_t *FindData)
{
  tJft          *J;
  int            Res;
  BYTE           AllowableAttr;
  BYTE           RequiredAttr;
  fd32_readdir_t Rd;

  if ((J = validate_search_handle(Handle)) == NULL) return FD32_EBADF;
  /* TODO: Make a tasty mix of passed flags and stored flags */
  AllowableAttr = (BYTE) J->SearchFlags;
  RequiredAttr  = (BYTE) (J->SearchFlags >> 8);
  Rd.Size  = sizeof(fd32_readdir_t);
  Rd.DirId = J->DeviceId;
  Rd.Entry = (void *) FindData;
  while ((Res = J->request(FD32_READDIR, &Rd)) == 0)
    if (((AllowableAttr | FindData->Attr) == AllowableAttr)
     && ((RequiredAttr & FindData->Attr) == RequiredAttr))
      if ((utf8_fnameicmp(FindData->LongName, J->SearchName) == 0)
       || (utf8_fnameicmp(FindData->ShortName, J->SearchName) == 0))
        return 0;
  return Res;
}


/* The Long File Name style FINDCLOSE system call.                     */
/* Terminates a search started by the LFN style FINDFIRST system call, */
/* closing the file handle used for the search.                        */
/* Returns 0 on success, or a negative error code on failure.          */
int fd32_lfn_findclose(int Handle)
{
  tJft *J;
  if ((J = validate_search_handle(Handle)) == NULL) return FD32_EBADF;
  fd32_kmem_free(J->SearchName, FD32_LFNPMAX);
  J->SearchName = NULL;
  return fd32_close(Handle);
}
