/**************************************************************************
 * FreeDOS 32 BIOSDisk Driver                                             *
 * Disk drive support via BIOS                                            *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2002, Salvatore Isaja                               *
 *                                                                        *
 * This is "request.c" - BIOSDisk driver request function                 *
 *                                                                        *
 *                                                                        *
 * This file is part of the FreeDOS32 BIOSDisk Driver.                    *
 *                                                                        *
 * The FreeDOS32 BIOSDisk Driver is free software; you can redistribute   *
 * it and/or modify it under the terms of the GNU General Public License  *
 * as published by the Free Software Foundation; either version 2 of the  *
 * License, or (at your option) any later version.                        *
 *                                                                        *
 * The FreeDOS32 BIOSDisk Driver is distributed in the hope that it will  *
 * be useful, but WITHOUT ANY WARRANTY; without even the implied warranty *
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with the FreeDOS32 BIOSDisk Driver; see the file COPYING;        *
 * if not, write to the Free Software Foundation, Inc.,                   *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA                *
 **************************************************************************/

#include <errors.h>
#include "biosdisk.h"

int biosdisk_request(DWORD Function, void *Params)
{
  tDisk *D;
  switch (Function)
  {
    case FD32_MEDIACHANGE:
    {
      fd32_mediachange_t *X = (fd32_mediachange_t *) Params;
      if (X->Size < sizeof(fd32_mediachange_t)) return FD32_EFORMAT;
      D = (tDisk *) X->DeviceId;
      if (!(D->PrivFlags & REMOVABLE)) return FD32_EINVAL;
      if (D->PrivFlags & CHANGELINE) return biosdisk_mediachange(D);
      /* If disk does not support change line, always report a disk change */
      return 1;
    }
    case FD32_BLOCKREAD:
    {
      fd32_blockread_t *X = (fd32_blockread_t *) Params;
      if (X->Size < sizeof(fd32_blockread_t)) return FD32_EFORMAT;
      D = (tDisk *) X->DeviceId;
      return biosdisk_read(D, X->Start, X->NumBlocks, X->Buffer);
    }
    #ifdef BIOSDISK_WRITE
    case FD32_BLOCKWRITE:
    {
      fd32_blockwrite_t *X = (fd32_blockwrite_t *) Params;
      if (X->Size < sizeof(fd32_blockwrite_t)) return FD32_EFORMAT;
      D = (tDisk *) X->DeviceId;
      return biosdisk_write(D, X->Start, X->NumBlocks, X->Buffer);
    }
    #endif
    case FD32_BLOCKINFO:
    {
      fd32_blockinfo_t *X = (fd32_blockinfo_t *) Params;
      if (X->Size < sizeof(fd32_blockinfo_t)) return FD32_EFORMAT;
      D = (tDisk *) X->DeviceId;
      X->BlockSize   = D->BlockSize;
      X->TotalBlocks = D->TotalBlocks;
      X->Type        = D->Type;
      X->MultiBootId = D->MultiBootId;
      return 0;
    }
  }
  return FD32_EINVAL;
}
