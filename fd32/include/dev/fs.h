#ifndef __FD32_DEV_FS_H
#define __FD32_DEV_FS_H

#include <ll/i386/hw-data.h>
#include <ll/i386/stdlib.h>
#include <devices.h>
#include <filesys.h>


/* Ops mnemonics */
#define FD32_DEV_FSVOL 1 /* File system volume */
#define FD32_DEV_FSDRV 2 /* File system driver */


/* Operations structure for "File system driver" devices */
typedef struct
{
  fd32_dev_ops_t *Next;
  DWORD           Type;
  void           *P;
  int           (*free_ops)(fd32_dev_ops_t *Ops);

  int (*mount)(char *DevName, BYTE PartSig);
}
fd32_dev_fsdrv_t;


/* Operations structure for "File system volume" devices */
typedef struct
{
  fd32_dev_ops_t *Next;
  DWORD           Type;
  void           *P;
  int           (*free_ops)(fd32_dev_ops_t *Ops);

  int (*open)         (void *P, char *FileName, DWORD Mode, WORD Attr,
                       WORD AliasHint, int *Result, int UseLfn);
  int (*close)        (void *P, int Fd);
  int (*fflush)       (void *P, int Fd);
  int (*lseek)        (void *P, int Fd, SQWORD Offset, int Origin,
                       SQWORD *Result);
  int (*read)         (void *P, int Fd, BYTE *Buffer, DWORD Size,
                       DWORD *Result);
  int (*write)        (void *P, int Fd, BYTE *Buffer, DWORD Size,
                       DWORD *Result);
  int (*get_attr)     (void *P, int Fd, fd32_dev_fs_attr_t *A);
  int (*set_attr)     (void *P, int Fd, fd32_dev_fs_attr_t *A);
  int (*lfn_findfirst)(void *P, char *FileSpec, DWORD Flags,
                       fd32_fs_lfnfind_t *FindData);
  int (*lfn_findnext) (void *P, int Fd, DWORD Flags,
                       fd32_fs_lfnfind_t *FindData);
  int (*lfn_findclose)(void *P, int Fd);
  int (*dos_findfirst)(void *P, char *FileSpec, BYTE Attributes,
                       fd32_fs_dosfind_t *Dta);
  int (*dos_findnext) (fd32_fs_dosfind_t *Dta);
  int (*mkdir)        (void *P, char *DirName, int UseLfn);
  int (*rmdir)        (void *P, char *DirName, int UseLfn);
  int (*unlink)       (void *P, char *FileName, DWORD Flags, int UseLfn);
  int (*rename)       (void *P, char *OldName, char *NewName, int UseLfn);
} 
fd32_dev_fsvol_t;


#endif /* #ifndef __FD32_DEV_FS_H */

