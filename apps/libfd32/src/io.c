#include <unistd.h>
#include <sys/stat.h>

#include <ll/i386/hw-data.h>
/*
#include <ll/i386/string.h>
*/
#include <kernel.h>
#include <filesys.h>

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
  
  res = fd32_write(fd, ptr, len);

  return res;
}

int close(int fd)
{
  return fd32_close(fd);
}

int open(const char *name, int flags, int mode)
{
  int res, action;

  res = fd32_open(name, flags, /*attr*/mode, 0, &action);

  return res;
}

int fstat(int fd, struct stat *st)
{
  st->st_mode = S_IFCHR;

  return 0;
}
