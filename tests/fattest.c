#include <ll/i386/error.h>
#include "devices.h"
#include "dev/fs.h"
#include "filesys.h"

int fat_test_init(void)
{
  fd32_dev_fsvol_t *Ops;
  int               Fd, k;
  BYTE              Buf[1024];
  DWORD             NumRead;

  Ops = fd32_dev_query("hda1", FD32_DEV_FSVOL);
  if (Ops == 0) {
    message("Cannot find hda1\n");
    return -1;
  }
  Fd = Ops->open(Ops->P, "\\tmp\\test.txt",
		 FD32_OPEN_ACCESS_READ | FD32_OPEN_EXIST_OPEN,
                 FD32_ATTR_NONE, 0, NULL, 1);
  if (Fd < 0) {
    message("Error Opening the file...%d\n", Fd);
    return -1;
  }
  Ops->read(Ops->P, Fd, Buf, sizeof(Buf), &NumRead);
  for (k = 0; k < NumRead; k++) {
    message("%c", Buf[k]);
  }

  Ops->close(Ops->P, Fd);

  return 0;
}
