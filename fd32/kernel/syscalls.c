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

/**
 * memset - Fill a region of memory with the given value
 * @s: Pointer to the start of the area.
 * @c: The byte to fill the area with
 * @count: The size of the area.
 *
 * Do not use memset() to access IO space, use memset_io() instead.
 */
void * imp_memset(void * s, int c, size_t count)
{
	char *xs = (char *) s;

	while (count--)
		*xs++ = c;

	return s;
}

/* TODO: Rename misleading "syscall_table" and make it dynamic
 *       using the Slab Memory Allocator. --Salvo
 */
static struct symbol syscall_table[] = {
  { "cputc", cputc },
  { "cputs", cputs },
  { "place", place },
  { "cursor", cursor },
  { "getcursorxy", getcursorxy },
  { "get_attr", get_attr },
  { "message", message },
  { "ksprintf", ksprintf },
  { "restore_sp", restore_sp },
  { "dosmem_get", dosmem_get },
  { "dosmem_get_region", dosmem_get_region },
  { "dosmem_free", dosmem_free },
  { "dosmem_resize", dosmem_resize },
  { "mem_get",  mem_get },
  { "mem_get_region", mem_get_region },
  { "mem_free", mem_free },
  { "mem_dump", mem_dump },
  /* FD32 functions */
  { "fd32_abort", fd32_abort },
  { "fd32_reboot", fd32_reboot},
  { "fd32_cpu_idle", fd32_cpu_idle},
  { "fd32_add_call", fd32_add_call },
  { "fd32_get_call", fd32_get_call },
  { "add_dll_table", add_dll_table },
  { "get_dll_table", get_dll_table },
  /* Slab Memory Allocator */
  { "slabmem_alloc",    slabmem_alloc   },
  { "slabmem_free",     slabmem_free    },
  { "slabmem_create",   slabmem_create  },
  { "slabmem_destroy",  slabmem_destroy },
  /* BIOS data area */
  { "bios_da", (void *)0x400 },
  /* OSlib Low level calls & data */
  { "rm_irq_table", &rm_irq_table },
  { "exc_table", &exc_table },
  { "x86_get_cpu", x86_get_cpu },
  { "kern_CS", &kern_CS },
  { "kern_DS", &kern_DS },
  { "vm86_get_tss",  vm86_get_tss  },
  { "vm86_call",     vm86_call     },
  { "vm86_callBIOS", vm86_callBIOS },
  { "vm86_init",     vm86_init     },
  { "ll_context_save", ll_context_save },
  { "ll_context_load", ll_context_load },
  { "ll_context_to",   ll_context_to   },
  { "irq_unmask", irq_unmask },
  { "l1_int_bind", l1_int_bind },
  /* Symbols for GDT and IDT handling */
  { "IDT",        &IDT      },
  { "idt_place",  idt_place   },
  { "gdt_place",  gdt_place   },
  { "GDT_base",   &GDT_base },
  { "gdt_read",   gdt_read    },
  /* Symbols for libc functions */
  { "memcpy",   imp_memcpy },
  { "memset",   imp_memset },
  { "strchr",   strchr  },
  { "strcpy",   strcpy  },
  { "strncpy",  strncpy },
  { "strlen",   strlen  },
  { "strcmp",   strcmp  },
  { "toupper",  toupper },
  { "strtoi",     strtoi},
  { "strcasecmp",   strcasecmp  },
  { "strncasecmp",  strncasecmp },
  { "stricmp",    stricmp },
  { "strnicmp",   strnicmp },
  { "strpbrk",   strpbrk },
  { "strsep",   strsep },
  { "strspn",   strspn },
  { "strcspn",  strcspn },
  { "strlcpy",  strlcpy },
  { "strcat",   strcat },
  { "strncat",  strncat },
  { "strlcat",  strlcat },
  { "strncmp",  strncmp },
  { "strnchr",  strnchr },
  { "strrchr",  strrchr },
  { "strstr",   strstr },
  { "strnlen",  strnlen },
  { "strscn",   strscn },
  { "strupr",   strupr },
  { "strlwr",   strlwr },
  { "getopt",   getopt },
  { "getopt_long",  getopt_long },
  { "optarg",   &optarg },
  { "optind",   &optind },
  { "opterr",   &opterr },
  { "optopt",   &optopt },
  /* Symbols for date and time functions (from fd32time.h) */
  { "fd32_get_date",  fake_get_date },
  { "fd32_get_time",  fake_get_time },
  /* Symbols for logging functions (from logger.h) */
  { "fd32_log_printf",  fd32_log_printf },
  { "fd32_log_stats",  fd32_log_stats },

  /* in DPMI module: Symbols for Descriptor Management services */

  /* Symbols for Device Engine services (from devices.h) */
  { "fd32_dev_get",         fd32_dev_get        },
  { "fd32_dev_search",      fd32_dev_search     },
  { "fd32_dev_first",       fd32_dev_first      },
  { "fd32_dev_last",        fd32_dev_last,      },
  { "fd32_dev_next",        fd32_dev_next,      },
  { "fd32_dev_prev",        fd32_dev_prev,      },
  { "fd32_dev_register",    fd32_dev_register   },
  { "fd32_dev_unregister",  fd32_dev_unregister },
  { "fd32_dev_replace",     fd32_dev_replace    },
  /* Symbols for File System services (from filesys.h) */
  { "fd32_mkdir",              fd32_mkdir             },
  { "fd32_rmdir",              fd32_rmdir             },
  { "fd32_add_fs",             fd32_add_fs            },
  { "fd32_get_drive",          fd32_get_drive         },
  { "fd32_set_default_drive",  fd32_set_default_drive },
  { "fd32_get_default_drive",  fd32_get_default_drive },
  { "fd32_open",               fd32_open              },
  { "fd32_get_dev_info", fd32_get_dev_info},
  { "fd32_close",              fd32_close             },
  { "fd32_fflush",             fd32_fflush            },
  { "fd32_read",               fd32_read              },
  { "fd32_write",              fd32_write             },
  { "fd32_unlink",             fd32_unlink            },
  { "fd32_lseek",              fd32_lseek             },
  { "fd32_dup",                fd32_dup               },
  { "fd32_forcedup",           fd32_forcedup          },
  { "fd32_get_attributes",     fd32_get_attributes    },
  { "fd32_set_attributes",     fd32_set_attributes    },
  { "fd32_rename",             fd32_rename            },
  { "fd32_get_fsinfo",         fd32_get_fsinfo        },
  { "fd32_get_fsfree",         fd32_get_fsfree        },
  { "fd32_dos_findfirst",      fd32_dos_findfirst     },
  { "fd32_dos_findnext",       fd32_dos_findnext      },
  { "fd32_lfn_findfirst",      fd32_lfn_findfirst     },
  { "fd32_lfn_findnext",       fd32_lfn_findnext      },
  { "fd32_lfn_findclose",      fd32_lfn_findclose     },
  { "fd32_create_subst",       fd32_create_subst      },
  { "fd32_terminate_subst",    fd32_terminate_subst   },
  { "fd32_query_subst",        fd32_query_subst       },
  { "fd32_chdir",              fd32_chdir             },
  { "fd32_getcwd",             fd32_getcwd            },
  { "fd32_truename",           fd32_truename          },
  { "fd32_sfn_truename",       fd32_sfn_truename      },
  { "fd32_init_jft",  fd32_init_jft },
  { "fd32_free_jft",  fd32_free_jft },
  { "fd32_drive_read",      fd32_drive_read },
  { "fd32_drive_write",     fd32_drive_write },
  { "fd32_drive_get_sector_size",  fd32_drive_get_sector_size },
  { "fd32_drive_get_sector_count", fd32_drive_get_sector_count },
  { "fd32_drive_count",     fd32_drive_count },
  { "fd32_drive_get_first", fd32_drive_get_first },
  { "fd32_drive_get_next",  fd32_drive_get_next },
  { "fd32_drive_set_parameter_block",  fd32_drive_set_parameter_block },
  { "fd32_drive_get_parameter_block",  fd32_drive_get_parameter_block },
  /* in unicode module: Symbols for Unicode support (from unicode.h) */
  /* in nls module: Symbols for NLS support (from nls.h) */
  /* TODO: Remove the following from the kernel */
	{ "block_enumerate",   block_enumerate  },
	{ "block_get",         block_get        },
	{ "block_register",    block_register   },
	{ "block_unregister",  block_unregister },
  /* { "event_post",  (&event_post) },
     { "event_delete",  (&event_delete) },
     { "ll_gettime",  ll_gettime }, */
  /* Symbols for Exec and Process */
  { "fd32_get_binfmt",     fd32_get_binfmt },
  { "fd32_set_binfmt",     fd32_set_binfmt },
  { "fd32_exec_process",   fd32_exec_process },
  { "fd32_load_process",   fd32_load_process },
  { "fd32_start_process",  fd32_start_process },
  { "fd32_stop_process",   fd32_stop_process },
  { "fd32_new_process",    fd32_new_process },
  { "fd32_get_current_pi", fd32_get_current_pi },
  { "fd32_set_current_pi", fd32_set_current_pi },
  { "fd32_get_argv",       fd32_get_argv },
  { "fd32_unget_argv",     fd32_unget_argv },
  { "fd32_kernel_open",    fd32_kernel_open },
  { "fd32_kernel_read",    fd32_kernel_read },
  { "fd32_kernel_seek",    fd32_kernel_seek },
  { "fd32_kernel_close",   fd32_kernel_close },
};

DWORD get_syscall_table_size(void)
{
  return sizeof(syscall_table)/sizeof(struct symbol);
}

/* The dynamic system call table */
#include <list.h>
static List syscall_table_d = { NULL, NULL, 0 };

/* Add a system call, return the previous address if substitute a existing call */
int fd32_add_call(const char *name, void *address, int mode)
{
  unsigned int i;
  int done = 0;
  int res = -1;

  for (i = 0; i < sizeof(syscall_table)/sizeof(struct symbol); i++) {
    if (!strcmp(syscall_table[i].name, name)) {
      /* Case sensitive */
      done = 1;
      break;
    }
  }

  if (done && mode == SUBSTITUTE) {
    res = (int)syscall_table[i].address;
    syscall_table[i].name = name;
    syscall_table[i].address = address;
  } else if (!done && mode == ADD) {
    ListItem *p = (ListItem *)mem_get(sizeof(ListItem)+sizeof(struct symbol));
    struct symbol *call;
    if (p != NULL) {
      list_push_back(&syscall_table_d, p);
      p++;
      call = (struct symbol *)p;
      call->name = name;
      call->address = address;
      res = 1;
    }
  }

  return res;
}

void *fd32_get_call(const char *name)
{
  unsigned int i;
  ListItem *p;
  struct symbol *call;

  /* Find in static system call table */
  for (i = 0; i < sizeof(syscall_table)/sizeof(struct symbol); i++) {
    if (strcmp(syscall_table[i].name, name) == 0) {
      return syscall_table[i].address;
    }
  }

  /* Find in dynamic system call table */
  for (p = syscall_table_d.begin; p != NULL; p = p->next) {
    call = (struct symbol *)(p+1);
    if (strcmp(call->name, name) == 0) {
      return call->address;
    }
  }

  return NULL;
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
