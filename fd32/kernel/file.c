/* FD/32 Kernel File handling
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <ll/i386/hw-data.h>

#include "kernel.h"
#include "filesys.h"

int fd32_kernel_open(const char *filename, DWORD mode, WORD attr, WORD alias_hint, struct kernel_file *f)
{
  int res;
  char *pathname;
  void *fs_device;
  fd32_openfile_t of;
/* TODO: filename must be canonicalized with fd32_truename, but fd32_truename
         resolve the current directory, that is per process.
         Have we a valid current_psp at this point?
         Next, truefilename shall be used instead of filename.
  char truefilename[FD32_LFNPMAX];
  if (fd32_truename(truefilename, filename, FD32_TNSUBST) < 0) {
#ifdef __DEBUG__
    fd32_log_printf("Cannot canonicalize the file name!!!\n");
#endif
    return -1;
  } */
  if ((res = fd32_get_drive(/*true*/filename, &f->request, &fs_device, &pathname)) < 0) {
#ifdef __DEBUG__
    fd32_log_printf("Cannot find the drive!!!\n");
#endif
  } else {
  #ifdef __DEBUG__
    fd32_log_printf("Opening %s\n", /*true*/file_name);
  #endif
    of.Size = sizeof(fd32_openfile_t);
    of.DeviceId = fs_device;
    of.FileName = pathname;
    of.Mode = mode;
    of.Attr = attr;
    of.AliasHint = alias_hint;
    if ((res = f->request(FD32_OPENFILE, &of)) < 0) {
  #ifdef __DEBUG__
      fd32_log_printf("File not found!!\n");
  #endif
    } else {
      f->file_id = of.FileId;
    }
  }

  return res;
}

int fd32_kernel_read(int id, void *buffer, int len)
{
  struct kernel_file *f;
  fd32_read_t r;
  int res;

  f = (struct kernel_file *)id;
  r.Size = sizeof(fd32_read_t);
  r.DeviceId = f->file_id;
  r.Buffer = buffer;
  r.BufferBytes = len;
  res = f->request(FD32_READ, &r);
#ifdef __DEBUG__
  if (res < 0) {
    fd32_log_printf("WTF!!!\n");
  }
#endif

  return res;
}

int fd32_kernel_seek(int id, int pos, int whence)
{
  int error;
  struct kernel_file *f;
  fd32_lseek_t ls;

  f = (struct kernel_file *)id;
  ls.Size = sizeof(fd32_lseek_t);
  ls.DeviceId = f->file_id;
  ls.Offset = (long long int) pos;
  ls.Origin = (DWORD) whence;
  error = f->request(FD32_LSEEK, &ls);
  return ls.Offset;
}

int fd32_kernel_close(int id)
{
  fd32_close_t cr;
  struct kernel_file *f;
  
  f = (struct kernel_file *)id;
  cr.Size = sizeof(fd32_close_t);
  cr.DeviceId = f->file_id;

  return f->request(FD32_CLOSE, &cr);
}
