#include <ll/i386/hw-data.h>
#include <ll/i386/error.h>
#include <ll/stdlib.h>
#include "devices.h"
#include "filesys.h"
#include "format.h"
#include "kernel.h"
#include "kmem.h"
#include "logger.h"
#include "exec.h"

/* from fd32/boot/modules.c */
int identify_module(struct kern_funcs *p, int file, struct read_funcs *parser);
/* from fd32/kernel/exec.c */

/* #define __DEBUG__ */

struct funky_file {
  fd32_request_t *request;
  void *file_id;
};

static int my_read(int id, void *b, int len)
{
  struct funky_file *f;
  fd32_read_t r;
  int res;

  f = (struct funky_file *)id;
  r.Size = sizeof(fd32_read_t);
  r.DeviceId = f->file_id;
  r.Buffer = b;
  r.BufferBytes = len;
  res = f->request(FD32_READ, &r);
#ifdef __DEBUG__
  if (res < 0) {
    fd32_log_printf("WTF!!!\n");
  }
#endif

  return res;
}

static int my_seek(int id, int pos, int w)
{
  int error;
  struct funky_file *f;
  fd32_lseek_t ls;
 
  f = (struct funky_file *)id;
  ls.Size = sizeof(fd32_lseek_t);
  ls.DeviceId = f->file_id;
  ls.Offset = (long long int) pos;
  ls.Origin = (DWORD) w;
  error = f->request(FD32_LSEEK, &ls);
  return (int) ls.Offset;
}

int dos_exec(char *filename, DWORD env_segment, char *args,
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
  fd32_close_t cr;
  DWORD base;

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
  fd32_log_printf("FileId = 0x%lx (0x%lx)\n", (DWORD)f.file_id, (DWORD)&f);
#endif
  p.file_read = my_read;
  p.file_seek = my_seek;
  p.mem_alloc = mem_get;
  p.mem_alloc_region = mem_get_region;
  p.mem_free = mem_free;
  p.message = message;
  p.log = fd32_log_printf;
  p.error = message;
  p.get_dll_table = get_dll_table;
  p.add_dll_table = add_dll_table;
  p.seek_set = FD32_SEEKSET;
  p.seek_cur = FD32_SEEKCUR;
  p.file_offset = 0; /* Reflect the changes in identify_module */

  mod_type = identify_module(&p, (int)(&f), &parser);
#ifdef __DEBUG__
  fd32_log_printf("Module type: %d\n", mod_type);
#endif
  if ((mod_type == 2) || (mod_type == 3)) {
    entry_point = load_process(&p, (int)(&f), &parser, &exec_base, &base, &size);
    cr.Size = sizeof(fd32_close_t);
    cr.DeviceId = f.file_id;
    f.request(FD32_CLOSE, &cr);
    if (entry_point == -1) {
#ifdef __DEBUG__
      fd32_log_printf("Error loading %s\n", filename);
#endif
      return -1;
    }
    
    if (exec_base == 0) {
      struct process_info pi;
#ifdef __DEBUG__
      fd32_log_printf("Seems to be a native application?\n");
#endif
      if (args == NULL) {
        pi.args = NULL;
      } else {
        pi.args = &args[1];
      }
      pi.memlimit = base + size + LOCAL_BSS;
      pi.name = filename;
#ifdef __DEBUG__
      message("Mem Limit: 0x%lx = 0x%lx 0x%lx\n", pi.memlimit, base, size);
#endif
      /* FIXME: args has a space at the beginning... Fix it in the caller! */
#if 0
      run(entry_point, 0, (DWORD)/*filename*/&args[1]);
#else
      run(entry_point, 0, (DWORD)&pi);
#endif
      return 0;
    } else {
#ifdef __DEBUG__
      fd32_log_printf("Executable: entry point = 0x%lx\n", entry_point);
#endif
    }
    /* *return_val = exec_process(&p, (int)(&f), &parser, filename); */
    *return_val = create_process(entry_point, exec_base, size, filename, args);
    message("Returned: %d!!!\n", *return_val);
    mem_free(exec_base, size);
  }

  if (mod_type == 4) {
    process_dos_module(&p, (int)(&f), &parser, filename, args);
    cr.Size = sizeof(fd32_close_t);
    cr.DeviceId = f.file_id;
    f.request(FD32_CLOSE, &cr);
  }
  return 1;
}

