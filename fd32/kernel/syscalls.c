/* FD32 Kernel exported symbols table & management functions
 * by Luca Abeni & Hanzac Chen
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/hw-data.h>
#include <ll/i386/hw-func.h>
#include <ll/i386/hw-arch.h>
#include <ll/i386/pic.h>
#include <ll/i386/x-bios.h>
/* #include <ll/i386/x-dosmem.h> */
#include <ll/i386/cons.h>
#include <ll/i386/error.h>
#include <ll/stdlib.h>
#include <ll/stdio.h>
#include <ll/string.h>
#include <ll/ctype.h>
#include <ll/getopt.h>

#include <ll/sys/ll/event.h>
#include <ll/sys/ll/time.h>

#include "exec.h"
#include "kernel.h"
#include "kmem.h"
#include "format.h"
#include "handler.h"
#include "logger.h"
#include "devices.h"
#include "filesys.h"
#include "fd32time.h"
#include "slabmem.h"

/* TODO: Remove the following from the kernel */
extern const char *block_enumerate (void **iterator);
extern int         block_get       (const char *name, int type, void **operations, void **handle);
extern int         block_register  (const char *name, int (*request)(int function, ...), void *handle);
extern int         block_unregister(const char *name);

/* "not used" extern DWORD ll_exc_table[16]; */
extern struct handler exc_table[32];
extern struct gate IDT[256];
extern DWORD rm_irq_table[256];
extern WORD kern_CS, kern_DS;
extern CONTEXT context_save(void);
extern union gdt_entry *GDT_base;

#define EMPTY_SLOT {0, (DWORD)0xFFFFFFFF}

int add_dll_table(char *dll_name, DWORD handle, DWORD symbol_num,
	struct symbol *symbol_array);

static void fake_get_date(fd32_date_t *D)
{
  D->Year = 1980;
  D->Mon  = 0;
  D->Day  = 0;
}

static void fake_get_time(fd32_time_t *T)
{
  memset(T, 0, sizeof(fd32_time_t));
}

/* Kernel File handling */
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

/* Delay a specified nanoseconds
 * NOTE: This needs to be rewritten. Move into the proper place of kernel?
 */
static void imp_delay(unsigned int ns)
{
    TIME start, current;
    TIME us = ns / 1000;
    us +=4;
    current = start = ll_gettime(TIME_NEW, NULL);
    while(current - start < us)
        current = ll_gettime(TIME_NEW, NULL);
}

static void *imp_memcpy(void *dest, const void *src, size_t size)
{
  size_t i, j, m, n = size>>3;
  BYTE *src_p8 = (BYTE *)src;
  BYTE *dest_p8 = (BYTE *)dest;
#ifdef __MMX__
  #include <mmintrin.h>
  __m64 *src_p64 = (__m64 *)src;
  __m64 *dest_p64 = (__m64 *)dest;
#else
  QWORD *src_p64 = (QWORD *)src;
  QWORD *dest_p64 = (QWORD *)dest;
#endif

  /* Copy 8 uint64_t one time */
  m = n>>3;
  for (i = 0; i < m; i ++)
  {
    j = i<<3;
    dest_p64[j+0] = src_p64[j+0];
    dest_p64[j+1] = src_p64[j+1];
    dest_p64[j+2] = src_p64[j+2];
    dest_p64[j+3] = src_p64[j+3];
    dest_p64[j+4] = src_p64[j+4];
    dest_p64[j+5] = src_p64[j+5];
    dest_p64[j+6] = src_p64[j+6];
    dest_p64[j+7] = src_p64[j+7];
  }

  /* Remainder 1 */
  for (i = n&(~0x07); i < n; i++)
    dest_p64[i] = src_p64[i];

  /* Remainder 2 */
  for (i = size&(~0x07); i < size; i++)
    dest_p8[i] = src_p8[i];

  return dest;
}

/* TODO: Rename misleading "syscall_table" and make it dynamic
 *       using the Slab Memory Allocator. --Salvo
 */
static struct symbol syscall_table[] = {
  { "cputc", (DWORD)cputc },
  { "cputs", (DWORD)cputs },
  { "place", (DWORD)place },
  { "cursor", (DWORD)cursor },
  { "getcursorxy", (DWORD)getcursorxy },
  { "get_attr", (DWORD)get_attr },
  { "message", (DWORD)message },
  { "ksprintf", (DWORD)ksprintf },
  { "fd32_abort", (DWORD)fd32_abort },
  { "fd32_reboot", (DWORD)fd32_reboot},
  { "restore_sp", (DWORD)restore_sp },
  { "dosmem_get", (DWORD)dosmem_get },
  { "dosmem_get_region", (DWORD)dosmem_get_region },
  { "dosmem_free", (DWORD)dosmem_free },
  { "mem_get",  (DWORD)mem_get },
  { "mem_get_region", (DWORD)mem_get_region },
  { "mem_free", (DWORD)mem_free },
  { "mem_dump", (DWORD)mem_dump },
  /* Slab Memory Allocator */
  { "slabmem_alloc",   (DWORD) slabmem_alloc   },
  { "slabmem_free",    (DWORD) slabmem_free    },
  { "slabmem_create",  (DWORD) slabmem_create  },
  { "slabmem_destroy", (DWORD) slabmem_destroy },
  /* BIOS data area */
  { "bios_da", (DWORD)0x400 },
  /* OSlib Low level calls & data */
  { "rm_irq_table", (DWORD)(&rm_irq_table) },
  { "exc_table", (DWORD)(&exc_table) },
  { "get_syscall_table", (DWORD)get_syscall_table },
  { "x86_get_cpu", (DWORD)x86_get_cpu },
  { "kern_CS", (DWORD)(&kern_CS) },
  { "kern_DS", (DWORD)(&kern_DS) },
  { "vm86_get_tss", (DWORD)vm86_get_tss },
  { "vm86_callBIOS", (DWORD)vm86_callBIOS },
  { "vm86_init", (DWORD)vm86_init },
  { "ll_context_save", (DWORD)ll_context_save },
  { "ll_context_load", (DWORD)ll_context_load },
  { "ll_context_to",   (DWORD)ll_context_to },
/*
 * { "DOS_alloc", (DWORD)DOS_alloc },
 * { "DOS_free", (DWORD)DOS_free },
 */
  { "irq_unmask", (DWORD)irq_unmask },
  { "add_call", (DWORD)add_call },
  { "l1_int_bind", (DWORD)l1_int_bind },
  {"fd32_cpu_idle", (DWORD)fd32_cpu_idle},
  /* Symbols for GDT and IDT handling */
  { "IDT",       (DWORD) (&IDT)      },
  { "idt_place", (DWORD) idt_place   },
  { "gdt_place", (DWORD) gdt_place   },
  { "GDT_base",  (DWORD) (&GDT_base) },
  { "gdt_read",  (DWORD) gdt_read    },
  /* Symbols for libc functions */
  { "memcpy",  (DWORD) imp_memcpy },
  { "strchr",  (DWORD) strchr  },
  { "strcpy",  (DWORD) strcpy  },
  { "strncpy", (DWORD) strncpy },
  { "strlen",  (DWORD) strlen  },
  { "strcmp",  (DWORD) strcmp  },
  { "toupper", (DWORD) toupper },
  { "strtoi",    (DWORD) strtoi},
  { "strcasecmp",  (DWORD) strcasecmp  },
  { "strncasecmp", (DWORD) strncasecmp },
  { "stricmp",   (DWORD) stricmp },
  { "strnicmp",  (DWORD) strnicmp },
  { "strpbrk",  (DWORD) strpbrk },
  { "strsep",  (DWORD) strsep },
  { "strspn",  (DWORD) strspn },
  { "strcspn", (DWORD) strcspn },
  { "strlcpy", (DWORD) strlcpy },
  { "strcat",  (DWORD) strcat },
  { "strncat", (DWORD) strncat },
  { "strlcat", (DWORD) strlcat },
  { "strncmp", (DWORD) strncmp },
  { "strnchr", (DWORD) strnchr },
  { "strrchr", (DWORD) strrchr },
  { "strstr",  (DWORD) strstr },
  { "strnlen", (DWORD) strnlen },
  { "strscn",  (DWORD) strscn },
  { "strupr",  (DWORD) strupr },
  { "strlwr",  (DWORD) strlwr },
  { "getopt",  (DWORD) getopt },
  { "getopt_long", (DWORD) getopt_long },
  { "optarg",  (DWORD) &optarg },
  { "optind",  (DWORD) &optind },
  { "opterr",  (DWORD) &opterr },
  { "optopt",  (DWORD) &optopt },
  /* Symbols for date and time functions (from fd32time.h) */
  { "fd32_get_date", (DWORD) fake_get_date },
  { "fd32_get_time", (DWORD) fake_get_time },
  { "delay",         (DWORD) imp_delay },
  /* Symbols for logging functions (from logger.h) */
  { "fd32_log_printf", (DWORD) fd32_log_printf },
  { "fd32_log_stats", (DWORD) fd32_log_stats },

  /* in DPMI module: Symbols for Descriptor Management services */

  /* Symbols for Device Engine services (from devices.h) */
  { "fd32_dev_get",        (DWORD) fd32_dev_get        },
  { "fd32_dev_search",     (DWORD) fd32_dev_search     },
  { "fd32_dev_first",      (DWORD) fd32_dev_first      },
  { "fd32_dev_last",       (DWORD) fd32_dev_last,      },
  { "fd32_dev_next",       (DWORD) fd32_dev_next,      },
  { "fd32_dev_prev",       (DWORD) fd32_dev_prev,      },
  { "fd32_dev_register",   (DWORD) fd32_dev_register   },
  { "fd32_dev_unregister", (DWORD) fd32_dev_unregister },
  { "fd32_dev_replace",    (DWORD) fd32_dev_replace    },
  /* Symbols for File System services (from filesys.h) */
  { "fd32_mkdir",             (DWORD) fd32_mkdir             },
  { "fd32_rmdir",             (DWORD) fd32_rmdir             },
  { "fd32_add_fs",            (DWORD) fd32_add_fs            },
  { "fd32_get_drive",         (DWORD) fd32_get_drive         },
  { "fd32_set_default_drive", (DWORD) fd32_set_default_drive },
  { "fd32_get_default_drive", (DWORD) fd32_get_default_drive },
  { "fd32_open",              (DWORD) fd32_open              },
  { "fd32_get_dev_info", (DWORD)fd32_get_dev_info},
  { "fd32_close",             (DWORD) fd32_close             },
  { "fd32_fflush",            (DWORD) fd32_fflush            },
  { "fd32_read",              (DWORD) fd32_read              },
  { "fd32_write",             (DWORD) fd32_write             },
  { "fd32_unlink",            (DWORD) fd32_unlink            },
  { "fd32_lseek",             (DWORD) fd32_lseek             },
  { "fd32_dup",               (DWORD) fd32_dup               },
  { "fd32_forcedup",          (DWORD) fd32_forcedup          },
  { "fd32_get_attributes",    (DWORD) fd32_get_attributes    },
  { "fd32_set_attributes",    (DWORD) fd32_set_attributes    },
  { "fd32_rename",            (DWORD) fd32_rename            },
  { "fd32_get_fsinfo",        (DWORD) fd32_get_fsinfo        },
  { "fd32_get_fsfree",        (DWORD) fd32_get_fsfree        },
  { "fd32_dos_findfirst",     (DWORD) fd32_dos_findfirst     },
  { "fd32_dos_findnext",      (DWORD) fd32_dos_findnext      },
  { "fd32_lfn_findfirst",     (DWORD) fd32_lfn_findfirst     },
  { "fd32_lfn_findnext",      (DWORD) fd32_lfn_findnext      },
  { "fd32_lfn_findclose",     (DWORD) fd32_lfn_findclose     },
  { "fd32_create_subst",      (DWORD) fd32_create_subst      },
  { "fd32_terminate_subst",   (DWORD) fd32_terminate_subst   },
  { "fd32_query_subst",       (DWORD) fd32_query_subst       },
  { "fd32_chdir",             (DWORD) fd32_chdir             },
  { "fd32_getcwd",            (DWORD) fd32_getcwd            },
  { "fd32_truename",          (DWORD) fd32_truename          },
  { "fd32_sfn_truename",      (DWORD) fd32_sfn_truename      },
  /* in unicode module: Symbols for Unicode support (from unicode.h) */
  /* in nls module: Symbols for NLS support (from nls.h) */
  /* TODO: Remove the following from the kernel */
	{ "block_enumerate",  (DWORD) block_enumerate  },
	{ "block_get",        (DWORD) block_get        },
	{ "block_register",   (DWORD) block_register   },
	{ "block_unregister", (DWORD) block_unregister },

  { "event_post", (DWORD) (&event_post) },
  { "event_delete", (DWORD) (&event_delete) },
  { "ll_gettime", (DWORD) ll_gettime },
  /* Symbols for Exec and Process */
  { "fd32_get_binfmt",     (DWORD) fd32_get_binfmt },
  { "fd32_set_binfmt",     (DWORD) fd32_set_binfmt },
  { "fd32_exec_process",   (DWORD) fd32_exec_process },
  { "fd32_load_process",   (DWORD) fd32_load_process },
  { "fd32_create_process", (DWORD) fd32_create_process },
  { "fd32_get_current_pi", (DWORD) fd32_get_current_pi },
  { "fd32_set_current_pi", (DWORD) fd32_set_current_pi },
  { "fd32_get_argv",       (DWORD) fd32_get_argv },
  { "fd32_unget_argv",     (DWORD) fd32_unget_argv },
  { "fd32_kernel_open",    (DWORD) fd32_kernel_open },
  { "fd32_kernel_read",    (DWORD) fd32_kernel_read },
  { "fd32_kernel_seek",    (DWORD) fd32_kernel_seek },
  { "fd32_kernel_close",   (DWORD) fd32_kernel_close },

  { "add_dll_table", (DWORD) add_dll_table },
  { "get_dll_table", (DWORD) get_dll_table },
  {"fd32_init_jft", (DWORD)fd32_init_jft},
  {"fd32_free_jft", (DWORD)fd32_free_jft},
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  EMPTY_SLOT,
  {0, 0}
};

void *get_syscall_table(void) 
{
  return syscall_table;
}

DWORD get_syscall_table_size(void)
{
  return sizeof(syscall_table)/sizeof(struct symbol);
}

int add_call(char *name, DWORD address, int mode)
{
  int i;
  int done;
  int res;
  
  i = 0;
  done = 0;
  while ((!done) && (syscall_table[i].address != 0)) {
    if (!strcmp(syscall_table[i].name, name)) {
      done = 1;
    } else {
      i++;
    }
  }

  if (done == 1) {
    if (mode == SUBSTITUTE) {
      res = syscall_table[i].address;
      syscall_table[i].name = name;
      syscall_table[i].address = address;
    } else {
      return -1;
    }
  } else {
    if (mode == SUBSTITUTE) {
      return -1;
    } else {
      i = 0;
      while (syscall_table[i].name != 0) {
        i++;
      }
      
      if (syscall_table[i].address != 0xFFFFFFFF) {
        return -1;
      }
      res = 1;
      syscall_table[i].name = name;
      syscall_table[i].address = address;    
    }
  }
  
  return res;
}


/** \brief DLL Table management...
 *
 * - implement with linked list
 * - pre-install FD32 syscalls' DLL
 *
 *  \author Hanzac Chen 
 */

/** \struct dll_int_table
 *  \brief DLL internal table */
struct dll_int_table
{
  DWORD ref_num;               /**< count of references that used it */
  DWORD handle;                /**< handle pointer to the DLL module */
  struct dll_table *dll;       /**< pointer to the DLL table */
  struct dll_int_table *next;  /**< link to next DLL table */
};

#define HANDLE_OF_FD32_SYS 0xFFFFFFFF
/** \brief FD32 syscalls' DLL */
static struct dll_table fd32_sys_dll = {
  "fd32-sys.dll", sizeof(syscall_table)/sizeof(struct symbol), syscall_table
};
/** \brief DLL management internal list */
static struct dll_int_table dll_int_list = {
  0x00000001, HANDLE_OF_FD32_SYS, &fd32_sys_dll, NULL
};

/* TODO: free_dll function, when the reference count is zero */
void free_dll(DWORD dll_handle)
{
/* Because every symbol name's memory is in the image memory
        
     for(j = 0; j < dylt_array[i].symbol_num; j++)
     mem_free((DWORD)s[i].name, strlen(s[i].name)+1);
     mem_free((DWORD)dylt_array[i].name, strlen(dylt_array[i].name)+1);
        
   So when we free the image we free all the memory */
/*
    mem_free((DWORD)symbol_table, sizeof(struct symbol)*sy);
    destroy_process(dll_handle);
*/
}
/* void load_dll(DWORD dll_handle) {} */

/** \brief Add the DLL export functions' table
 *
 * - Return TRUE(1) if succeeded
 * - Return EXISTED(2)
 * - Return FALSE(0) if failed
 *  \note if the DLL is created and loaded in application runtime,
 *        the handle is the image_base otherwise, the handle is
 *        system defined numbers, e.g. in the WINB module
 */
int add_dll_table(char *dll_name, DWORD handle, DWORD symbol_num, struct symbol *symbol_table)
{
  DWORD mem;
  struct dll_int_table *p, *q;

  /* Search for existing DLL with the same name */
  for (p = &dll_int_list; p != NULL; q = p, p = p->next) {
    if (strcmp(strlwr(dll_name), p->dll->name) == 0) {
      return 2;
    }
  }
  /* Construct a new DLL */
  mem = mem_get(sizeof(struct dll_int_table)+sizeof(struct dll_table));
  p = (struct dll_int_table *)mem;
  p->handle = handle;
  p->ref_num = 1;
  p->next = NULL;
  p->dll = (struct dll_table *)(mem+sizeof(struct dll_int_table));
  p->dll->name = strlwr(dll_name);
  p->dll->symbol_num = symbol_num;
  p->dll->symbol_table = symbol_table;
  
  /* Link at the end of list */
  q->next = p;

  fd32_log_printf("Add DLL %s at 0x%x\n", dll_name, (int)mem);
  
  return 1;
}

struct dll_table *get_dll_table(char *dll_name)
{
  struct dll_int_table *p, *q;
  strlwr(dll_name);

  /* Search for the DLL */
  for (p = &dll_int_list; p != NULL; q = p, p = p->next) {
    if (strcmp(dll_name, p->dll->name) == 0) {
      return p->dll;
    }
  }

  /* Find and Load the DLL */
  message("Find and Load %s ...\n", dll_name);
  /* dos_exec(dll_name, 0, 0, 0, 0, &retval); */
  /* TODO: load the DLL, add the DLL table */

  /* Search for the DLL again */
  for (p = &dll_int_list; p != NULL; q = p, p = p->next) {
    if (strcmp(dll_name, p->dll->name) == 0) {
      return p->dll;
    }
  }

  /* return NULL if failed */
  return NULL;
}
