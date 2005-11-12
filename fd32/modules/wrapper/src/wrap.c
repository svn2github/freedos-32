/* NOTE: wrapper of the exec environment of coff-go32 programs,
         rely on DPMI module
 */

#include <ll/i386/hw-data.h>
#include <ll/i386/hw-func.h>
#include <ll/i386/error.h>
#include <ll/i386/string.h>
#include <ll/stdlib.h>
#include <ll/getopt.h>

#include "devices.h"
#include "filesys.h"
#include "format.h"
#include "kmem.h"
#include "logger.h"
#include "exec.h"

#include "wrap.h"

WORD user_cs, user_ds;
static DWORD funky_base;

int funkymem_get_region(DWORD base, DWORD size)
{
  message("Funky Mem get region: 0x%lx, 0x%lx...\n",
		  base, size);
  funky_base = base;

  return -1;
}


DWORD funkymem_get(DWORD size)
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
  tmp = mem_get(size + funky_base);
  /* NOTE: DjLibc doesn't need the memory address to be above the image base anymore? */
  if (tmp == 0) {
    message("Failed!\n");
    return 0;
  } else {
    message("Got 0x%lx\n", tmp);
  }

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

static void my_process_dos_module(struct kern_funcs *kf, int file,
		struct read_funcs *rf, char *filename, char *args)
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
        my_exec_process(kf, file, rf, filename, args);
        return;
      }
    }
  }

  message("Wrapper: Unsupported executable!\n");
}


/* Using in the DPMI module's DOS execute
int wrap_exec(char *filename, DWORD env_segment, DWORD cmd_tail,
             DWORD fcb1, DWORD fcb2, WORD *return_val)
*/
int wrap_exec(char *filename, char *args)
{
  struct kern_funcs kf;
  struct read_funcs rf;
  struct kernel_file f;
  WORD magic;

  if (fd32_kernel_open(filename, O_RDONLY, 0, 0, &f) < 0)
    return -1;

#ifdef __DEBUG__
  fd32_log_printf("FileId = 0x%lx (0x%lx)\n", (DWORD)f.file_id, (DWORD)&f);
#endif
  kf.file_read = fd32_kernel_read;
  kf.file_seek = fd32_kernel_seek;
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
    my_process_dos_module(&kf, (int)(&f), &rf, filename, args);
  }

  fd32_kernel_close((int)&f);
  return 1;
}

static struct option wrapper_options[] =
{
  /* These options set a flag. */

  /* These options don't set a flag.
     We distinguish them by their indices. */
  {"tsr-mode", no_argument, 0, 'D'},
  {0, 0, 0, 0}
};

int wrap_init(process_info_t *pi)
{
  char **argv;
  int argc = fd32_get_argv(pi->filename, pi->args, &argv);

  if (argc > 1) {
    DWORD res;
    char *sub_name, *sub_args;
    int c, option_index = 0;
    /* Parse the command line, stop when meeting the first non-option (+) */
    for ( ; (c = getopt_long (argc, argv, "+D", wrapper_options, &option_index)) != -1; ) {
      switch (c) {
        case 'D':
          message("TSR mode is currently unimplemented\n");
          break;
        default:
          break;
      }
    }
    sub_name = argv[optind], res = strlen(sub_name);
    if (++optind < argc)
      sub_args = argv[optind];
    else
      sub_args = "";
    /* Recover the original args and free the argv */
    fd32_unget_argv(argc, argv);
    /* Split the sub_name and sub_args */
    sub_name[res] = 0;
    message("FD/32 Wrapper... Trying to run %s (args: %s)\n",
		sub_name, sub_args);
#ifdef __DEBUG__
    fd32_log_printf("Program name: %s\t CmdLine: %s\n", sub_name, sub_args);
#endif
    wrap_exec(sub_name, sub_args);
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
