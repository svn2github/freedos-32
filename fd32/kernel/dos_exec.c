#include <ll/i386/hw-data.h>
#include <ll/i386/error.h>
#include <ll/stdlib.h>

#include "dev/fs.h"
#include "dev/char.h"
#include "filesys.h"
#include "format.h"
#include "kmem.h"
#include "drives.h"
#include "logger.h"

struct funky_file {
  int fd;
  fd32_dev_fsvol_t *p;
};

static int my_read(int id, void *b, int len)
{
  int error;
  DWORD result;
  struct funky_file *f;

  f = (struct funky_file *)id;

  error = f->p->read(f->p->P, f->fd, b, len, &result);
  return result;
}

static int my_seek(int id, int pos, int w)
{
  int error;
  long long int result;
  struct funky_file *f;

  f = (struct funky_file *)id;
  error = f->p->lseek(f->p->P, f->fd, pos, w, &result);
  return result;
}

int dos_exec(char *filename, DWORD env_segment, DWORD cmd_tail,
	DWORD fcb1, DWORD fcb2, BYTE *return_val)
{
  struct kern_funcs p;
  int mod_type;
  DWORD entry_point;
  struct read_funcs parser;
  DWORD exec_base;
  int size;
  struct funky_file f;

  f.p = get_drive(filename);
  if (f.p == NULL) {
#ifdef __DEBUG__
    fd32_log_printf("Cannot find the drive!!!\n");
#endif
    return -1;
  }
#ifdef __DEBUG__
  fd32_log_printf("Opening %s\n", filename);
#endif
  f.fd = f.p->open(f.p->P, get_name_without_drive(filename),
	  FD32_OPEN_EXIST_OPEN | FD32_OPEN_ACCESS_READ,
	  FD32_ATTR_NONE, 0, NULL, FD32_NOLFN);

  if (f.fd < 0) {
#ifdef __DEBUG__
    fd32_log_printf("File not found!!\n");
#endif
    return -1;
  }

#ifdef __DEBUG__
  fd32_log_printf("FD = %d\n", fd);
#endif
  p.file_read = my_read;
  p.file_seek = my_seek;
  p.mem_alloc = mem_get;
  p.mem_alloc_region = mem_get_region;
  p.mem_free = mem_free;
  p.message = message;
  p.log = fd32_log_printf;
  p.error = message;
  p.seek_set = FD32_SEEK_SET;
  p.seek_cur = FD32_SEEK_CUR;

  mod_type = identify_module(&p, (int)(&f), &parser);
#ifdef __DEBUG__
  fd32_log_printf("Module type: %d\n", mod_type);
#endif
  if (mod_type == 2) {
    entry_point = load_process(&p, (int)(&f), &parser, &exec_base,
	    &size);
    if (entry_point == -1) {
#ifdef __DEBUG__
      fd32_log_printf("Error loading %s\n", filename);
#endif
      return -1;
    }
    
    if (exec_base == 0) {
#ifdef __DEBUG__
      fd32_log_printf("What the heck!?\n");
#endif
      return 0;
    } else {
#ifdef __DEBUG__
      fd32_log_printf("Executable: entry point = 0x%lx\n", entry_point);
#endif
    }
    *return_val = exec_process(&p, (int)(&f), &parser, filename);
  }
  return 1;
}

