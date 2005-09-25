#include <unistd.h>

#include <ll/i386/hw-data.h>
#include <ll/i386/error.h>
#include <kmem.h>
#include <kernel.h>
#include <filesys.h>
#include <logger.h>

int execve(const char *filename, char * const argv[], char * const env[])
{
  struct kern_funcs kf;
  struct read_funcs rf;
  struct bin_format *binfmt;
  struct kernel_file f;
  DWORD i, retcode = -1;

  if (fd32_kernel_open(filename, O_RDONLY, 0, 0, &f) < 0)
    return -1;

#ifdef __DOS_EXEC_DEBUG__
  fd32_log_printf("FileId = 0x%lx (0x%lx)\n", (DWORD)f.file_id, (DWORD)&f);
#endif
  kf.file_read = fd32_kernel_read;
  kf.file_seek = fd32_kernel_seek;
  kf.mem_alloc = mem_get;
  kf.mem_alloc_region = mem_get_region;
  kf.mem_free = mem_free;
  kf.message = message;
  kf.log = fd32_log_printf;
  kf.error = message;
  kf.get_dll_table = get_dll_table;
  kf.add_dll_table = add_dll_table;
  kf.seek_set = FD32_SEEKSET;
  kf.seek_cur = FD32_SEEKCUR;
  kf.file_offset = 0;

  /* Get the binary format object table, ending with NULL name */
  binfmt = fd32_get_binfmt();
  
  /* Load different modules in various binary format */
  for (i = 0; binfmt[i].name != NULL; i++)
  {
    if (binfmt[i].check(&kf, (int)(&f), &rf)) {
      /* NOTE: args[0] is the length of the args */
      retcode = binfmt[i].exec(&kf, (int)(&f), &rf, filename, argv[0]);
      break;
    }
#ifdef __DOS_EXEC_DEBUG__
    else {
      fd32_log_printf("[MOD] Not '%s' format\n", binfmt[i].name);
    }
#endif
    /* p->file_seek(file, p->file_offset, p->seek_set); */
  }

  fd32_kernel_close((int)&f);

  return retcode;
}
