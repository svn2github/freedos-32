/**************************************************************************
 * FreeDOS32 File System Layer                                            *
 * Wrappers for file system driver functions and JFT support              *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2002-2005, Salvatore Isaja                               *
 *                                                                        *
 * This is "drives.c" - Services to handle drives and assign letters.     *
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
#include <ll/i386/stdlib.h>
#include <ll/i386/error.h>
#include <ll/ctype.h>

//#include <devices.h>
#include <dr-env.h> /* temp hack for uint??_t */
#include <block/block.h>
#include <errno.h>
#include <filesys.h>

#define NULBOOTDEV 0xFFFFFFFF
#define DRIVE_MAX_NUM 26

//#define DRIVES_FIXED /* Define this to enable fixed letter assignment */
#ifdef DRIVES_FIXED
 #error "Fixed drive letter assignment is broken"
#endif

typedef struct Drive
{
	const char *blk_name;
	fd32_request_t *req;
	void *handle;
}
Drive;

static Drive drives[DRIVE_MAX_NUM];

static DWORD BootDevice   = NULBOOTDEV; /* From MultiBoot info */
static char  DefaultDrive = 0; /* Drive letter, or 0 if not defined */
#ifdef DRIVES_FIXED
static char *FixedLetters[DRIVE_MAX_NUM];
#endif

/* Registered file system driver request functions */
/* TODO: Make the size of this dynamic! */
static int             NumFSReq = 20;
static fd32_request_t *fsreq[20];


/* Assign drive letters dynamically, searching for block devices of the 
 * specified type, as enumerated, starting from the passed drive number.
 * Returns 0 on success, or a negative error code on failure.
 */
static int dynamic_assign(unsigned type, int d)
{
	BlockDeviceInfo  bdi;
	BlockOperations *bops;
	void            *iterator = NULL;
	const char      *name;
	void            *handle;
	unsigned         partid;
	int              res;

	while ((name = block_enumerate(&iterator)) != NULL)
	{
		res = block_get(name, BLOCK_OPERATIONS_TYPE, (void **)&bops, &handle);
		if (res < 0) continue;
		res = bops->get_device_info(handle, &bdi);
		bops->request(REQ_RELEASE);
		if (res < 0) continue;
		/* Check if this device is already assigned */
		for (res = 0; res < DRIVE_MAX_NUM; res++)
			if (drives[res].handle == handle) break;
		if (res != DRIVE_MAX_NUM) continue;
		/* If the block device type is not what we are searching we skip it */
		if ((bdi.flags & BLOCK_DEVICE_INFO_TYPEMASK) != type) continue;
		/* If no file system driver can handle such a partition we skip it */
		partid = bdi.flags & BLOCK_DEVICE_INFO_PARTID;
		if (partid)
		{
			fd32_partcheck_t p;
			p.Size = sizeof(fd32_partcheck_t);
			p.PartId = partid;
			for (res = 0; res < NumFSReq; res++)
				if ((fsreq[res]) && (fsreq[res](FD32_PARTCHECK, &p) == 0))
					break;
			if (res == NumFSReq) continue;
		}
		/* Assign the next available drive number (0 is 'A') to the device */
		while (drives[d].blk_name) if (d++ == 'Z' - 'A') return -ENODEV;
		drives[d].blk_name = name;
		drives[d].req = NULL;
		drives[d].handle = NULL;
		message("FS Layer: '%c' drive assigned to device '%s'\n", d + 'A', name);
		/* Set the default drive to this drive if boot device is known,
		 * otherwise set the default drive to the first drive detected. */
		if (!DefaultDrive)
			if ((BootDevice == NULBOOTDEV) || (BootDevice == bdi.multiboot_id))
				DefaultDrive = d + 'A';
	}
	return 0;
}


static int assign_drive_letters(void)
{
  #ifdef DRIVES_FIXED
  int hDev;
  int k;
  #endif

  /* Initialize the Drives array with all drives unassigned */
  memset(drives, 0, sizeof(drives));

  #ifdef DRIVES_FIXED
  /* Assign fixed drive letters */
  memset(FixedLetters, 0, sizeof(FixedLetters));
//  FixedLetters['C' - 'A'] = "hda5";
//  FixedLetters['D' - 'A'] = "hda5";
  for (k = 0; k < DRIVE_MAX_NUM; k++)
    if (FixedLetters[k])
    {
      if ((hDev = fd32_dev_search(FixedLetters[k])) < 0)
        message("Warning: drive %c can not be %s, device not found",
                     k + 'A', FixedLetters[k]);
       else
        /* TODO: Should check if hDev is a valid block device! */
        Drives[k] = hDev;
    }
  #endif

  /* Assign remaining drive letters according to the Win2000 behaviour */
  dynamic_assign(BLOCK_DEVICE_INFO_TACTIVE,  'C' - 'A');
  dynamic_assign(BLOCK_DEVICE_INFO_TLOGICAL, 'C' - 'A');
  dynamic_assign(BLOCK_DEVICE_INFO_TPRIMARY, 'C' - 'A');
  dynamic_assign(BLOCK_DEVICE_INFO_TFLOPPY,  'A' - 'A');
  dynamic_assign(BLOCK_DEVICE_INFO_TCDDVD,   'D' - 'A');
  return 0;
}


/* Gets the file system driver request funciton and the file system device
 * identifier for the drive specified in a file name, if present, or to the
 * default drive letter if not.
 * If the device is not mounted the fd32_mount function is called in order
 * to try to mount a file system.
 * On success, returns the detected drive letter (if defined, 0 if not),
 * fills request with the pointer to the file system driver request function,
 * DeviceId with the file system device identifier, and Path with a pointer
 * to the beginning of the path, removing the drive specification.
 * Returns a negative error code on failure.
 */
int fd32_get_drive(char *FileSpec, fd32_request_t **request, void **DeviceId, char **Path)
{
	int res;
	int letter = DefaultDrive;
	Drive *drive;
	char *colon = strchr(FileSpec, ':');

	if (Path) *Path = FileSpec;
	if (colon)
	{
		if (colon != FileSpec + 1) return -ENODEV;
		letter = toupper(FileSpec[0]);
		if ((letter < 'A') || (letter > 'Z')) return -ENODEV;
		if (Path) *Path = colon + 1;
	}
	drive = &drives[letter - 'A'];
	if (!drive->blk_name) return -ENODEV;
	if (!drive->req)
	{
		int k;
		fd32_mount_t m;
		m.Size   = sizeof(fd32_mount_t);
		m.BlkDev = drive->blk_name;
		for (k = 0; k < NumFSReq; k++)
			if (fsreq[k])
			{
				res = fsreq[k](FD32_MOUNT, &m);
				if (res == 0)
				{
					drive->req = fsreq[k];
					drive->handle = m.FsDev;
					break;
				}
				if (res != -ENODEV /* Unknown FS */) return res;
			}
		if (k == NumFSReq) return -ENODEV; /* Unknown FS */
	}
	*request  = drive->req;
	*DeviceId = drive->handle;
	return letter;
}


#if __FILENAME_WITHOUT_DRIVE__
/* Returns the file name without the drive specification part. */
/* At present it is useless.                                   */
static char *get_name_without_drive(char *FileName)
{
  char *s;
  for (s = FileName; *s; s++)
    if (*s == ':') return s + 1;
  return FileName;
}
#endif


/* Get the drive letter from device name */
char fd32_get_drive_letter(char *device_name)
{
	int i;
	for (i = 0; i < DRIVE_MAX_NUM; i++)
		if (strcmp(device_name, drives[i].blk_name) == 0)
			return 'A'+i;

	return 0;
}


/* The SET DEFAULT DRIVE system call.                 */
/* Returns the number of available drives on success, */
/* or a negative error code on failure.               */
int fd32_set_default_drive(char Drive)
{
  /* TODO: LASTDRIVE should be read frm Config.sys */
  if ((toupper(Drive) < 'A') || (toupper(Drive) > 'Z')) return -ENODEV;
  if (drives[Drive - 'A'].blk_name != NULL)
    DefaultDrive = toupper(Drive);
  /* TODO: From the RBIL (INT 21h, AH=0Eh)
           under DOS 3.0+, the return value is the greatest of 5,
           the value of LASTDRIVE= in CONFIG.SYS, and the number of
           drives actually present. */
  return 'Z' - 'A' + 1;
}


/* The GET DEFAULT DRIVE system call.               */
/* Returns the letter of the current default drive. */
char fd32_get_default_drive()
{
  return DefaultDrive;
}


/* Registers a new file system driver request function to the FS Layer */
int fd32_add_fs(fd32_request_t *request)
{
  int k;
  for (k = 0; k < NumFSReq; k++)
    if (!fsreq[k])
    {
      fsreq[k] = request;
      return assign_drive_letters();
    }
  return -ENOMEM;
}


void fd32_set_boot_device(DWORD MultiBootId)
{
  BootDevice = MultiBootId;
}
