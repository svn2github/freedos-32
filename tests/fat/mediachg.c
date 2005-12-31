/****************************************************************************/
/* FD32 FAT Driver test program "read.c" by Salvo Isaja                     */
/* This program performs a read from a file.                                */
/* Disclaimer: The FAT Driver might have bugs that could corrupt your data. */
/*             Test it AT YOUR OWN RISK.                                    */
/****************************************************************************/

#include <dr-env.h>
#include <devices.h>       /* For driver interfaces    */
#include <filesys.h>       /* For opening modes        */

/* Set the following to the file system device you like          */
#define DEVNAME "fd0"
/* Set it to the canonicalized name of the file you want to open */
#define FILENAME "\\"


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
  message("Closing... ");
  C.Size     = sizeof(fd32_close_t);
  C.DeviceId = FileId;
  if ((Res = fat_request(FD32_CLOSE, &C)) < 0)
    message("Error %08xh while closing the file\n", Res);
  message("done %i\n", Res);
  /* And unmouunt the FAT volume */
  message("Unmounting... ");
  Um.Size     = sizeof(fd32_unmount_t);
  Um.DeviceId = FsDev;
  if ((Res = fat_request(FD32_UNMOUNT, &Um)) < 0)
    message("Error %08xh while unmounting the FAT volume\n", Res);
  message("done %i\n", Res);
  return Res;
}


/* The FAT test entry point, called by the FD32 kernel. */
int fat_mediachangetest_init(void)
{
  fd32_mount_t    M;
  fd32_openfile_t Of;
  fd32_read_t     R;
  BYTE            Buf[1024];
  int             Res, k, j;

#if 0
  /* Mount a FAT volume on the device */
  message("Mouting... ");
  M.Size = sizeof(fd32_mount_t);
  M.BlkDev = DEVNAME;
  if ((Res = fat_request(FD32_MOUNT, &M)) < 0)
  {
    message("Couldn't get data for device: %08xh\n", Res);
    return Res;
  }
  message("done %i\n", Res);
  /* Open file */
  message("Opening %s... ", FILENAME);
  Of.Size      = sizeof(fd32_openfile_t);
  Of.DeviceId  = M.FsDev; /* Returned by the FD32_MOUNT request */
  Of.FileName  = FILENAME;
  Of.Mode      = O_RDONLY;
  if ((Res = fat_request(FD32_OPENFILE, &Of)) < 0)
  {
    message("Error %08xh while opening the file\n", Res);
    return Res;
  }
  message("done %i\n", Res);
  /* Read some data from file */
  for (k = 0; k < 3; k++)
  {
	fd32_lseek_t lsp =
	{
		.Size = sizeof(fd32_lseek_t),
		.DeviceId = Of.FileId,
		.Offset = 0,
		.Origin = FD32_SEEKSET
	};
	message("Round #%i\n", k);
	Res = fat_request(FD32_LSEEK, &lsp);
	if (Res < 0)
	{
		message("Error %08xh while seeking into file\n", Res);
		close_and_unmount(Of.FileId, M.FsDev);
		return Res;
	}
	message("Reading... ");
	R.Size        = sizeof(fd32_read_t);
	R.DeviceId    = Of.FileId; /* Returned by the FD32_OPENFILE request */
	R.Buffer      = Buf;
	R.BufferBytes = sizeof(Buf);
	if ((Res = fat_request(FD32_READ, &R)) < 0)
	{
		message("Error %08xh while reading from file\n", Res);
		close_and_unmount(Of.FileId, M.FsDev);
		return Res;
	}
	message("done %i\n", Res);
	message("Delay...\n");
	for (j = 0; j < 100000000; j++);
  }
  /* Finally close the file and unmount the FAT volume */
  return close_and_unmount(Of.FileId, M.FsDev);
#endif
	fd32_unmount_t Um;
	fd32_close_t C;
		/* Mount a FAT volume on the device */
		message("Mouting... ");
		M.Size = sizeof(fd32_mount_t);
		M.BlkDev = DEVNAME;
		if ((Res = fat_request(FD32_MOUNT, &M)) < 0)
		{
			message("Couldn't get data for device: %08xh\n", Res);
			return Res;
		}
		message("done %i\n", Res);
	for (k = 0; k < 3; k++)
	{
		/* Open file */
		message("Opening %s... ", FILENAME);
		Of.Size      = sizeof(fd32_openfile_t);
		Of.DeviceId  = M.FsDev; /* Returned by the FD32_MOUNT request */
		Of.FileName  = FILENAME;
		Of.Mode      = O_RDONLY;
		if ((Res = fat_request(FD32_OPENFILE, &Of)) < 0)
		{
			message("Error %08xh while opening the file\n", Res);
			return Res;
		}
		message("done %i\n", Res);
		/* Close the file */
		message("Closing... ");
		C.Size     = sizeof(fd32_close_t);
		C.DeviceId = Of.FileId;
		if ((Res = fat_request(FD32_CLOSE, &C)) < 0)
			message("Error %08xh while closing the file\n", Res);
		message("done %i\n", Res);
		message("Delay...\n");
		for (j = 0; j < 100000000; j++);
	}
		/* And unmouunt the FAT volume */
		message("Unmounting... ");
		Um.Size     = sizeof(fd32_unmount_t);
		Um.DeviceId = M.FsDev;
		if ((Res = fat_request(FD32_UNMOUNT, &Um)) < 0)
			message("Error %08xh while unmounting the FAT volume\n", Res);
		message("done %i\n", Res);
	return 0;
}
