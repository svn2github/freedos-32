/**************************************************************************
 * FreeDOS 32 BIOSDisk Driver                                             *
 * Disk drive support via BIOS                                            *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2002, Salvatore Isaja                               *
 *                                                                        *
 * This is "partscan.c" - Hard disk partitions scanner                    *
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

#define __DEBUG__

typedef struct
{
  BYTE  Active;
  BYTE  StartH;
  BYTE  StartS;
  BYTE  StartC;
  BYTE  Type;
  BYTE  EndH;
  BYTE  EndS;
  BYTE  EndC;
  DWORD LbaStart;
  DWORD LbaSize;
}
__attribute__ ((packed)) tPartTabl;


#define PART_EMPTY     0x00
#define PART_EXT_DOS   0x05
#define PART_EXT_WIN   0x0F
#define PART_EXT_LINUX 0x85

#ifdef __DEBUG__
static const struct { BYTE Id; char *Name; } PartitionTypes[] =
{
  { 0x00, "Empty"             },
  { 0x01, "FAT12"             },
  { 0x04, "FAT16 up to 32 MB" },
  { 0x05, "DOS extended"      },
  { 0x06, "FAT16 over 32 MB"  },
  { 0x07, "NTFS or HPFS"      },
  { 0x0B, "FAT32"             },
  { 0x0C, "FAT32 with LBA"    },
  { 0x0E, "FAT16 with LBA"    },
  { 0x0F, "Win extended"      },
  { 0x82, "Linux Swap"        },
  { 0x83, "Linux native"      },
  { 0x85, "Linux extended"    },
  { 0xFF, "unknown"           },
  { 0, 0 }
};


static char *partname(BYTE Id)
{
  int k;
  for (k = 0; PartitionTypes[k].Name; k++)
    if (PartitionTypes[k].Id == Id) return PartitionTypes[k].Name;
  return "unknown";
}
#endif


static DWORD chs_to_lba(DWORD C, DWORD H, DWORD S, DWORD NumH, DWORD NumS)
{
  return (C * NumH + H) * NumS + S - 1;
}


static int add_partition(tDisk *D, char *Name, int Part, tPartTabl *P)
{
  #ifdef BIOSDISK_FD32DEV
  int    Res;
  char  *DevName;
  #endif
  DWORD  LbaStart = P->LbaStart;
  DWORD  LbaEnd   = P->LbaStart + P->LbaSize - 1;
  DWORD  Type;
  char   NewName[256];
  tDisk *NewD;

  if ((P->StartC != 0xFF) && (P->StartH != 0xFF) && (P->StartS != 0xFF))
    LbaStart = chs_to_lba(P->StartC + ((WORD) (P->StartS & 0xC0) << 2),
                          P->StartH, P->StartS & 0x3F, D->BiosH, D->BiosS);

  if ((P->EndC != 0xFF) && (P->EndH != 0xFF) && (P->EndS != 0xFF))
    LbaEnd   = chs_to_lba(P->EndC + ((WORD) (P->EndS & 0xC0) << 2),
                          P->EndH, P->EndS & 0x3F, D->BiosH, D->BiosS);

  if ((P->Type != PART_EMPTY)
   && (P->Type != PART_EXT_DOS)
   && (P->Type != PART_EXT_WIN)
   && (P->Type != PART_EXT_LINUX))
  {
    ksprintf(NewName, "%s%d", Name, Part);
    Type = FD32_BILOG;
    if (Part < 5)
    {
      if (P->Active) Type = FD32_BIACT;
                else Type = FD32_BIPRI;
    }
    Type |= (DWORD) P->Type << 4;
    #ifdef __DEBUG__
    fd32_message("%s is %s (%02xh), Start: %lu, Size: %lu (%lu MiB)\n",
                 NewName, partname(P->Type), (int) P->Type, LbaStart,
                 LbaEnd - LbaStart + 1, (LbaEnd - LbaStart + 1) * D->BlockSize / 1048576);
    #endif

    /* Allocate and initialize the private data structure */
    if ((NewD = (tDisk *) fd32_kmem_get(sizeof(tDisk))) == NULL)
      return FD32_ENOMEM;
    *NewD = *D;
    NewD->OpenCount = 0;
    NewD->FirstSector = LbaStart;
    NewD->TotalBlocks = LbaEnd - LbaStart + 1;
    NewD->Type        = Type;

    #ifdef BIOSDISK_FD32DEV
    /* Register the new device to the FD32 kernel */
    DevName = (char *) fd32_kmem_get(strlen(NewName) + 1);
    if (DevName == NULL) return FD32_ENOMEM;
    strcpy(DevName, NewName);
    Res = fd32_dev_register(biosdisk_request, (void *) NewD, DevName);
    if (Res < 0) return Res;
    #endif
  }
  return 0;
}


int biosdisk_scanpart(tDisk *D, char *DevName)
{
  tPartTabl Tbl[4];
  int       k, Res;
  int       PartNum = 0;
  BYTE      Buffer[D->BlockSize];
  DWORD     ExtStart = 0, ExtStart1 = 0;

  /* Read the MBR and copy the partition table in the Tbl array */
  if ((Res = biosdisk_read(D, 0, 1, Buffer)) < 0) return Res;
  memcpy(Tbl, Buffer + 446, 64);

  /* If there are no active partitions, mark the first primary as active */
  for (k = 0; k < 4; k++) if (Tbl[k].Active) break;
  if (k == 4) for (k = 0; k < 4; k++)
    if ((Tbl[k].Type != PART_EMPTY)
     && (Tbl[k].Type != PART_EXT_DOS)
     && (Tbl[k].Type != PART_EXT_WIN)
     && (Tbl[k].Type != PART_EXT_LINUX)) { Tbl[k].Active = 0x80; break; }
  /* Create partition devices for primary partitions */
  for (k = 0; k < 4; k++) add_partition(D, DevName, ++PartNum, &Tbl[k]);
  /* Search for an extended partition in the primary partition table */
  for (k = 0; k < 4; k++)
  {
    if ((Tbl[k].Type == PART_EXT_DOS)
     || (Tbl[k].Type == PART_EXT_WIN)
     || (Tbl[k].Type == PART_EXT_LINUX))
    {
      #ifdef __DEBUG__
      fd32_message("%s%d is a %s partition\n", DevName, k + 1,
                   partname(Tbl[k].Type));
      #endif
      ExtStart = Tbl[k].LbaStart;
      if ((Res = biosdisk_read(D, ExtStart, 1, Buffer)) < 0) return Res;
      break;
    }
  }

  /* Read extended partitions */
  if (ExtStart) for (;;)
  {
    memcpy(Tbl, Buffer + 446, 64);
    for (k = 0; k < 4; k++)
    {
      if ((Tbl[k].Type != PART_EMPTY)
       && (Tbl[k].Type != PART_EXT_DOS)
       && (Tbl[k].Type != PART_EXT_WIN)
       && (Tbl[k].Type != PART_EXT_LINUX))
      {
        PartNum++;
        Tbl[k].LbaStart += ExtStart + ExtStart1;
      }
      add_partition(D, DevName, PartNum, &Tbl[k]);
    }
    for (k = 0; k < 4; k++)
    {
      if ((Tbl[k].Type == PART_EXT_DOS)
       || (Tbl[k].Type == PART_EXT_WIN)
       || (Tbl[k].Type == PART_EXT_LINUX))
      {
        ExtStart1 = Tbl[k].LbaStart;
        if ((Res = biosdisk_read(D, ExtStart + ExtStart1, 1, Buffer)) < 0)
          return Res;
        break;
      }
    }
    if (k == 4) break;
  }
  return 0;
}
