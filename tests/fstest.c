#include <dr-env.h>
#include <filesys.h>       /* For file system services */

/* Set it to the name of the file you want to open. */
/* Not canonicalized paths are allowed.             */
#define FILENAME "c:\\tmp\\test.txt"


/* Closes a file. This is called two times, let's make it a function. */
static int close(int hFile)
{
  int Res;
  if ((Res = fd32_close(hFile)) < 0)
    message("Error %08xh while closing the file\n", Res);
  return Res;
}


/* The FAT test entry point, called by the FD32 kernel. */
int fat_test_init(void)
{
  BYTE Buf[1024];
  int  hFile, Res, k;

  /* Open file. The FS volume is mounted on the first access */
  hFile = fd32_open(FILENAME, O_RDONLY, FD32_ANONE, 0, NULL);
  if (hFile < 0)
  {
    message("Error %08xh while opening the file\n", hFile);
    return hFile;
  }
  message("File opened with handle %i\n", hFile);
  /* Read some data from file */
  Res = fd32_read(hFile, Buf, sizeof(Buf));
  if (Res < 0)
  {
    message("Error %08xh while reading from file\n", Res);
    close(hFile);
    return Res;
  }
  /* Display that data */
  message("--BEGIN OF DATA READ--\n");
  for (k = 0; k < Res; k++) message("%c", Buf[k]);
  message("---END OF DATA READ---\n");
  /* And finally close the file */
  return close(hFile);
}
