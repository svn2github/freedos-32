/**************************************************************************
 * FreeDOS 32 File System Layer                                           *
 * Wrappers for file system driver functions, SFT and JFT support         *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2002, Salvatore Isaja                                    *
 *                                                                        *
 * The FreeDOS 32 File System Layer is part of FreeDOS 32.                *
 *                                                                        *
 * FreeDOS 32 is free software; you can redistribute it and/or modify it  *
 * under the terms of the GNU General Public License as published by the  *
 * Free Software Foundation; either version 2 of the License, or (at your *
 * option) any later version.                                             *
 *                                                                        *
 * FreeDOS 32 is distributed in the hope that it will be useful, but      *
 * WITHOUT ANY WARRANTY; without even the implied warranty                *
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with FreeDOS 32; see the file COPYING; if not, write to the      *
 * Free Software Foundation, Inc.,                                        *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA                *
 **************************************************************************/

/* TO DO:
 * - Open must check if the file name is a DOS character device name.
 *   Opening a character device should be slightly different than a file.
 * - File names are passed to file system driver functions not canonicalized.
 *   Thus, using "." in the root directory will fail.
 * - Lacks of support for current directory.
 * - Lacks of the SUBST feature.
 * - Lacks of wrapper functions for several system calls.
 */

#include <ll/i386/hw-data.h>
#include <ll/i386/string.h>
#include <ll/i386/cons.h>
#include <ll/ctype.h>

#include "jft.h"
#include "stubinfo.h"
#include "kmem.h"
#include "filesys.h"
#include "devices.h"
#include "dev/fs.h"
#include "dev/char.h"


typedef struct Cds
{
  struct Cds       *Next;
  fd32_dev_fsvol_t *Ops;
  char              Drive;
  char              Device[FD32_MAX_LFN_LENGTH];
  char              CurDir[FD32_MAX_LFN_PATH_LENGTH];
}
tCds;


extern struct psp *current_psp;
static char        DefaultDrive = 'C';
static tCds       *CdsList = NULL;


/* Given a handle, checks if valid and returns a pointer to the JFT entry */
static inline jft_t *validate_jft(int Handle)
{
  if ((Handle < 0) || (Handle >= current_psp->JftSize)) return NULL;
  if (current_psp->Jft[Handle].Ops == NULL) return NULL;
  return &current_psp->Jft[Handle];
}


/* Returns the file system driver operations related to the drive letter */
/* specified in the FileName, if present, or the default drive letter.   */
/* Returns NULL if an invalid drive is specified.                        */
static fd32_dev_fsvol_t *get_drive(char *FileName)
{
  char  DriveLetter = DefaultDrive;
  tCds *D;

  if (FileName[1] == ':')
  {
    /* FIX ME: LASTDRIVE should be read from Config.sys */
    if ((toupper(FileName[0]) < 'A') || (toupper(FileName[0]) > 'Z'))
      return NULL; /* Invalid drive specification */
    DriveLetter = toupper(FileName[0]);
  }

  for (D = CdsList; D != NULL; D = D->Next)
    if (DriveLetter == D->Drive) return D->Ops;
  return NULL; /* Invalid drive specification */
}


/* Returns the file name without the drive specification part */
static char *get_name_without_drive(char *FileName)
{
  if (FileName[1] == ':') return &FileName[2];
                     else return FileName;
}


/* The SET DEFAULT DRIVE system call.                 */
/* Returns the number of available drives on success, */
/* or a negative error code on failure.               */
int fd32_set_default_drive(char Drive)
{
  /* FIX ME: LASTDRIVE should be read frm Config.sys */
  if ((toupper(Drive) < 'A') || (toupper(Drive) > 'Z'))
    return FD32_ERROR_UNKNOWN_UNIT;
  DefaultDrive = toupper(Drive);
  /* FIX ME: From the RBIL (INT 21h, AH=0Eh)                          */
  /*         under DOS 3.0+, the return value is the greatest of 5,   */
  /*         the value of LASTDRIVE= in CONFIG.SYS, and the number of */
  /*         drives actually present.                                 */
  return 'Z' - 'A' + 1;
}


/* The GET DEFAULT DRIVE system call.               */
/* Returns the letter of the current default drive. */
char fd32_get_default_drive()
{
  return DefaultDrive;
}


/* The MKDIR system call.                                              */
/* Calls the "mkdir" function of the file system driver related to the */
/* specified directory name.                                           */
/* Returns 0 on success, or a negative error code on failure.          */
/*                                                                     */
/* FIX ME: mkdir should forbid directory creation if path name becomes */
/*         longer than 260 chars for LFN or 64 chars for short names.  */
int fd32_mkdir(char *DirName, int UseLfn)
{
  fd32_dev_fsvol_t *Ops = get_drive(DirName);
  if (Ops == NULL) return FD32_ERROR_UNKNOWN_UNIT;
  return Ops->mkdir(Ops->P, get_name_without_drive(DirName), UseLfn);
}


/* The RMDIR system call.                                              */
/* Calls the "rmdir" function of the file system driver related to the */
/* specified directory name.                                           */
/* Returns 0 on success, or a negative error code on failure.          */
int fd32_rmdir(char *DirName, int UseLfn)
{
  fd32_dev_fsvol_t *Ops = get_drive(DirName);
  if (Ops == NULL) return FD32_ERROR_UNKNOWN_UNIT;
  return Ops->rmdir(Ops->P, get_name_without_drive(DirName), UseLfn);
}


/* The OPEN system call.                                                */
/* Calls the "open" function of the file system driver related to the   */
/* specified file name, allocates a JFT entry and returns a file handle */
/* (the JFT index) to identify the open file.                           */
/* On failure, returns a negative error code.                           */
int fd32_open(char *FileName, DWORD Mode, WORD Attr,
              WORD AliasHint, int *Result, int UseLfn)
{
  fd32_dev_fsvol_t *Ops;
  int               Fd, Handle;

  /* FIX ME: Must check if FileName is a valid character device name */

  /* Find a free JFT entry */
  for (Handle = 0; Handle < current_psp->JftSize; Handle++)
    if (current_psp->Jft[Handle].Ops == NULL) break;
  if (Handle == current_psp->JftSize) return FD32_ERROR_TOO_MANY_OPEN_FILES;

  /* Call the open function of the file system device */
  Ops = get_drive(FileName);
  if (Ops == NULL) return FD32_ERROR_UNKNOWN_UNIT;
  Fd  = Ops->open(Ops->P, get_name_without_drive(FileName),
                  Mode, Attr, AliasHint, Result, UseLfn);
  if (Fd < 0) return Fd;

  /* Fill JFT entry */
  current_psp->Jft[Handle].Ops = (fd32_dev_ops_t *) Ops;
  current_psp->Jft[Handle].Fd  = Fd;
  return Handle;
}


/* The CLOSE system call.                                     */
/* Calls the "close" function of the device hosting the file, */
/* and marks the JFT entry as free.                           */
/* Returns 0 on success, or a negative error code on failure. */
int fd32_close(int Handle)
{
  int   Res;
  jft_t *J;

  if ((J = validate_jft(Handle)) == NULL) return FD32_ERROR_INVALID_HANDLE;
  switch (J->Ops->Type)
  {
    case FD32_DEV_FSVOL :
    {
      fd32_dev_fsvol_t *D = (fd32_dev_fsvol_t *) J->Ops;
      Res = D->close(D->P, J->Fd);
      if (Res) return Res;
      J->Ops = NULL;
      return 0;
    }
  }
  return -1; /* FIX ME: Error is "Unknown device type" */
}


/* The FFLUSH system call.                                     */
/* Calls the "fflush" function of the device hosting the file. */
/* Returns 0 on success, or a negative error code on failure.  */
int fd32_fflush(int Handle)
{
  jft_t *J;

  if ((J = validate_jft(Handle)) == NULL) return FD32_ERROR_INVALID_HANDLE;
  switch (J->Ops->Type)
  {
    case FD32_DEV_FSVOL :
    {
      fd32_dev_fsvol_t *D = (fd32_dev_fsvol_t *) J->Ops;
      return D->fflush(D->P, J->Fd);
    }
  }
  return -1; /* FIX ME: Error is "Unknown device type" */
}

fd32_dev_char_t fake_console;
int fake_console_input(void *p, DWORD len, BYTE *buffer)
{
  fd32_dev_char_t *console_ops = p;

  return console_ops->read(NULL, 1, buffer);
}

/* The READ system call.                                      */
/* Calls the "read" function of the device hosting the file.  */
/* Returns 0 on success, or a negative error code on failure. */
int fd32_read(int Handle, void *Buffer, DWORD Size, DWORD *Result)
{
  jft_t *J;
  if ((J = validate_jft(Handle)) == NULL) return FD32_ERROR_INVALID_HANDLE;
  switch (J->Ops->Type)
  {
    case FD32_DEV_FSVOL:
      return ((fd32_dev_fsvol_t *) J->Ops)->read(J->Ops->P, J->Fd, Buffer,
                                                 Size, Result);
    case FD32_DEV_CHAR:
     *Result = ((fd32_dev_char_t *) J->Ops)->read(J->Ops->P, Size, Buffer);
     return 0;
  }
  return -1; /* FIX ME: Error is "Unknown device type" */
}


/* We want to do some output even if the   */
/* console driver does not implement it... */
static int fake_console_output(char *buffer, int len)
{
  char string[255];
  int n, count;

  count = len;
  while (count > 0) {
    n = 254;
    if (n > count) n = count;
    memcpy(string, (char *)buffer, n);
    string[n] = 0;
    cputs(string);
    count -= n;
    buffer += n;
  }
  return len;
}


/* The WRITE system call.                                     */
/* Calls the "write" function of the device hosting the file. */
/* Returns 0 on success, or a negative error code on failure. */
int fd32_write(int Handle, void *Buffer, DWORD Size, DWORD *Result)
{
  jft_t *J;

  if ((Handle < 0) || (Handle >= current_psp->JftSize))
    return FD32_ERROR_INVALID_HANDLE;
  J = &current_psp->Jft[Handle];
  if (J->Ops == NULL)
  {
    if ((Handle == 1) || (Handle == 2)) {
      *Result = fake_console_output((char *) Buffer, Size);
      return 0;
    } else {
      return FD32_ERROR_INVALID_HANDLE;
    }
  }
  switch (J->Ops->Type)
  {
    case FD32_DEV_FSVOL:
      return ((fd32_dev_fsvol_t *) J->Ops)->write(J->Ops->P, J->Fd, Buffer,
                                                  Size, Result);
    case FD32_DEV_CHAR:
      return ((fd32_dev_char_t *) J->Ops)->write(J->Ops->P, Size, Buffer);
  }
  return -1; /* FIX ME: Error is "Unknown device type" */
}


/* The UNLINK system call.                                                  */
/* Wrapper for the "unlink" function of the appropriate file system driver. */
/* Returns 0 on success, or a negative error code on failure.               */
int fd32_unlink(char *FileName, DWORD Flags, int UseLfn)
{
  fd32_dev_fsvol_t *Ops = get_drive(FileName);
  if (Ops == NULL) return FD32_ERROR_UNKNOWN_UNIT;
  return Ops->unlink(Ops->P, get_name_without_drive(FileName), Flags, UseLfn);
}


/* The LSEEK system call.                                     */
/* Calls the "lseek" function of the device hosting the file. */
/* Returns 0 on success, or a negative error code on failure. */
int fd32_lseek(int Handle, SQWORD Offset, int Origin, SQWORD *Result)
{
  jft_t *J;
  if ((J = validate_jft(Handle)) == NULL) return FD32_ERROR_INVALID_HANDLE;
  switch (J->Ops->Type)
  {
    case FD32_DEV_FSVOL :
    {
      fd32_dev_fsvol_t *D = (fd32_dev_fsvol_t *) J->Ops;
      return D->lseek(D->P, J->Fd, Offset, Origin, Result);
    }
  }
  return -1; /* FIX ME: Error is "Unknown device type" */
}


/* The DUP system call.                                                    */
/* Allocates a JFT entry referring to the same file identified by the      */
/* source handle.                                                          */
/* Returns the new handle on success, or a negative error code on failure. */
int fd32_dup(int Handle)
{
  int   NewHandle;
  jft_t *J;

  if ((J = validate_jft(Handle)) == NULL) return FD32_ERROR_INVALID_HANDLE;
  for (NewHandle = 0; NewHandle < current_psp->JftSize; NewHandle++)
    if (current_psp->Jft[NewHandle].Ops == NULL)
    {
      current_psp->Jft[NewHandle] = current_psp->Jft[Handle];
      /* FIX ME: Should increment the open counter of the fs device */
      return NewHandle;
    }
  return FD32_ERROR_TOO_MANY_OPEN_FILES;
}


/* The DUP2 or FORCEDUP system call.                                    */
/* Causes a specified JFT entry to refer to the same file identified by */
/* the source handle. If the new handle is an open file, it is closed.  */
/* Returns 0, or a negative error code on failure.                      */
int fd32_forcedup(int Handle, int NewHandle)
{
  int   Res;
  jft_t *J;

  if ((J = validate_jft(Handle)) == NULL) return FD32_ERROR_INVALID_HANDLE;
  if (NewHandle == Handle) return 0; /* if same handle do nothing */
  if ((NewHandle < 0) || (NewHandle >= current_psp->JftSize))
    return FD32_ERROR_INVALID_HANDLE;

  /* Close the file associated to NewHandle if currently open */
  if (current_psp->Jft[NewHandle].Ops != NULL)
    if ((Res = fd32_close(NewHandle)) < 0) return Res;
  /* Assign the JFT entry to the source SFT entry */
  current_psp->Jft[NewHandle] = current_psp->Jft[Handle];
  /* FIX ME: Should increment the open counter of the fs device */
  return 0;
}


int fd32_get_attributes(int Handle, fd32_dev_fs_attr_t *A)
{
  jft_t *J;
  if ((J = validate_jft(Handle)) == NULL) return FD32_ERROR_INVALID_HANDLE;
  switch (J->Ops->Type)
  {
    case FD32_DEV_FSVOL :
    {
      fd32_dev_fsvol_t *D = (fd32_dev_fsvol_t *) J->Ops;
      return D->get_attr(D->P, J->Fd, A);
    }
  }
  return -1; /* FIX ME: Error is "Unknown device type" */
}


int fd32_set_attributes(int Handle, fd32_dev_fs_attr_t *A)
{
  jft_t *J;
  if ((J = validate_jft(Handle)) == NULL) return FD32_ERROR_INVALID_HANDLE;
  switch (J->Ops->Type)
  {
    case FD32_DEV_FSVOL :
    {
      fd32_dev_fsvol_t *D = (fd32_dev_fsvol_t *) J->Ops;
      return D->set_attr(D->P, J->Fd, A);
    }
  }
  return -1; /* FIX ME: Error is "Unknown device type" */
}


/* The DOS style FINDFIRST system call.                                    */
/* Finds the first file or device that matches with the specified file     */
/* name. No handles are opened by this call, and the search status for     */
/* subsequent FINXNEXT calls is stored in a user buffer, the Disk Transfer */
/* Area (DTA) in the real-mode DOS world. Thus, invoking another system    */
/* call that writes into the DTA overwrites the search status with         */
/* unpredictable results.                                                  */
/* Returns 0 on success, or a negative error code on failure.              */
int fd32_dos_findfirst(char *FileSpec, BYTE Attrib, fd32_fs_dosfind_t *Dta)
{
  fd32_dev_fsvol_t *D;

  /* FIX ME: About character devices, the RBIL says:
             this call also returns successfully if given the name of a
             character device without wildcards.  DOS 2.x returns attribute
             00h, size 0, and the current date and time.  DOS 3.0+ returns
             attribute 40h and the current date and time. */

  /* Call the dos_findfirst function of the file system device */
  Dta->Ops = D = get_drive(FileSpec);
  if (D == NULL) return FD32_ERROR_UNKNOWN_UNIT;
  return D->dos_findfirst(D->P, get_name_without_drive(FileSpec),
                          Attrib, Dta);
}


/* The DOS style FINDNEXT system call.                                    */
/* Finds the next file (or device?) that matches with file name specified */
/* by a previous DOS style FINDFIRST system call.                         */
int fd32_dos_findnext(fd32_fs_dosfind_t *Dta)
{
  fd32_dev_fsvol_t *D = Dta->Ops;
  /* FIX ME: What about character devices? */
  /* A simple validity check of the search status stored into the DTA */
  if (D == NULL) return FD32_ERROR_FORMAT_INVALID;
  if (D->Type != FD32_DEV_FSVOL) return FD32_ERROR_FORMAT_INVALID;
  return D->dos_findnext(Dta);
}


/* The Long File Name style FINDFIRST system call.                         */
/* Finds the first file (or device?) that matches with the specified name, */
/* returning a file handle to continue the search.                         */
/* This function is almost an exact copy of the OPEN system call.          */
/* On failure, returns a negative error code.                              */
int fd32_lfn_findfirst(char *FileSpec, DWORD Flags, fd32_fs_lfnfind_t *FindData)
{
  fd32_dev_fsvol_t *Ops;
  int               Fd, Handle;

  /* FIX ME: What about character devices? */

  /* Find a free JFT entry */
  for (Handle = 0; Handle < current_psp->JftSize; Handle++)
    if (current_psp->Jft[Handle].Ops == NULL) break;
  if (Handle == current_psp->JftSize) return FD32_ERROR_TOO_MANY_OPEN_FILES;

  /* Call the open function of the file system device */
  Ops = get_drive(FileSpec);
  if (Ops == NULL) return FD32_ERROR_UNKNOWN_UNIT;
  Fd  = Ops->lfn_findfirst(Ops->P, get_name_without_drive(FileSpec),
                           Flags, FindData);
  if (Fd < 0) return Fd;

  /* Fill JFT entry */
  current_psp->Jft[Handle].Ops = (fd32_dev_ops_t *) Ops;
  current_psp->Jft[Handle].Fd  = Fd;
  return Handle;
}


/* The Long File Name style FINDNEXT system call.                        */
/* Finds the next file (or device?) that matches with the name specified */
/* by a previous LFN style FINDFIRST system call.                        */
/* Returns 0 on success, or a negative error code on failure.            */
int fd32_lfn_findnext(int Handle, DWORD Flags, fd32_fs_lfnfind_t *FindData)
{
  jft_t *J;
  if ((J = validate_jft(Handle)) == NULL) return FD32_ERROR_INVALID_HANDLE;
  switch (J->Ops->Type)
  {
    case FD32_DEV_FSVOL :
    {
      fd32_dev_fsvol_t *D = (fd32_dev_fsvol_t *) J->Ops;
      return D->lfn_findnext(D->P, J->Fd, Flags, FindData);
    }
  }
  return -1; /* FIX ME: Error is "Unknown device type" */
}


/* The Long File Name style FINDCLOSE system call.                     */
/* Terminates a search started by the LFN style FINDFIRST system call, */
/* closing the file handle used for the search.                        */
/* This function is almost an exact copy of the CLOSE system call.     */
/* Returns 0 on success, or a negative error code on failure.          */
int fd32_lfn_findclose(int Handle)
{
  int   Res;
  jft_t *J;

  if ((J = validate_jft(Handle)) == NULL) return FD32_ERROR_INVALID_HANDLE;
  switch (J->Ops->Type)
  {
    case FD32_DEV_FSVOL :
    {
      fd32_dev_fsvol_t *D = (fd32_dev_fsvol_t *) J->Ops;
      Res = D->lfn_findclose(D->P, J->Fd);
      if (Res) return Res;
      J->Ops = NULL;
      return 0;
    }
  }
  return -1; /* FIX ME: Error is "Unknown device type" */
}


/* The RENAME system call.                                                  */
/* Wrapper for the "rename" function of the appropriate file system driver. */
/* Files cannot be moved across different file systems.                     */
/* Returns 0 on success, or a negative error code on failure.               */
int fd32_rename(char *OldName, char *NewName, int UseLfn)
{
  fd32_dev_fsvol_t *OpsOld = get_drive(OldName);
  fd32_dev_fsvol_t *OpsNew = get_drive(NewName);
  if ((OpsOld == NULL) || (OpsNew = NULL)) return FD32_ERROR_UNKNOWN_UNIT;
  if (OpsOld != OpsNew) return FD32_ERROR_NOT_SAME_DEVICE;
  return OpsOld->rename(OpsOld->P, get_name_without_drive(OldName),
                        get_name_without_drive(NewName), UseLfn);
}


/* Adds a drive in the Current Directory Structure list.      */
/* Returns 0 on success, or a negative error code on failure. */
int fd32_add_drive(char Drive, char *Device)
{
  tCds *D;
  fd32_dev_fsvol_t *Ops = fd32_dev_query(Device, FD32_DEV_FSVOL);
  if (Ops == NULL) return -1; /* FIX ME: Error is "invalid device" */
  D = mem_get(sizeof(tCds));
  D->Ops   = Ops;
  D->Drive = toupper(Drive);
  strcpy(D->Device, Device);
  strcpy(D->CurDir, "\\");
  D->Next = CdsList;
  CdsList = D;
  return 0;
}


#if 0
int fd32_chdir(char Drive, char *DirName)
{
  struct Cds *D;
  for (D = CdsList; D != NULL; D = D->Next)
    if (toupper(Drive) == D->Drive)
    {
      strcpy(D->CurDir, DirName);
      return 0;
    }
  return -1; /* Invalid drive */
}


fd32_dev_fsvol_t *fd32_cwd(char Drive, char *DirName)
{
  struct Cds *D;
  for (D = CdsList; D != NULL; D = D->Next)
    if (toupper(Drive) == D->Drive)
    {
      strcpy(DirName, D->CurDir);
      return D->Ops;
    }
  return NULL; /* Invalid drive */
}


int fd32_canonicalize(char *Dest, char *Source)
{
  char              Aux[FD32_MAX_LFN_PATH_LENGTH];
  char              Partial[FD32_MAX_LFN_LENGTH];
  char             *pPartial;
  char             *pAux = Aux;
  int               Dots;
  fd32_dev_fsvol_t *Ops;
//  fd32_fs_lfnfind_t FindData;
//  SDWORD         Fd;

  /* Insert drive letter */
  if (Source[1] == ':')
  {
    /* FIX ME: LASTDRIVE should be read from Config.sys */
    if ((toupper(*Source) < 'A') || (toupper(*Source) > 'Z'))
      return -1; /* FIX ME: Replace with Invalid drive specification */
    *(pAux++) = *(Source++); /* Copy drive letter */
    *(pAux++) = *(Source++); /* Copy colon (':')  */
  }
  else
  {
    *(pAux++) = DefaultDrive;
    *(pAux++) = ':';
  }

  Ops = fd32_cwd(Aux[0], Partial);

  /* Insert current directory if relative path */
  if ((*Source != '\\') && (*Source != '/'))
  {
    /* Copy the current directory path into the Aux buffer */
    for (pPartial = Partial; (*pAux = *pPartial); ++pAux, ++pPartial);
    /* Append a trailing backslash if not already preset */
    if (*(pAux - 1) != '\\') *(pAux++) = '\\';
  }
  else
  {
    *(pAux++) = '\\';
    Source++;
  }

  /* Canonicalize the rest of the pathname */
  while (*Source)
  {
    /* Extract the next partial from the path (before '\' or '/' or NULL) */
    pPartial = Partial;
    while ((*Source != '\\') && (*Source != '/') && (*Source))
      *(pPartial++) = *(Source++);
    if (*Source) Source++;
    *pPartial = 0;
    /* Skip consecutive slashes */
    if (pPartial == Partial) continue;
    
    #ifdef DEBUG
    printf("Partial : #%s#\n",Partial);
    #endif

    /* Search for ".", "..", "..." etc. entries */
    Dots = 0;
    for (pPartial = Partial; *pPartial; pPartial++)
      if (*pPartial == '.') Dots++;
                       else { Dots = 0; break; }

    /* If not a ".", "..", "..." etc. entry, append the Partial to Aux */
    if (Dots == 0)
    {
//      Fd = Ops->lfn_findfirst(Ops->P, Partial, 0 /*flags*/, &FindData);
//      Ops->lfn_findclose(Ops->P, Fd);
      for (pPartial = Partial; (*pAux = *pPartial); ++pAux, ++pPartial);
      *(pAux++) = '\\';
      *pAux = 0;
    }
    else
    /* Else rewind the path in Aux for the number of dots minus one */
    {
      for (Dots--; Dots > 0; Dots--)
      {
        /* Skip the trailing backslash */
        if (pAux > &Aux[3]) pAux -= 2; else return 1; /* Path not found */
        /* Go back to the previous backslash */
        while (*pAux != '\\')
          if (pAux > &Aux[2]) pAux--; else return 1; /* Path not found */
        *(++pAux) = 0;
      }
    }
  }
  /* Remove trailing backslash, convert to upper case and copy to Dest */
  if (*(pAux - 1) == '\\') *(pAux - 1) = 0;
//  strupr(Aux);
  for (pAux = Aux; (*Dest = *pAux); ++Dest, ++pAux);
  return 0;
}
#endif


#if 0
int fd32_resolve_subst(char *Dest, char *Source)
{
  char          Aux[FD32_MAX_LFN_PATH_LENGTH];
  char         *s, *pAux;
  struct Subst *S;

  /* Canonicalize the path */
  if (fd32_canonicalize(Aux, Source)) return 1;
//  strcpy(Aux, Source);
  /* Scan the list of 'struct Subst's */
  for (S = SubstList; S != NULL; S = S->Next)
  {
    /* Check if the Logical field of a struct Subst matches with the */
    /* first part of the canonicalized name                          */
    for (s = S->Logical, pAux = Aux; (*s != NULL) && (*s == *pAux); s++, pAux++);
    /* If matches, replace the occurrence of Logical with Physical   */
    /* into the canonicalized name                                   */
    if (*s == NULL)
    {
      strcpy(Dest, S->Physical);
      strcat(Dest, pAux);
      return 0;
    }
  }
  /* If not found, use the canonicalized name unchanged */
  strcpy(Dest, Aux);
  return 0;
}
#endif


#if 0
extern void fat_driver_init();
extern void biosdisk_init();

int main()
{
  SDWORD            Handle, Res = 0;
  int               k;
  fd32_fs_lfnfind_t FindData;
  fd32_fs_dosfind_t Dta;

  /* Initialization... */
  fat_driver_init();
  biosdisk_init();
  DefaultDrive = 'D';
  fd32_add_drive('d', "hdb1");
  current_psp->Jft = (tJft *) fd32_kmem_get(sizeof(tJft) * 20);
  for (k = 0; k < 20; k++) current_psp->Jft[k].Ops = NULL;
  current_psp->JftSize = 20;

  /* Do something with the file system */
  #if 0
  Handle = fd32_open("root\\..\\logistica.pdf",
                     FD32_OPEN_ACCESS_READ | FD32_OPEN_EXIST_OPEN,
                     0, 0, NULL, FD32_LFN);
  printf("\nHandle is %li\n",Handle);
  if (Handle < 0) return Handle;
  fd32_read(Handle, Buffer, sizeof(Buffer), NULL);
  #endif

  #if 0
  Handle = fd32_lfn_findfirst("*",
                              FD32_FIND_ALLOW_ALL | FD32_FIND_REQUIRE_NONE,
                              &FindData);
  printf("Filefind handle is %li\n",Handle);
  if (Handle < 0) return Handle;

  while (!Res)
  {
    printf("Long Name: \"%s\", Short: \"%s\"\n", FindData.LongName, FindData.ShortName);
    Res = fd32_lfn_findnext(Handle,
                            FD32_FIND_ALLOW_ALL | FD32_FIND_REQUIRE_NONE,
                            &FindData);
  }
  fd32_lfn_findclose(Handle);
  #endif

  Res = fd32_dos_findfirst("*pro", FD32_ATTR_ALL, &Dta);
  while (!Res)
  {
    printf("Found: \"%s\"\n", Dta.Name);
    Res = fd32_dos_findnext(&Dta);
  }

//  fd32_chdir('c',"\\prova\\bello");
//  fd32_add_subst("c","hda1");
//  fd32_resolve_subst(Name, Name);
//  fd32_canonicalize(Name, Name);
//  printf("Name: %s\n",Name);
  return 0;
}
#endif
