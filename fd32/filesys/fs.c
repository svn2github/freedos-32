/**************************************************************************
 * FreeDOS32 File System Layer                                            *
 * Wrappers for file system driver functions and JFT support              *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2002-2003, Salvatore Isaja                               *
 *                                                                        *
 * This is "fs.c" - Wrappers for file system drivers' miscellaneous       *
 *                  service (e.g. open, rename, read, delete, etc.).      *
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

/* TODO: Character device detection is completely broken */
//#define CHARDEVS

#include <ll/i386/hw-data.h>

#include <devices.h>
#include <filesys.h>
#include <errors.h>
#include "fspriv.h"
#ifndef NULL
#define NULL 0
#endif

/* Define this to enable console output on handles 1 (stdout) */
/* and 2 (stderr) even when they are closed.                  */
#define SAFE_CON_OUT


/* Given a handle, checks if it's valid for operations on files (in   */
/* valid range, open and with no search name defined.                 */
/* Returns a pointer to the JFT entry on success, or NULL on failure. */
static inline tJft *validate_file_handle(int Handle)
{
  int   JftSize;
  tJft *Jft = (tJft *) fd32_get_jft(&JftSize);

  if ((Handle < 0) || (Handle >= JftSize)) return NULL;
  if (Jft[Handle].request == NULL) return NULL;
  if (Jft[Handle].SearchName != NULL) return NULL;
  return &Jft[Handle];
}

int fd32_get_dev_info(int fd)
{
  struct jft *j;
  int res;

  j = validate_file_handle(fd);
  if (j == NULL) {
    return FD32_EBADF;
  }

  res = j->request(FD32_GET_DEV_INFO, NULL);

  if (res > 0) {
    return res;
  }

  return 0;
}
#ifdef CHARDEVS
/* TODO: Character devs ABSOLUTELY WRONG! */
static fd32_dev_file_t *is_char_device(char *FileName)
{
  char           *s;
  fd32_dev_t     *Dev;
  
  /* Move to the end of the FullPath string */
  for (s = FileName; *s; s++);
  /* Now move backward until '\' is found. The name starts next the '\' */
  for (; (*s != '\\') && (s != FileName); s--);
  if (*s == '\\') s++;
  if (*s == 0) return NULL;
  if ((Dev = fd32_dev_find(s)) == NULL) return NULL;
  return fd32_dev_query(Dev, FD32_DEV_FILE);
}
#endif


/* The OPEN system call.                                                 */
/* Calls the "open" function of the file system driver related to the    */
/* specified file name, allocates a JFT entry and returns a not negative */
/* file handle (the JFT index) to identify the open file.                */
/* On failure, returns a negative error code.                            */
int fd32_open(char *FileName, DWORD Mode, WORD Attr, WORD AliasHint, int *Result)
{
  fd32_request_t  *request;
  void            *DeviceId;
  char            *Path;
  char             AuxName[FD32_LFNPMAX];
  fd32_openfile_t  Of;
  int              Res, h;
  int              JftSize;
  tJft            *Jft = fd32_get_jft(&JftSize);

  /* Find a free JFT entry and return its handle h */
  for (h = 0; Jft[h].request; h++) if (h == JftSize) {
	  return FD32_EMFILE;
  }

  #ifdef CHARDEVS
  /* First check if we are requested to open a character device */
  if ((F = is_char_device(FileName)))
  {
    F->open(F->P);
    Jft[Handle].Ops = F;
    if (Result) *Result = FD32_OROPEN;
    return Handle;
  }
  #endif

  /* Otherwise call the open function of the file system device */
  if ((Res = fd32_truename(AuxName, FileName, FD32_TNSUBST)) < 0) {
    return Res;
  }
  for (;;)
  {
    if ((Res = fd32_get_drive(AuxName, &request, &DeviceId, &Path)) < 0) {
      return Res;
    }
    Of.Size      = sizeof(fd32_openfile_t);
    Of.DeviceId  = DeviceId;
    Of.FileName  = Path;
    Of.Mode      = Mode;
    Of.Attr      = Attr;
    Of.AliasHint = AliasHint;
    Res = request(FD32_OPENFILE, &Of);
    if (Res > 0) break;
    if (Res != FD32_ENMOUNT) return Res;
  }
  if (Result) *Result = Res;
  Jft[h].request  = request;
  Jft[h].DeviceId = Of.FileId;
  return h;
}


/* The CLOSE system call.                                     */
/* Calls the "close" function of the device hosting the file, */
/* and marks the JFT entry as free.                           */
/* Returns 0 on success, or a negative error code on failure. */
int fd32_close(int Handle)
{
  int           Res;
  fd32_close_t  C;
  tJft         *J;

  if ((J = validate_file_handle(Handle)) == NULL) return FD32_EBADF;
  C.Size     = sizeof(fd32_close_t);
  C.DeviceId = J->DeviceId;
  if ((Res = J->request(FD32_CLOSE, &C)) < 0) return Res;
  J->request = NULL;
  return 0;
}


/* The FFLUSH system call.                                     */
/* Calls the "fflush" function of the device hosting the file. */
/* Returns 0 on success, or a negative error code on failure.  */
int fd32_fflush(int Handle)
{
  fd32_fflush_t  F;
  tJft          *J;

  if ((J = validate_file_handle(Handle)) == NULL) return FD32_EBADF;
  F.Size     = sizeof(fd32_fflush_t);
  F.DeviceId = J->DeviceId;
  return J->request(FD32_FFLUSH, &F);
}


/* The READ system call.                                      */
/* Calls the "read" function of the device hosting the file.  */
/* On success, returns the not negative number of bytes read. */
/* On failure, returns a negative error code.                 */
int fd32_read(int Handle, void *Buffer, int Size)
{
  fd32_read_t  R;
  tJft        *J;

  if ((J = validate_file_handle(Handle)) == NULL) return FD32_EBADF;
  R.Size        = sizeof(fd32_read_t);
  R.DeviceId    = J->DeviceId;
  R.Buffer      = Buffer;
  R.BufferBytes = Size;
  return J->request(FD32_READ, &R);
}


/* The WRITE system call.                                        */
/* Calls the "write" function of the device hosting the file.    */
/* On success, returns the not negative number of bytes written. */
/* On failure, returns a negative error code.                    */
/* TODO: Make the fake console output functional again */
int fd32_write(int Handle, void *Buffer, int Size)
{
  fd32_write_t  W;
  tJft         *J;

  if ((J = validate_file_handle(Handle)) == NULL)
  {
    #ifdef SAFE_CON_OUT
    if ((Handle == 1) || (Handle == 2))
      return fake_console_write(Buffer, Size);
     else
    #endif
      return FD32_EBADF;
  }
  W.Size        = sizeof(fd32_write_t);
  W.DeviceId    = J->DeviceId;
  W.Buffer      = Buffer;
  W.BufferBytes = Size;
  return J->request(FD32_WRITE, &W);
}


/* The UNLINK system call.                                                  */
/* Wrapper for the "unlink" function of the appropriate file system driver. */
/* Returns 0 on success, or a negative error code on failure.               */
int fd32_unlink(char *FileName, DWORD Flags)
{
  fd32_request_t *request;
  fd32_unlink_t   U;
  char            AuxName[FD32_LFNPMAX];
  int             Res;

  if ((Res = fd32_truename(AuxName, FileName, FD32_TNSUBST)) < 0) return Res;
  U.Size  = sizeof(fd32_unlink_t);
  U.Flags = Flags;
  for (;;)
  {
    Res = fd32_get_drive(AuxName, &request, &U.DeviceId, &U.FileName);
    if (Res < 0) return Res;
    Res = request(FD32_UNLINK, &U);
    if (Res != FD32_ENMOUNT) return Res;
  }
}


/* The LSEEK system call.                                     */
/* Calls the "lseek" function of the device hosting the file. */
/* Returns 0 on success, or a negative error code on failure. */
int fd32_lseek(int Handle, long long int Offset, int Origin, long long int *Result)
{
  fd32_lseek_t  S;
  tJft         *J;
  int           Res;

  if ((J = validate_file_handle(Handle)) == NULL) return FD32_EBADF;
  S.Size     = sizeof(fd32_lseek_t);
  S.DeviceId = J->DeviceId;
  S.Offset   = Offset;
  S.Origin   = Origin;
  Res = J->request(FD32_LSEEK, &S);
  if (Result) *Result = S.Offset;
  return Res;
}


/* The DUP system call.                                                    */
/* Allocates a JFT entry referring to the same file identified by the      */
/* source handle.                                                          */
/* Returns the new handle on success, or a negative error code on failure. */
int fd32_dup(int Handle)
{
  fd32_open_t O;
  int         h;
  int         JftSize;
  tJft       *Jft = (tJft *) fd32_get_jft(&JftSize);

  if ((Handle < 0) || (Handle >= JftSize)) return FD32_EBADF;
  if (Jft[Handle].request == NULL) return FD32_EBADF;

  for (h = 0; h < JftSize; h++)
    if (Jft[h].request == NULL)
    {
      Jft[h] = Jft[Handle];
      /* Increment the references counter for the file */
      O.Size     = sizeof(fd32_open_t);
      O.DeviceId = Jft[Handle].DeviceId;
      Jft[Handle].request(FD32_OPEN, &O);
      return h;
    }
  return FD32_EMFILE;
}


/* The DUP2 or FORCEDUP system call.                                    */
/* Causes a specified JFT entry to refer to the same file identified by */
/* the source handle. If the new handle is an open file, it is closed.  */
/* Returns 0, or a negative error code on failure.                      */
int fd32_forcedup(int Handle, int NewHandle)
{
  fd32_open_t O;
  int         Res;
  int         JftSize;
  tJft       *Jft = (tJft *) fd32_get_jft(&JftSize);

  if ((Handle < 0) || (Handle >= JftSize)) return FD32_EBADF;
  if (Jft[Handle].request == NULL) return FD32_EBADF;
  if (NewHandle == Handle) return 0; /* If NewHandle is the same do nothing */
  if ((NewHandle < 0) || (NewHandle >= JftSize)) return FD32_EBADF;

  /* Close the file associated to NewHandle if currently open */
  if (Jft[NewHandle].request != NULL)
    if ((Res = fd32_close(NewHandle)) < 0) return Res;
  /* Assign the JFT entry to the source SFT entry */
  Jft[NewHandle] = Jft[Handle];
  /* Increment the references counter for the file */
  O.Size     = sizeof(fd32_open_t);
  O.DeviceId = Jft[Handle].DeviceId;
  Jft[Handle].request(FD32_OPEN, &O);
  return 0;
}


/* The GET ATTRIBUTES system call.                                     */
/* Gets attributes and time stamps of an open file.                    */
/* On success, returns 0 and fills the passed attributes structure.    */
/* On failure, returns a negative error code.                          */
/* Note: the Size field of the attributes structure must be set before */
/* calling this function.                                              */
int fd32_get_attributes(int Handle, fd32_fs_attr_t *A)
{
  fd32_getattr_t  Ga;
  tJft           *J;

  if ((J = validate_file_handle(Handle)) == NULL) return FD32_EBADF;
  Ga.Size   = sizeof(fd32_getattr_t);
  Ga.FileId = J->DeviceId;
  Ga.Attr   = (void *) A;
  return J->request(FD32_GETATTR, &Ga);
}


/* The CHMOD or SET ATTRIBUTES system call.                   */
/* Sets attributes and time stamps of an open file.           */
/* Returns 0 on success, or a negative error code on failure. */
/* Note: the Size field of the attributes structure must be   */
/* set before calling this function.                          */
int fd32_set_attributes(int Handle, fd32_fs_attr_t *A)
{
  fd32_setattr_t  Sa;
  tJft           *J;

  if ((J = validate_file_handle(Handle)) == NULL) return FD32_EBADF;
  Sa.Size   = sizeof(fd32_setattr_t);
  Sa.FileId = J->DeviceId;
  Sa.Attr   = (void *) A;
  return J->request(FD32_SETATTR, &Sa);
}


/* The RENAME system call.                                                  */
/* Wrapper for the "rename" function of the appropriate file system driver. */
/* Files cannot be moved across different file systems.                     */
/* Returns 0 on success, or a negative error code on failure.               */
int fd32_rename(char *OldName, char *NewName)
{
  fd32_request_t *request_old;
  fd32_request_t *request_new;
  fd32_rename_t   R;
  void           *OldDev;
  void           *NewDev;
  char            AuxOld[FD32_LFNPMAX];
  char            AuxNew[FD32_LFNPMAX];
  int             Res;

  if ((Res = fd32_truename(AuxOld, OldName, FD32_TNSUBST)) < 0) return Res;
  if ((Res = fd32_truename(AuxNew, NewName, FD32_TNSUBST)) < 0) return Res;
  for (;;)
  {
    if ((Res = fd32_get_drive(AuxOld, &request_old, &OldDev, &R.OldName)) < 0) return Res;
    if ((Res = fd32_get_drive(AuxNew, &request_new, &NewDev, &R.NewName)) < 0) return Res;
    if ((request_old != request_new) || (OldDev != NewDev)) return FD32_EXDEV;
    R.Size     = sizeof(fd32_rename_t);
    R.DeviceId = OldDev;
    Res = request_old(FD32_RENAME, &R);
    if (Res != FD32_ENMOUNT) return Res;
  }
}


/* The GET FS INFO system call.                                     */
/* Wrapper for the "get_fsinfo" function of the appropriate driver. */
/* Returns 0 on success, or a negative error code on failure.       */
int fd32_get_fsinfo(char *PathName, fd32_fs_info_t *FSInfo)
{
  fd32_request_t   *request;
  fd32_getfsinfo_t  Gfsi;
  char              AuxName[FD32_LFNPMAX];
  int               Res;

  if ((Res = fd32_truename(AuxName, PathName, FD32_TNSUBST)) < 0) return Res;
  Gfsi.Size   = sizeof(fd32_getfsinfo_t);
  Gfsi.FSInfo = FSInfo;
  for (;;)
  {
    Res = fd32_get_drive(AuxName, &request, &Gfsi.DeviceId, NULL);
    if (Res < 0) return Res;
    Res = request(FD32_GETFSINFO, &Gfsi);
    if (Res != FD32_ENMOUNT) return Res;
  }
}


/* The GET FREE SPACE ON FS system call.                            */
/* Wrapper for the "get_fsfree" function of the appropriate driver. */
/* Returns 0 on success, or a negative error code on failure.       */
int fd32_get_fsfree(char *PathName, fd32_getfsfree_t *FSFree)
{
  fd32_request_t *request;
  char            AuxName[FD32_LFNPMAX];
  int             Res;

  if ((Res = fd32_truename(AuxName, PathName, FD32_TNSUBST)) < 0) return Res;
  if (FSFree->Size < sizeof(fd32_getfsfree_t)) return FD32_EFORMAT;
  for (;;)
  {
    Res = fd32_get_drive(AuxName, &request, &FSFree->DeviceId, NULL);
    if (Res < 0) return Res;
    Res = request(FD32_GETFSFREE, FSFree);
    if (Res != FD32_ENMOUNT) return Res;
  }
}
