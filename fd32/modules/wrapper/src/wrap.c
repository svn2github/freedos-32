#include <ll/i386/hw-data.h>
#include <ll/i386/hw-func.h>
#include <ll/i386/error.h>
#include <ll/i386/string.h>
#include <ll/stdlib.h>

#include "devices.h"
#include "filesys.h"
#include "format.h"
#include "kernel.h"
#include "kmem.h"
#include "logger.h"
#include "exec.h"
#include "doshdr.h"

#define RUN_RING 0

#define VERSION "0.1"
DWORD pgm_mem_base = 0x400000;
DWORD pgm_mem_step = 0x100000;
#define PGM_MEM_LIMIT 0x4000000

/* from fd32/kernel/exec.c */
int my_exec_process(struct kern_funcs *p, int file, struct read_funcs *parser, char *cmdline);

WORD user_cs, user_ds;

struct funky_file {
  fd32_request_t *request;
  void *file_id;
};
static DWORD funky_base;

int funkymem_get_region(DWORD base, DWORD size)
{
  message("Funky Mem get region: 0x%lx, 0x%lx...\n",
		  base, size);
  funky_base = base;

  return -1;
}


int funkymem_get(DWORD size)
{
  DWORD tmp;
  int done;
  WORD data_selector, code_selector;
  BYTE acc;
  
  message("Funky Mem get: 0x%lx\n", size);
  if (funky_base == 0) {
    message("Mmmrm... Funky base is not set... I assume it is a regular mem_alloc?\n");
    return mem_get(size);
  }
 
  message("Allocating 0x%lx = 0x%lx + 0x%lx\n",
		  size + funky_base, size, funky_base);
#if 0
  tmp = mem_get(size + funky_base);
#else
  /* FIXME!!! */
  tmp = pgm_mem_base;
  done = 0;
  while ((!done) && (tmp < PGM_MEM_LIMIT)) {
    if (mem_get_region(tmp, size + funky_base) < 0) {
      tmp += pgm_mem_step;
    } else {
      done = 1;
    }
  }
#endif
  if (done == 0) {
    message("Failed!\n");

    return 0;
  }
  message("Got 0x%lx\n", tmp);

  done = 0; data_selector = 8;
  while (data_selector < 256 && (!done)) {
    gdt_read(data_selector, NULL, &acc, NULL);
    if (acc == 0) {
      done = 1;
    } else {
      data_selector += 8;
    }
  }
  if (done == 0) {
    message("Cannot allocate data selector!\n");

    return 0;
  }
  gdt_place(data_selector, tmp, size + funky_base, 0x92 | (RUN_RING << 5), 0xC0);
  code_selector = data_selector; done = 0;
  while (code_selector < 256 && (!done)) {
    gdt_read(code_selector, NULL, &acc, NULL);
    if (acc == 0) {
      done = 1;
    } else {
      code_selector += 8;
    }
  }
  if (done == 0) {
    message("Cannot allocate code selector!\n");
    gdt_place(data_selector, 0, 0, 0, 0);

    return 0;
  }
  gdt_place(code_selector, tmp, size + funky_base, 0x9A | (RUN_RING << 5), 0xC0);
  tmp += funky_base;
  funky_base = 0;
 
  user_cs = code_selector;
  user_ds = data_selector;
  fd32_log_printf("User CS, DS = 0x%x, 0x%x\n", code_selector, data_selector);

  return tmp;
}

static int funkymem_free(DWORD base, DWORD size)
{
  message("FIXME: Implement Funky Mem free!!!\n");

  return mem_free(base, size);
}

void my_close(int id)
{
  fd32_close_t cr;
  struct funky_file *f;
  int res;
  
  f = (struct funky_file *)id;
  cr.Size = sizeof(fd32_close_t);
  cr.DeviceId = f->file_id;
  res = f->request(FD32_CLOSE, &cr);
}

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

static void my_process_dos_module(struct kern_funcs *kf, int file,
		struct read_funcs *rf, char *cmdline)
{
  struct dos_header hdr;
  struct bin_format *binfmt = fd32_get_binfmt();
  DWORD dj_header_start;
  DWORD i;

  kf->file_seek(file, 0, kf->seek_set);
  kf->file_read(file, &hdr, sizeof(struct dos_header));

  dj_header_start = hdr.e_cp * 512L;
  if (hdr.e_cblp) {
    dj_header_start -= (512L - hdr.e_cblp);
  }
  kf->file_offset = dj_header_start;
  kf->file_seek(file, dj_header_start, kf->seek_set);

  for (i = 0; binfmt[i].name != NULL; i++)
  {
    if (strcmp(binfmt[i].name, "coff") == 0) {
      if (binfmt[i].check(kf, file, rf)) {
        my_exec_process(kf, file, rf, cmdline);
        return;
      }
    }
  }

  message("Wrapper: Unsupported executable!\n");
}


#if 0
int wrap_exec(char *filename, DWORD env_segment, DWORD cmd_tail,
             DWORD fcb1, DWORD fcb2, WORD *return_val)
#else
int wrap_exec(char *filename, char *args)
#endif
{
  struct kern_funcs kf;
  struct read_funcs rf;
  struct funky_file f;
  void *fs_device;
  char *pathname;
  fd32_openfile_t of;
  fd32_close_t cr;
  int res;
  WORD magic;

  if (fd32_get_drive(filename, &f.request, &fs_device, &pathname) < 0) {
#ifdef __DEBUG__
    fd32_log_printf("Cannot find the drive!!!\n");
#endif
    return -1;
  }
#ifdef __DEBUG__
  fd32_log_printf("Opening %s\n", filename);
#endif
  of.Size = sizeof(fd32_openfile_t);
  of.DeviceId = fs_device;
  of.FileName = pathname;
  of.Mode = O_RDONLY;
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
  kf.file_read = my_read;
  kf.file_seek = my_seek;
  kf.file_offset = 0; /* Reflect the changes in identify_module */
  kf.mem_alloc = funkymem_get;
  kf.mem_alloc_region = funkymem_get_region;
  kf.mem_free = funkymem_free;
  kf.message = message;
  kf.log = fd32_log_printf;
  kf.error = message;
  kf.seek_set = FD32_SEEKSET;
  kf.seek_cur = FD32_SEEKCUR;

  kf.file_read((int)(&f), &magic, 2);
  if (magic == 0x5A4D) { /* "MZ" */
    my_process_dos_module(&kf, (int)(&f), &rf, args);
  }
  
  cr.Size = sizeof(fd32_close_t);
  cr.DeviceId = f.file_id;
  message("Closing %d\n", (int)f.file_id);
  res = f.request(FD32_CLOSE, &cr);
  message("Returned %d\n", res);
  return 1;
}

static char *arg_parse(char *c)
{
  int done, res;
  char *tmp;
  char tmpbuff[20];

  done = 0;
  while (!done) {
    if (*c == 0) {
      done = 1;
    } else {
      if (*c =='-') {
	c++;
	if (*c == 0) {
          message("Malformed cmd line!\n");

	  return NULL;
	}
        switch (*c) {
          case 'D':
	    message("TSR mode is currently unimplemented\n");
	    break;
          case 'b':
#if 0
	    message("Setting memory base is currently unimplemented\n");
#else
	    tmp = tmpbuff;
	    do {
              c++;
	    } while ((*c != 0) && (*c == ' '));
	    while ((*c != 0) && (*c != ' ')) {
              *tmp++ = *c++;
	    }
	    *tmp = 0;
            res = atoi(tmpbuff);
	    if ((res > 0x100000) && (res < 0x10000000)) {
              pgm_mem_base = res;
	    } else {
              message("Strange Memory Base 0x%x: not setting\n", res);
	    }
#endif
	    break;
          case 's':
#if 0
	    message("Setting memory step is currently unimplemented\n");
#else
	    tmp = tmpbuff;
	    do {
              c++;
	    } while ((*c != 0) && (*c == ' '));
	    while ((*c != 0) && (*c != ' ')) {
              *tmp++ = *c++;
	    }
	    *tmp = 0;
	    res = atoi(tmpbuff);
	    if ((res > 0x100) && (res < 0x1000000)) {
              pgm_mem_step = res;
	    } else {
              message("Strange Memory Step 0x%x: not setting\n", res);
	    }
#endif
	    break;
          default:
	    message("Unknown option -%c\n", *c);
	    break;
	}
	do {
          c++;
	} while ((*c != 0) && (*c == ' '));
      } else {
	done = 1;
      }
    }
  }

  return c;
}
int wrap_init(struct process_info *pi)
{
  int i, done;
  char prgname[15], *p;
  char *file, *arglist;

  arglist = args_get(pi);

  if (arglist != NULL) {
    file = arg_parse(arglist);
    done = 0;
    p = file;
    i = 0;
    while ((!done) && (p[i] != 0)) {
      if (p[i] == ' ') {
        done = 1;
      } else {
        prgname[i] = p[i];
        i++;
      }
    }
    prgname[i] = 0;
    message("FD/32 Wrapper... Trying to run %s (args: %s)\n",
		    prgname, file);
#ifdef __DEBUG__
    fd32_log_printf("Program name: %s\t CmdLine: %s\n", prgname, file);
#endif
    wrap_exec(prgname, file);
    /* NOTE: wrap exec does not return here, but exit() returns back to
     * the function that called the wrapper...
     * (hint: the current_SP that is really used is in the kernel, not in
     * the wrapper...
     */
    return 0;
  } else {
    /* TODO: Add some blah */
    message("Wrapper %s\n", VERSION);
  }

  return -1;
}
