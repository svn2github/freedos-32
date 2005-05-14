/****************************************************************************/
/* FD32 FAT Driver test program "readdir.c" by Salvo Isaja                  */
/* This program reads the content of the entries of a directory.            */
/* Disclaimer: The FAT Driver might have bugs that could corrupt your data. */
/*             Test it AT YOUR OWN RISK.                                    */
/****************************************************************************/

#include <dr-env.h>
#include <devices.h>       /* For driver interfaces                     */
#include <filesys.h>       /* For opening modes and finddata structure  */
#include <errno.h>

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
int fat_readdirtest_init(void)
{
  fd32_mount_t      M;
  fd32_openfile_t   Of;
  fd32_readdir_t    Rd;
  fd32_fs_lfnfind_t F;
  int               hDev, Res;

  message("FD32 FAT Driver test program \"readdir.o\".\n");
  message("Directory listing of '%s'\n", DIRNAME);
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
  Rd.Size  = sizeof(fd32_readdir_t);
  Rd.DirId = Of.FileId; /* Returned by the FD32_OPENFILE request */
  Rd.Entry = (void *) &F;
  while ((Res = fat_request(FD32_READDIR, &Rd)) == 0)
  {
    int k;
    /* Display short name... */
    message("%s", F.ShortName);
    for (k = 0; k < 15 - strlen(F.ShortName); k++) message(" ");
    /* ...file size or "<DIR>" mark... */
    if (F.Attr & FD32_ADIR) message("  <DIR>  ");
                       else message("%9lu", F.SizeLo);
    /*...last modification date and time and long name */
    message(" %04i-%02i-%02i %02i:%02i %s\n",
            (int) ((F.MTime >> 25) & 0x7F) + 1980, /* year    */
            (int)  (F.MTime >> 21) & 0x0F,         /* month   */
            (int)  (F.MTime >> 16) & 0x1F,         /* day     */
            (int)  (F.MTime >> 11) & 0x1F,         /* hour    */
            (int)  (F.MTime >>  5) & 0x3F,         /* minutes */
            F.LongName);
  }
  if (Res != -ENOENT)
  {
    message("Error %08xh while reading from directory\n", Res);
    close_and_unmount(Of.FileId, M.FsDev);
    return Res;
  }
  /* Finally close the file and unmount the FAT volume */
  return close_and_unmount(Of.FileId, M.FsDev);
}
