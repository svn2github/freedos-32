/****************************************************************************/
/* FD32 FAT Driver test program "creat.c" by Salvo Isaja                    */
/* This program creates a new empty file, or replaces an existing one.      */
/* Disclaimer: The FAT Driver might have bugs that could corrupt your data. */
/*             Test it AT YOUR OWN RISK.                                    */
/****************************************************************************/

#include <dr-env.h>
#include <devices.h>       /* For driver interfaces              */
#include <filesys.h>       /* For opening modes and seek origins */

/* Set the following to the file system device you like          */
#define DEVNAME "hda1"
/* Set it to the canonicalized name of the file you want to open */
#define FILENAME "\\tmp\\newfile.txt"


/* We need this because we suppose we want to use only FAT as file system. */
/* If we wanted to try all file system drivers available to mount volume,  */
/* we couldn't use the FAT driver directly, but we'd need the FS Layer.    */
extern fd32_request_t fat_request;


/* Closes a file and unmounts the FAT volume.                      */
/* This is called only once, but I've edited another test file :-) */
static int close_and_unmount(void *FileId, void *FsDev)
{
  fd32_close_t   C;
  fd32_unmount_t Um;
  int            Res;

  /* Close the file */
  C.Size     = sizeof(fd32_close_t);
  C.DeviceId = FileId;
  if ((Res = fat_request(FD32_CLOSE, &C)) < 0)
    message("Error %08xh while closing the file\n", Res);
  /* And unmouunt the FAT volume */
  Um.Size     = sizeof(fd32_unmount_t);
  Um.DeviceId = FsDev;
  if ((Res = fat_request(FD32_UNMOUNT, &Um)) < 0)
    message("Error %08xh while unmounting the FAT volume\n", Res);
  return Res;
}


/* The FAT test entry point, called by the FD32 kernel. */
int fat_creattest_init(void)
{
  fd32_mount_t    M;
  fd32_openfile_t Of;
  int             hDev, Res;

  /* Find device */
  if ((hDev = fd32_dev_search(DEVNAME)) < 0)
  {
    message("Couldn't find device: %08xh\n", hDev);
    return hDev;
  }
  /* Mount a FAT volume on the device */
  M.Size = sizeof(fd32_mount_t);
  M.hDev = hDev;
  if ((Res = fat_request(FD32_MOUNT, &M)) < 0)
  {
    message("Couldn't get data for device: %08xh\n", Res);
    return Res;
  }
  /* Open file */
  Of.Size      = sizeof(fd32_openfile_t);
  Of.DeviceId  = M.FsDev; /* Returned by the FD32_MOUNT request */
  Of.FileName  = FILENAME;
  Of.Mode      = FD32_OWRITE | FD32_OCREAT | FD32_OTRUNC;
  if ((Res = fat_request(FD32_OPENFILE, &Of)) < 0)
  {
    message("Error %08xh while opening the file\n", Res);
    return Res;
  }
  /* Finally close the file and unmount the FAT volume */
  return close_and_unmount(Of.FileId, M.FsDev);
}
