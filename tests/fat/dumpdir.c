/****************************************************************************/
/* FD32 FAT Driver test program "dumpdir.c" by Salvo Isaja                  */
/* This program reads the content a directory providing a raw dump.         */
/* Disclaimer: The FAT Driver might have bugs that could corrupt your data. */
/*             Test it AT YOUR OWN RISK.                                    */
/****************************************************************************/

#include <ll/i386/error.h> /* For the message function                  */
#include <devices.h>       /* For driver interfaces                     */
#include <filesys.h>       /* For opening modes and finddata structure  */
#include <errors.h>        /* For FD32_ENMFILE readdir termination code */

/* Set the following to the file system device you like          */
#define DEVNAME "hda1"
/* Set it to the canonicalized name of the file you want to open */
#define DIRNAME "\\tmp"


/* We need this because we suppose we want to use only FAT as file system. */
/* If we wanted to try all file system drivers available to mount volume,  */
/* we couldn't use the FAT driver directly, but we'd need the FS Layer.    */
extern fd32_request_t fat_request;


/* Closes a file and unmounts the FAT volume.              */
/* This is called several times, let's make it a function. */
static int close_and_unmount(void *FileId, void *FsDev)
{
  fd32_close_t   C;
  fd32_unmount_t Um;
  int            Res;

  /* Close the file */
  C.Size     = sizeof(fd32_close_t);
  C.DeviceId = FileId;
  if ((Res = fat_request(FD32_CLOSE, &C)) < 0)
    message("Error %08xh while closing the directory\n", Res);
  /* And unmouunt the FAT volume */
  Um.Size     = sizeof(fd32_unmount_t);
  Um.DeviceId = FsDev;
  if ((Res = fat_request(FD32_UNMOUNT, &Um)) < 0)
    message("Error %08xh while unmounting the FAT volume\n", Res);
  return Res;
}


/* The FAT test entry point, called by the FD32 kernel. */
int fat_dumpdirtest_init(void)
{
  fd32_mount_t    M;
  fd32_openfile_t Of;
  fd32_read_t     R;
  BYTE            Buf[32];
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
  /* Open directory */
  Of.Size      = sizeof(fd32_openfile_t);
  Of.DeviceId  = M.FsDev; /* Returned by the FD32_MOUNT request */
  Of.FileName  = DIRNAME;
  Of.Mode      = FD32_OREAD | FD32_OEXIST | FD32_ODIR;
  if ((Res = fat_request(FD32_OPENFILE, &Of)) < 0)
  {
    message("Error %08xh while opening the directory\n", Res);
    return Res;
  }
  /* Read directory entries in the finddata structure F */
  R.Size        = sizeof(fd32_read_t);
  R.DeviceId    = Of.FileId; /* Returned by the FD32_OPENFILE request */
  R.Buffer      = Buf;
  R.BufferBytes = sizeof(Buf);
  while ((Res = fat_request(FD32_READ, &R)) == sizeof(Buf))
  {
    int k;
    if (Buf[0] == 0) break; /* End-of-dir mark */
    for (k = 0; k < sizeof(Buf); k++) message("%02x", (int) Buf[k]);
    message("\n");
    for (k = 0; k < sizeof(Buf); k++)
      if (Buf[k] < ' ') message(".."); else message("%c ", Buf[k]);
    message("\n");
  }
  message("Directory dump ended with return code %08xh\n", Res);
  /* Finally close the file and unmount the FAT volume */
  return close_and_unmount(Of.FileId, M.FsDev);
}
