/**************************************************************************
 * FreeDOS32 File System Layer                                            *
 * Wrappers for file system driver functions and JFT support              *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2002-2003, Salvatore Isaja                               *
 *                                                                        *
 * This is "dir.c" - Wrappers for FS drivers' CHDIR and RMDIR services.   *
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

#include <ll/i386/hw-data.h>

#include <devices.h>
#include <filesys.h>
#include <errors.h>


/* The MKDIR system call.                                              */
/* Calls the "mkdir" function of the appropriate file system driver.   */
/* Returns 0 on success, or a negative error code on failure.          */
/*                                                                     */
/* TODO: mkdir should forbid directory creation if path name becomes
         longer than 260 chars for LFN or 64 chars for short names. */
int fd32_mkdir(char *DirName)
{
  fd32_request_t *request;
  fd32_mkdir_t    Md;
  char            AuxName[FD32_LFNPMAX];
  int             Res;

  if ((Res = fd32_truename(AuxName, DirName, FD32_TNSUBST)) < 0) return Res;
  Md.Size = sizeof(fd32_mkdir_t);
  for (;;)
  {
    Res = fd32_get_drive(AuxName, &request, &Md.DeviceId, &Md.DirName);
    if (Res < 0) return Res;
    Res = request(FD32_MKDIR, &Md);
    if (Res != FD32_ENMOUNT) return Res;
  }
}


/* The RMDIR system call.                                              */
/* Calls the "rmdir" function of the appropriate file system driver.   */
/* Returns 0 on success, or a negative error code on failure.          */
int fd32_rmdir(char *DirName)
{
  fd32_request_t *request;
  fd32_rmdir_t    Rd;
  char            AuxName[FD32_LFNPMAX];
  int             Res;

  if ((Res = fd32_truename(AuxName, DirName, FD32_TNSUBST)) < 0) return Res;
  Rd.Size = sizeof(fd32_rmdir_t);
  for (;;)
  {
    Res = fd32_get_drive(AuxName, &request, &Rd.DeviceId, &Rd.DirName);
    if (Res < 0) return Res;
    Res = request(FD32_RMDIR, &Rd);
    if (Res != FD32_ENMOUNT) return Res;
  }
}
