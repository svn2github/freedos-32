#include <ll/i386/hw-data.h>
#include <ll/i386/error.h>
#include <ll/stdlib.h>
#include "devices.h"
#include "filesys.h"
#include "format.h"
#include "kmem.h"
#include "logger.h"
#include "exec.h"

/* from fd32/boot/modules.c */
int identify_module(struct kern_funcs *p, int file, struct read_funcs *parser);
/* from fd32/kernel/exec.c */
DWORD load_process(struct kern_funcs *p, int file, struct read_funcs *parser, DWORD *e_s, int *s);

#define __DEBUG__

struct funky_file {
  fd32_request_t *request;
  void *file_id;
};

static int my_read(int id, void *b, int len)
{
  struct funky_file *f;
  fd32_read_t r;

  f = (struct funky_file *)id;
  r.Size = sizeof(fd32_request_t);
  r.DeviceId = f->file_id;
  r.Buffer = b;
  r.BufferBytes = len;
  return f->request(FD32_READ, &r);
}

static int my_seek(int id, int pos, int w)
{
  int error;
  struct funky_file *f;
  fd32_lseek_t ls;
  
  f = (struct funky_file *)id;
  ls.Size = sizeof(fd32_lseek_t);
  ls.DeviceId = f->file_id;
  ls.Offset = (LONGLONG) pos;
  ls.Origin = (DWORD) w;
  error = f->request(FD32_LSEEK, &ls);
  return (int) ls.Offset;
}

int dos_exec(char *filename, DWORD env_segment, DWORD cmd_tail,
             DWORD fcb1, DWORD fcb2, WORD *return_val)
{
  struct kern_funcs p;
  int mod_type;
  DWORD entry_point;
  struct read_funcs parser;
  DWORD exec_base;
  int size;
  struct funky_file f;
  void *fs_device;
  char *pathname;
  fd32_openfile_t of;

  /*
  TODO: filename must be canonicalized with fd32_truename, but fd32_truename
        resolve the current directory, that is per process.
        Have we a valid current_psp at this point?
        Next, truefilename shall be used instead of filename.
  char truefilename[FD32_LFNPMAX];
  if (fd32_truename(truefilename, filename, FD32_TNSUBST) < 0) {
#ifdef __DEBUG__
    fd32_log_printf("Cannot canonicalize the file name!!!\n");
#endif
    return -1;
  }
  */
  if (fd32_get_drive(/*true*/filename, &f.request, &fs_device, &pathname) < 0) {
#ifdef __DEBUG__
    fd32_log_printf("Cannot find the drive!!!\n");
#endif
    return -1;
  }
#ifdef __DEBUG__
  fd32_log_printf("Opening %s\n", /*true*/filename);
#endif
  of.Size = sizeof(fd32_openfile_t);
  of.DeviceId = fs_device;
  of.FileName = pathname;
  of.Mode = FD32_OREAD | FD32_OEXIST;
  if (f.request(FD32_OPENFILE, &of) < 0) {
#ifdef __DEBUG__
    fd32_log_printf("File not found!!\n");
#endif
    return -1;
  }
  f.file_id = of.FileId;

#ifdef __DEBUG__
  fd32_log_printf("FileId = %08lx\n", (DWORD) f.file_id);
#endif
  p.file_read = my_read;
  p.file_seek = my_seek;
  p.mem_alloc = mem_get;
  p.mem_alloc_region = mem_get_region;
  p.mem_free = mem_free;
  p.message = message;
  p.log = fd32_log_printf;
  p.error = message;
  p.seek_set = FD32_SEEKSET;
  p.seek_cur = FD32_SEEKCUR;

  mod_type = identify_module(&p, (int)(&f), &parser);
#ifdef __DEBUG__
  fd32_log_printf("Module type: %d\n", mod_type);
#endif
  if (mod_type == 2) {
    entry_point = load_process(&p, (int)(&f), &parser, &exec_base, &size);
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

