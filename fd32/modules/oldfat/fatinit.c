/**************************************************************************
 * FreeDOS 32 FAT Driver                                                  *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2002, Salvatore Isaja                               *
 *                                                                        *
 * This is "fatinit.c" - Driver initialization for the FreeDOS 32 kernel  *
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

/* These are from fd32/include/kernel.h */
/* TODO: Include the file cleanly */
#define SUBSTITUTE 1
#define ADD 0
int add_call(char *name, DWORD address, int mode);

/* FAT Driver entry point. Called by the FD32 kernel. */
int fat_init()
{
  int Res;
  fd32_message("Installing the FAT Driver...\n");
  /* Register the FAT Driver to the File System Layer */
  if ((Res = fd32_add_fs(fat_request)) < 0) return Res;
  /* Register the FAT Driver request function to the kernel symbol table */
  if (add_call("fat_request", (DWORD) fat_request, ADD) == -1)
  {
    fd32_error("Couldn't add 'fat_request' to the symbol table\n");
    return FD32_ENOMEM; /* TODO: Check if ENOMEM is true... */
  }
  fd32_message("FAT Driver installed.\n");
  return 0;
}
