/**************************************************************************
 * FreeDOS32 File System Layer                                            *
 * Wrappers for file system driver functions, SFT and JFT support         *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2002, Salvatore Isaja                                    *
 *                                                                        *
 * The FreeDOS32 File System Layer is part of FreeDOS32.                  *
 *                                                                        *
 * FreeDOS32 is free software; you can redistribute it and/or modify it   *
 * under the terms of the GNU General Public License as published by the  *
 * Free Software Foundation; either version 2 of the License, or (at your *
 * option) any later version.                                             *
 *                                                                        *
 * FreeDOS32 is distributed in the hope that it will be useful, but       *
 * WITHOUT ANY WARRANTY; without even the implied warranty                *
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with FreeDOS32; see the file COPYING; if not, write to the       *
 * Free Software Foundation, Inc.,                                        *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA                *
 **************************************************************************/

#ifndef __FD32_FILESYS_H
#define __FD32_FILESYS_H

#include <devices.h>


/* Error codes */
#define FD32_ERROR_INVALID_FUNCTION    -0x01
#define FD32_ERROR_FILE_NOT_FOUND      -0x02
#define FD32_ERROR_PATH_NOT_FOUND      -0x03
#define FD32_ERROR_TOO_MANY_OPEN_FILES -0x04
#define FD32_ERROR_ACCESS_DENIED       -0x05
#define FD32_ERROR_INVALID_HANDLE      -0x06
#define FD32_ERROR_FORMAT_INVALID      -0x0B
#define FD32_ERROR_ACCESS_CODE_INVALID -0x0C
#define FD32_ERROR_DATA_INVALID        -0x0D
#define FD32_ERROR_INVALID_DRIVE       -0x0F
#define FD32_ERROR_RMDIR_CURRENT       -0x10
#define FD32_ERROR_NOT_SAME_DEVICE     -0x11
#define FD32_ERROR_NO_MORE_FILES       -0x12
#define FD32_ERROR_WRITE_PROTECTED     -0x13
#define FD32_ERROR_UNKNOWN_UNIT        -0x14
#define FD32_ERROR_DRIVE_NOT_READY     -0x15
#define FD32_ERROR_CRC_ERROR           -0x17
#define FD32_ERROR_INVALID_SEEK        -0x19
#define FD32_ERROR_UNKNOWN_MEDIA       -0x1A
#define FD32_ERROR_SECTOR_NOT_FOUND    -0x1B
#define FD32_ERROR_WRITE_FAULT         -0x1D
#define FD32_ERROR_READ_FAULT          -0x1E
#define FD32_ERROR_GENERAL_FAILURE     -0x1F
#define FD32_ERROR_SHARING_VIOLATION   -0x20
#define FD32_ERROR_LOCK_VIOLATION      -0x21
#define FD32_ERROR_INVALID_DISK_CHANGE -0x22 /* (ES:DI -> media ID structure)(see #1546) */
#define FD32_ERROR_OUT_OF_INPUT        -0x26
#define FD32_ERROR_DISK_FULL           -0x27
#define FD32_ERROR_FILE_EXISTS         -0x50
#define FD32_ERROR_CANNOT_MAKE_DIR     -0x52
#define FD32_ERROR_FAIL_ON_INT24       -0x53
#define FD32_ERROR_NOT_LOCKED          -0xB0
#define FD32_ERROR_LOCKED_IN_DRIVE     -0xB1
#define FD32_ERROR_NOT_REMOVABLE       -0xB2
#define FD32_ERROR_LOCK_COUNT_EXCEEDED -0xB4
#define FD32_ERROR_EJECT_FAILED        -0xB5


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* File and path names limits, in bytes, including the NULL terminator */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#define FD32_MAX_LFN_PATH_LENGTH 260
#define FD32_MAX_LFN_LENGTH      255
#define FD32_MAX_SFN_PATH_LENGTH 64
#define FD32_MAX_SFN_LENGTH      14


/* * * * * * * * * * * * * * * * * * * */
/* Mnemonics for the UseLfn parameters */
/* * * * * * * * * * * * * * * * * * * */
#define FD32_NOLFN 0
#define FD32_LFN   1


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* OPEN system call - Flags for opening mode and action taken  */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Access codes */
#define FD32_OPEN_ACCESS_MASK      0x0007 /* Bit mask for access type       */
#define FD32_OPEN_ACCESS_READ      0x0000 /* Allow only reads from file     */
#define FD32_OPEN_ACCESS_WRITE     0x0001 /* Allow only writes into file    */
#define FD32_OPEN_ACCESS_READWRITE 0x0002 /* Allow both reads and writes    */
#define FD32_OPEN_ACCESS_READ_NA   0x0004 /* Read only without updating the */
                                          /* last access date               */
/* Sharing modes and inheritance */
#define FD32_OPEN_SHARE_MASK       0x0070 /* Bit mask for sharing type      */
#define FD32_OPEN_SHARE_COMPAT     0x0000 /* Compatibility mode             */
#define FD32_OPEN_SHARE_DENYALL    0x0010 /* Deny r/w by other handles      */
#define FD32_OPEN_SHARE_DENYWRITE  0x0020 /* Deny write by other handles    */
#define FD32_OPEN_SHARE_DENYREAD   0x0030 /* Deny read by other handles     */
#define FD32_OPEN_SHARE_DENYNONE   0x0040 /* Allow full access by others    */
#define FD32_OPEN_NO_INHERIT       0x0080 /* Child processes will not       */
                                          /* inherit the file               */
/* Extended LFN open flags */
#define FD32_OPEN_NO_BUFFERS     (1 << 8)  /* Do not use bufferized I/O      */
#define FD32_OPEN_NO_COMPRESS    (1 << 9)  /* Do not compress files (N/A)    */
#define FD32_OPEN_USE_ALIAS_HINT (1 << 10) /* Use the numeric hint for alias */
#define FD32_OPEN_NO_INT24       (1 << 13) /* Do not generate INT 24 on fail */
#define FD32_OPEN_ALWAYS_COMMIT  (1 << 14) /* Commit file at every write     */
/* Action to take */
#define FD32_OPEN_EXIST_OPEN      (1 << 16) /* Open existing file     */
#define FD32_OPEN_EXIST_REPLACE   (1 << 17) /* Truncate existing file */
#define FD32_OPEN_NOTEXIST_CREATE (1 << 20) /* Create unexisting file */
//#define FD32_OPEN_USE_LFN       0x010000 /* Use long file names           */
//#define FD32_OPEN_DIRECTORY     0x100000 /* Open a directory as a file    */

/* Return codes for the action taken by the open system call */
#define FD32_OPEN_RESULT_OPENED   1
#define FD32_OPEN_RESULT_CREATED  2
#define FD32_OPEN_RESULT_REPLACED 3


/* * * * * * * * * * * * * * * * * * * * * * * * */
/* CHMOD system call - Flags for file attributes */
/* * * * * * * * * * * * * * * * * * * * * * * * */
#define FD32_ATTR_READONLY  0x01
#define FD32_ATTR_HIDDEN    0x02
#define FD32_ATTR_SYSTEM    0x04
#define FD32_ATTR_VOLUMEID  0x08
#define FD32_ATTR_DIRECTORY 0x10
#define FD32_ATTR_ARCHIVE   0x20
#define FD32_ATTR_LONGNAME  0x0F /* ReadOnly + Hidden + System + VolumeId */
#define FD32_ATTR_ALL       0x3F
#define FD32_ATTR_NONE      0x00


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Search flags for FINDFIRST/FINDNEXT and UNLINK system calls */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Allowable attributes */
#define FD32_FIND_ALLOWABLE_MASK  0x00FF
#define FD32_FIND_ALLOW_READONLY  0x0001
#define FD32_FIND_ALLOW_HIDDEN    0x0002
#define FD32_FIND_ALLOW_SYSTEM    0x0004
#define FD32_FIND_ALLOW_VOLUMEID  0x0008
#define FD32_FIND_ALLOW_DIRECTORY 0x0010
#define FD32_FIND_ALLOW_ARCHIVE   0x0020
#define FD32_FIND_ALLOW_ALL       0x003F
#define FD32_FIND_ALLOW_NONE      0x0000
/* Required attributes */
#define FD32_FIND_REQUIRED_MASK     0xFF00
#define FD32_FIND_REQUIRE_READONLY  0x0100
#define FD32_FIND_REQUIRE_HIDDEN    0x0200
#define FD32_FIND_REQUIRE_SYSTEM    0x0400
#define FD32_FIND_REQUIRE_VOLUMEID  0x0800
#define FD32_FIND_REQUIRE_DIRECTORY 0x1000
#define FD32_FIND_REQUIRE_ARCHIVE   0x2000
#define FD32_FIND_REQUIRE_ALL       0x3F00
#define FD32_FIND_REQUIRE_NONE      0x0000
/* Other search flags */
#define FD32_FIND_USE_DOS_DATE    (1 << 16)
#define FD32_FIND_ALLOW_WILDCARDS (1 << 17)


/* * * * * * * * * * * * * * * * * * * * * * */
/* LSEEK system call - Mnemonics for origins */
/* * * * * * * * * * * * * * * * * * * * * * */
#define FD32_SEEK_SET 0 /* Seek from the beginning of the file */
#define FD32_SEEK_CUR 1 /* Seek from the current file position */
#define FD32_SEEK_END 2 /* Seek from the end of file           */


/* * * * * * * * * */
/* Data structures */
/* * * * * * * * * */


/* Parameters structure for "get/set attributes" functions */
typedef struct
{
  WORD Attr;  /* Standard DOS file attributes mask              */
  WORD MDate; /* Last modification date in DOS format           */
  WORD MTime; /* Last modification time in DOS format           */
  WORD ADate; /* Last access date in DOS format                 */
  WORD CDate; /* File creation date in DOS format               */
  WORD CTime; /* File creation time in DOS format               */
  WORD CHund; /* Hundredths of second of the file creation time */
}
fd32_dev_fs_attr_t;


/* Format of the finddata record for LFN search operations */
typedef struct
{
  DWORD Attr;          /* File attributes                                 */
  QWORD CTime;         /* File creation time in Win32 or DOS format       */
  QWORD ATime;         /* Last access time in Win32 or DOS format         */
  QWORD MTime;         /* Last modification time in Win32 or DOS format   */
  DWORD SizeHi;        /* High 32 bits of the file size in bytes          */
  DWORD SizeLo;        /* Low 32 bits of the file size in bytes           */
  BYTE  Reserved[8];   /* Reserved bytes, must be ignored                 */
  char  LongName[260]; /* Null-terminated long file name in Unicode UTF-8 */
  char  ShortName[14]; /* Null-terminated short file name in ASCII        */
}
__attribute__ ((packed)) fd32_fs_lfnfind_t;


/* Format of the finddata record and search status for DOS style search */
typedef struct
{
  BYTE              Reserved0[17]; /* Can be used to store the status      */
  void             *Ops;           /* File system device where to search   */
  BYTE              Attr;          /* File attributes                      */
  WORD              MTime;         /* Last modification time in DOS format */
  WORD              MDate;         /* Last modification date in DOS format */
  DWORD             Size;          /* File size in bytes                   */
  char              Name[13];      /* ASCIZ short file name found          */
  BYTE              Reserved1[85]; /* Can be used to store the status      */
}
__attribute__ ((packed)) fd32_fs_dosfind_t;


/* * * * * * * * * * * */
/* Function prototypes */
/* * * * * * * * * * * */
int  fd32_mkdir            (char *DirName, int UseLfn);
int  fd32_rmdir            (char *DirName, int UseLfn);
int  fd32_open             (char *FileName, DWORD Mode, WORD Attr, WORD AliasHint, int *Result, int UseLfn);
int  fd32_close            (int Handle);
int  fd32_fflush           (int Handle);
int  fd32_read             (int Handle, void *Buffer, DWORD Size, DWORD *Result);
int  fd32_write            (int Handle, void *Buffer, DWORD Size, DWORD *Result);
int  fd32_unlink           (char *FileName, DWORD Flags, int UseLfn);
int  fd32_lseek            (int Handle, SQWORD Offset, int Origin, SQWORD *Result);
int  fd32_dup              (int Handle);
int  fd32_forcedup         (int Handle, int NewHandle);
int  fd32_get_attributes   (int Handle, fd32_dev_fs_attr_t *A);
int  fd32_set_attributes   (int Handle, fd32_dev_fs_attr_t *A);
int  fd32_dos_findfirst    (char *FileSpec, BYTE Attrib, fd32_fs_dosfind_t *Dta);
int  fd32_dos_findnext     (fd32_fs_dosfind_t *Dta);
int  fd32_lfn_findfirst    (char *FileSpec, DWORD Flags, fd32_fs_lfnfind_t *FindData);
int  fd32_lfn_findnext     (int Handle, DWORD Flags, fd32_fs_lfnfind_t *FindData);
int  fd32_lfn_findclose    (int Handle);
int  fd32_rename           (char *OldName, char *NewName, int UseLfn);

int fd32_add_drive(char Drive, char *Device);

#endif /* ifndef __FD32_FILESYS_H */

