/****************************************************************************/
/* FD32 FAT Driver test program "unlink.c" by Salvo Isaja                   */
/* This program deletes a file.                                             */
/* Disclaimer: The FAT Driver might have bugs that could corrupt your data. */
/*             Test it AT YOUR OWN RISK.                                    */
/****************************************************************************/

#include <dr-env.h>
#include <devices.h>       /* For driver interfaces              */
#include <filesys.h>       /* For opening modes and seek origins */

/* Set the following to the file system device you like            */
#define DEVNAME "hda1"
/* Set it to the canonicalized name of the file you want to delete */
#define FILENAME "\\tmp\\newfile.txt"


/* We need this because we suppose we want to use only FAT as file system. */
/* If we wanted to try all file system drivers available to mount volume,  */
/* we couldn't use the FAT driver directly, but we'd need the FS Layer.    */
extern fd32_request_t fat_request;


/* The FAT test entry point, called by the FD32 kernel. */
int fat_unlinktest_init(void)
{
  fd32_mount_t   M;
  fd32_unlink_t  U;
  fd32_unmount_t Um;
  int            hDev, Res;

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
  /* Unlink the file */
  U.Size     = sizeof(fd32_unlink_t);
  U.DeviceId = M.FsDev; /* Returned by the FD32_MOUNT request */
  U.FileName = FILENAME;
  U.Flags    = FD32_FAALL | FD32_FRNONE; /* Don't need attribute specification */
  if ((Res = fat_request(FD32_UNLINK, &U)) < 0)
  {
    message("Error %08xh while unlinking the file\n", Res);
    return Res;
  }
  /* Finally unmount the FAT volume */
  Um.Size     = sizeof(fd32_unmount_t);
  Um.DeviceId = M.FsDev;
  if ((Res = fat_request(FD32_UNMOUNT, &Um)) < 0)
    message("Error %08xh while unmounting the FAT volume\n", Res);
  return Res;
}
