#include <sys/stat.h>
#include <sys/fcntl.h>

#include "sys/syscalls.h"


ssize_t	_read(int fd, void *ptr, size_t len)
{
  int res;

  res = fd32_read(fd, ptr, len);

  return res;
}

off_t _lseek(int fd, off_t ptr, int dir)
{
  int res;
  long long int newpos;
  
  res = fd32_lseek(fd, ptr, dir, &newpos);
  if (res < 0) {
    return res;
  }

  return newpos;
}

ssize_t	_write(int fd, const void *ptr, size_t len)
{
  int res;
  
  res = fd32_write(fd, ptr, len);

  return res;
}

int _close(int fd)
{
  return fd32_close(fd);
}

//int _open(const char *name, int flags, ...)
int _open(const char *name, int flags, int mode)
{
  int /*mode,*/ res/*, action*/;
//  uint32_t dosflags;
  uint16_t dosattr;
//  va_list ap;
  
#if 0
  /* Get the mode */
  va_start (ap, flags);
  mode = va_arg (ap, int);
  va_end (ap);
#endif

//  dosflags = 0;
  dosattr = 0;
#if 0
  if (flags & O_WRONLY) {
    dosflags |= FD32_OWRITE;
  }
  if (flags & O_RDWR) {
    dosflags |= FD32_ORDWR;
  }
  if (flags & O_CREAT) {
    dosflags |= FD32_OCREAT;
  }
  if (flags & O_TRUNC) {
    dosflags |= FD32_OTRUNC;
  }
#endif
  if ((mode & S_IRUSR) && !(mode & S_IWUSR)) {
    dosattr |= FD32_ARDONLY;
  }
#if 0
	/* What about these ones? */
#define	O_APPEND	_FAPPEND
#define	O_EXCL		_FEXCL
#define FD32_OEXIST   (1 << 16) /* Open existing file             */
#define FD32_ODIR     (1 << 24) /* Open a directory as a file     */
#define FD32_OFILEID  (1 << 25) /* Interpret *FileName as a fileid */
#define FD32_OACCESS  0x0007    /* Bit mask for access type       */
#endif

//  message("0x%x --> 0x%lx\n", flags, dosflags);
  res = fd32_open(name, /*uint32_t(*/flags/*)*/, dosattr/*mode*/, 0, NULL /*&action*/);

  return res;
}

int _fstat(int fd, struct stat *st)
{
  st->st_mode = S_IFCHR;

  return 0;
}
