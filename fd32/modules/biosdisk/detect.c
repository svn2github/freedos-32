/**************************************************************************
 * FreeDOS 32 BIOSDisk Driver                                             *
 * Disk drive support via BIOS                                            *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2002, Salvatore Isaja                               *
 *                                                                        *
 * This is "detect.c" - Detection and initialization of disk devices      *
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


/* Drive informations returned by standard INT 13h/AH=08h */
/* 'get drive parameters'.                                */
typedef struct
{
  DWORD BiosNumber;
  DWORD DriveType;
  DWORD Cylinders;
  DWORD Sectors;
  DWORD Heads;
  DWORD NumberOfDrives;
  WORD  ParamTableSeg;
  WORD  ParamTableOff;
}
tStdDriveInfo;


/* Informations returned by extended INT 13h/AH=41h */
/* 'IBM/MS Extensions - installation check'.        */
typedef struct
{
  BYTE  Version;
  BYTE  Internal;
  WORD  ApiSupport;
  BYTE  ExtVersion;
}
tExtInstCheck;


/* Extended drive informations returned by extended INT 13h/AH=48h */
/* 'IBM/MS Extensions - get drive parameters'.                     */
typedef struct
{
  /* Fields present in version 1.x+ */
  WORD   BufSize;      /* 001Ah for v1.x, 001Eh for v2.x, 42h for v3.0     */
  WORD   InfoFlags;
  DWORD  PhysC;
  DWORD  PhysH;
  DWORD  PhysS;
  QWORD  TotalSectors;
  WORD   BytesPerSector;
  /* Fields present in version 2.0+ */
  DWORD  EddParams;    /* FFFFh:FFFFh if not available                     */
  /* Fields present in version 3.0 */
  WORD   Signature;    /* BEDDh to indicate presence of Device Path info   */
  BYTE   DevPathLen;   /* including signature and this byte (24h for v3.0) */
  BYTE   Reserved[3];  /* 0                                                */
  char   HostBus[4];   /* ASCIZ name of host bus ("ISA" or "PCI")          */
  char   Interface[8]; /* ASCIZ name of interface type                     */
  BYTE   IntPath[8];   /* Interface Path (see #00275)                      */
  BYTE   DevPath[8];   /* Device Path (see #00276)                         */
  BYTE   Reserved1;    /* 0                                                */
  BYTE   Checksum;
}
__attribute__ ((packed)) tExtDriveInfo;


/* Device Parameter Table Extension returned by newer versions of      */
/* extended INT 13h/AH=48h 'IBM/MS Extensions - get drive parameters'. */
typedef struct
{
  WORD   BasePort;    /* physical I/O port base address                    */
  WORD   ControlPort; /* disk-drive control port address                   */
  BYTE   Flags;
  BYTE   Proprietary;
  BYTE   Irq;         /* bits 3-0; bits 7-4 reserved and must be 0         */
  BYTE   MultiSector; /* sector count for multi-sector transfers           */
  BYTE   DmaControl;  /* bits 7-4: DMA type (0-2), bits 3-0: DMA channel   */
  BYTE   PioControl;  /* bits 7-4: reserved (0), bits 3-0: PIO type (1-4)  */
  WORD   Options;
  WORD   Reserved;    /* 0                                                 */
  BYTE   ExtRevision; /* extension revision level                          */
  BYTE   Checksum;
}
__attribute__ ((packed)) tEddParams;


/* Wrapper function for standard INT 13h 'get disk type' service. */
/* On success, returns one of the following value:                */
/*  0 - Drive not present                                         */
/*  1 - Floppy without change-line support                        */
/*  2 - Floppy with change-line support                           */
/*  3 - Hard disk                                                 */
/* On failure, returns a negative number.                         */
static int get_disk_type(BYTE Drive)
{
  FD32_DECLARE_REGS(Regs);

  AH(Regs) = 0x15; /* Disk - Get disk type */
  DL(Regs) = Drive;
  fd32_int(0x13, Regs);
  if (FLAGS(Regs) & 0x1) return -1; /* FIX ME: Convert BIOS error codes! */
  return AH(Regs);
}


/* Gets drive parameters using the standard BIOS service INT 13h/AH=08h. */
/* On success, returns 0 and fills the Std structure.                    */
/* Returns nonzero on failure.                                           */
static int std_driveinfo(BYTE Drive, tStdDriveInfo *Std)
{
  FD32_DECLARE_REGS(Regs);

  AH(Regs) = 0x08;  /* Disk - get drive parameters */
  DL(Regs) = Drive;
  ES(Regs) = 0;     /* to guard against BIOS bugs  */
  DI(Regs) = 0;     /* to guard against BIOS bugs  */
  fd32_int(0x13, Regs);
  if (FLAGS(Regs) & 0x1) return -1; //INT 13h error code in AH

  Std->BiosNumber     = Drive;
  Std->DriveType      = BL(Regs);
  Std->Cylinders      = CH(Regs) + ((WORD) (CL(Regs) & 0xC0) << 2) + 1;
  Std->Sectors        = CL(Regs) & 0x3F;
  Std->Heads          = DH(Regs) + 1;
  Std->NumberOfDrives = DL(Regs);
  Std->ParamTableSeg  = ES(Regs);
  Std->ParamTableOff  = DI(Regs);
  return 0;
}


/* Wrapper function for extended INT 13h 'extensions installation check'. */
/* Returns zero if extensions are installed, nonzero if not.              */
static int ext_instcheck(BYTE Drive, tExtInstCheck *C)
{
  FD32_DECLARE_REGS(Regs);

  AH(Regs) = 0x41;
  BX(Regs) = 0x55AA;
  DL(Regs) = Drive;
  fd32_int(0x13, Regs);
  if ((FLAGS(Regs) & 0x1) || (BX(Regs) != 0xAA55)) return 1;
  C->Version    = AH(Regs);
  C->Internal   = AL(Regs);
  C->ApiSupport = CX(Regs);
  C->ExtVersion = DH(Regs);
  return 0;
}


/* Gets drive parameters using the extended BIOS service INT 13h/AH=48h. */
/* On success, returns 0 and fills the P structure.                      */
/* Returns nonzero on failure.                                           */
static int ext_driveinfo(BYTE Drive, tDisk *D)
{
  FD32_DECLARE_REGS(Regs);
  LOWMEM_ADDR   BufSelector;
  WORD          BufSegment, BufOffset;
  tExtInstCheck Chk;
  tExtDriveInfo Ext;
  tEddParams    Edd;
  
  memset(&Ext, 0, sizeof(tExtDriveInfo));

  if (ext_instcheck(Drive, &Chk)) return 1;
  switch (Chk.Version)
  {
    case 0x01 : Ext.BufSize = 0x1A; break;
    case 0x20 :
    case 0x21 : Ext.BufSize = 0x1E; break;
    case 0x30 : Ext.BufSize = 0x42; break;
    default   : return -1;
  }

  BufSelector = fd32_dosmem_get(Ext.BufSize, &BufSegment, &BufOffset);
  fd32_memcpy_to_lowmem(BufSelector, 0, &Ext, Ext.BufSize);
  /* IBM/MS INT 13 Extensions - GET DRIVE PARAMETERS */
  AH(Regs) = 0x48;
  DL(Regs) = Drive;
  DS(Regs) = BufSegment;
  SI(Regs) = BufOffset;
  fd32_int(0x13, Regs);
  if (FLAGS(Regs) & 0x1)
  {
    fd32_dosmem_free(BufSelector, Ext.BufSize);
    return -1;
  }

  fd32_memcpy_from_lowmem(&Ext, BufSelector, 0, Ext.BufSize);
  fd32_dosmem_free(BufSelector, Ext.BufSize);
  D->PrivFlags   = EXTAVAIL;
  D->PhysC       = Ext.PhysC;
  D->PhysH       = Ext.PhysH;
  D->PhysS       = Ext.PhysS;
  D->BlockSize   = Ext.BytesPerSector;
  D->TotalBlocks = Ext.TotalSectors;

  /* Check if Device Parameter Table Extension is available (RBIL #00278) */
  if ((Chk.Version >= 0x20) && (Chk.ApiSupport & 0x04)
   && (Ext.EddParams != 0xFFFFFFFF))
  {
    fd32_memcpy_from_lowmem(&Edd, DOS_SELECTOR, ((Ext.EddParams >> 16) << 4)
                            + (WORD) Ext.EddParams, sizeof(tEddParams));
    /* If Device Parameter Table Extension available detect disk channel */
    switch (Edd.BasePort & 0xFFF0)
    {
      case 0x1F0: D->PrivFlags |= KNOWSPORTS;
                  if (Edd.Flags & 0x10) D->PrivFlags |= SLAVE;
                  break;
      case 0x170: D->PrivFlags |= KNOWSPORTS | SECONDARY;
                  if (Edd.Flags & 0x10) D->PrivFlags |= SLAVE;
                  break;
      default:
        #ifdef BIOSDISK_SHOWMSG
        fd32_message("Unknown base port!\n");
        #endif
    }
  }
  return 0;
}


#ifdef BIOSDISK_SHOWMSG
static const char *FloppyTypes[] = { "unknown", "360 KiB", "1200 KiB",
                                     "720 KiB", "1440 KiB", "unknown",
                                     "2880 KiB" };
#endif

static int detect_floppies()
{
  #ifdef BIOSDISK_FD32DEV
  char          *DevName;
  #endif
  tDisk         *D;
  tStdDriveInfo  Std;
  char           Name[10];
  int            Res;
  int            Disk = 0;

  do
  {
    if ((Res = std_driveinfo(Disk, &Std)))
    {
      #ifdef BIOSDISK_SHOWMSG
      fd32_message("Error %i getting parameters for drive %xh\n", Res, Disk);
      #endif
      return -1;
    }
    if (Std.NumberOfDrives == 0) break;

    /* Allocate memory for the private data, ops and device structures */
    if ((D = (tDisk *) fd32_kmem_get(sizeof(tDisk))) == NULL)
      return FD32_ENOMEM;
    ksprintf(Name, "fd%d", Disk);

    /* Initialize the private data */
    D->OpenCount   = 0;
    D->FirstSector = 0;
    D->BiosNumber  = Std.BiosNumber;
    D->PrivFlags   = 0;
    D->BiosC       = D->PhysC = Std.Cylinders;
    D->BiosH       = D->PhysH = Std.Heads;
    D->BiosS       = D->PhysS = Std.Sectors;
    D->BlockSize   = 512;
    D->TotalBlocks = Std.Cylinders * Std.Heads * Std.Sectors;
    D->Type        = FD32_BIFLO;
    D->MultiBootId = 0x00FFFFFF | (Disk << 24);

    /* Initialize and add removable block operations */
    switch (get_disk_type(Std.BiosNumber))
    {
      case 1  : D->PrivFlags = REMOVABLE; break;
      case 2  : D->PrivFlags = REMOVABLE | CHANGELINE; break;
      default : fd32_kmem_free(D, sizeof(tDisk)); return -1;
    }
    #ifdef BIOSDISK_SHOWMSG
    fd32_message("BIOS drive %02lxh is %s (%s)\n", Std.BiosNumber, Name,
                 (Std.DriveType <= 6) ? FloppyTypes[Std.DriveType] : "unknown");
    #endif

    #ifdef BIOSDISK_FD32DEV
    /* Register the new device to the FD32 kernel */
    DevName = (char *) fd32_kmem_get(strlen(Name) + 1);
    if (DevName == NULL) return FD32_ENOMEM;
    strcpy(DevName, Name);
    Res = fd32_dev_register(biosdisk_request, (void *) D, DevName);
    if (Res < 0) return Res;
    #endif
  }
  while (++Disk < Std.NumberOfDrives);
  return 0;
}


static int detect_harddisks()
{
  #ifdef BIOSDISK_FD32DEV
  char          *DevName;
  #endif
  tDisk         *D;
  tStdDriveInfo  Std;
  char           Name[4] = "hd ";
  int            Res;
  int            Disk = 0x80;

  do
  {
    if ((Res = std_driveinfo(Disk, &Std)))
    {
      #ifdef BIOSDISK_SHOWMSG
      fd32_message("Error %i getting parameters for drive %xh\n", Res, Disk);
      #endif
      return -1;
    }
    if (Std.NumberOfDrives == 0) break;

    if ((D = (tDisk *) fd32_kmem_get(sizeof(tDisk))) == NULL)
      return FD32_ENOMEM;

    /* Initialize the private data */
    D->OpenCount   = 0;
    D->FirstSector = 0;
    D->BiosNumber  = Std.BiosNumber;
    D->BiosC       = Std.Cylinders;
    D->BiosH       = Std.Heads;
    D->BiosS       = Std.Sectors;
    D->Type        = FD32_BIGEN;
    D->MultiBootId = 0x00FFFFFF | (Disk << 24);
    if (ext_driveinfo(Disk, D))
    {
      /* If extensions are not available, use standard BIOS values */
      D->PrivFlags   = 0;
      D->PhysC       = Std.Cylinders;
      D->PhysH       = Std.Heads;
      D->PhysS       = Std.Sectors;
      D->BlockSize   = 512;
      D->TotalBlocks = Std.Cylinders * Std.Heads * Std.Sectors;
    }

    /* Complete the device name */
    if (D->PrivFlags & KNOWSPORTS)
    {
      Name[2] = 'a';
      if (D->PrivFlags & SECONDARY) Name[2] = 'c';
      if (D->PrivFlags & SLAVE)     Name[2]++;
    }
    else Name[2] = Disk - 0x80 + 'a';

    #ifdef BIOSDISK_SHOWMSG
    if (D->PrivFlags & EXTAVAIL)
    {
      if (D->PrivFlags & KNOWSPORTS)
        fd32_message("BIOS drive %02lxh is %s (%lu MiB) supports LBA and knows ports\n",
                     D->BiosNumber, Name, D->TotalBlocks * D->BlockSize / 1048576);
       else
        fd32_message("BIOS drive %02lxh is %s (%lu MiB) supports LBA\n",
                     D->BiosNumber, Name, D->TotalBlocks * D->BlockSize / 1048576);
    }
    else
      fd32_message("BIOS drive %02lxh is %s (%lu MiB)\n",
                   D->BiosNumber, Name, D->TotalBlocks * D->BlockSize / 1048576);
    #endif

    #ifdef BIOSDISK_FD32DEV
    /* Register the new device to the FD32 kernel */
    DevName = (char *) fd32_kmem_get(strlen(Name) + 1);
    if (DevName == NULL) return FD32_ENOMEM;
    strcpy(DevName, Name);
    Res = fd32_dev_register(biosdisk_request, (void *) D, DevName);
    if (Res < 0) return Res;
    #endif
    biosdisk_scanpart(D, DevName);
  }
  while (++Disk - 0x80 < Std.NumberOfDrives);
  return 0;
}


int biosdisk_detect(void)
{
  int Res;
  if ((Res = detect_floppies()) < 0) return Res;
  if ((Res = detect_harddisks()) < 0) return Res;
  return 0;
}


#if 0
int main()
{
  return biosdisk_detect();
}
#endif
