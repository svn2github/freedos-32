#include <sys/stat.h>

#include <ll/i386/hw-data.h>
#include <ll/i386/error.h>
#include <ll/stdarg.h>
#include <filesys.h>
#define FD32_O_ACCMODE O_ACCMODE
#define FD32_O_RDONLY  O_RDONLY
#define FD32_O_WRONLY  O_WRONLY
#define FD32_O_RDWR    O_RDWR
#define FD32_O_SYNC    O_SYNC
#define FD32_O_CREAT   O_CREAT
#define FD32_O_EXCL    O_EXCL
#define FD32_O_TRUNC   O_TRUNC
#define FD32_O_APPEND  O_APPEND
#undef O_ACCMODE
#undef O_RDONLY
#undef O_WRONLY
#undef O_RDWR
#undef O_SYNC
#undef O_CREAT
#undef O_EXCL
#undef O_TRUNC
#undef O_APPEND
#include <kernel.h>

#include <sys/fcntl.h>


ssize_t	read(int fd, void *ptr, size_t len)
{
  int res;

  res = fd32_read(fd, ptr, len);

  return res;
}

off_t lseek(int fd, off_t ptr, int dir)
{
  int res;
  long long int newpos;
  
  res = fd32_lseek(fd, ptr, dir, &newpos);
  if (res < 0) {
    return res;
  }

  return newpos;
}

ssize_t	write(int fd, const void *ptr, size_t len)
{
  int res;
  
  res = fd32_write(fd, (void *) ptr, len);

  return res;
}

int close(int fd)
{
  return fd32_close(fd);
}

int open(const char *name, int flags, ...)
{
  int mode, res, action;
  DWORD dosflags;
  WORD dosattr;
  va_list ap;
  
  /* Get the mode */
  va_start (ap, flags);
  mode = va_arg (ap, int);
  va_end (ap);

  dosflags = 0;
  dosattr = 0;
  switch (flags & O_ACCMODE)
  {
    case O_RDONLY: dosflags |= FD32_O_RDONLY; break;
    case O_WRONLY: dosflags |= FD32_O_WRONLY; break;
    case O_RDWR  : dosflags |= FD32_O_RDWR;   break;
  }
  if (flags & O_CREAT)  dosflags |= FD32_O_CREAT;
  if (flags & O_TRUNC)  dosflags |= FD32_O_TRUNC;
  if (flags & O_EXCL)   dosflags |= FD32_O_EXCL;
  if (flags & O_APPEND) dosflags |= FD32_O_APPEND;
  if (flags & O_SYNC)   dosflags |= FD32_O_SYNC;
  if ((mode & S_IRUSR) && !(mode & S_IWUSR)) dosattr |= FD32_ARDONLY;
  message("0x%08xh --> 0x%08lxh\n", flags, dosflags);
  res = fd32_open((char *) name, dosflags, dosattr/*mode*/, 0, &action);

  return res;
}

int fstat(int fd, struct stat *st)
{
  st->st_mode = S_IFCHR;

  return 0;
}
