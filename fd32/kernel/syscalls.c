/* FD32 Kernel exported symbols table & management functions
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/hw-data.h>
#include <ll/i386/hw-func.h>
#include <ll/i386/hw-arch.h>
#include <ll/i386/pic.h>
#include <ll/i386/x-bios.h>
#include <ll/i386/x-dosmem.h>
#include <ll/i386/cons.h>
#include <ll/i386/error.h>
#include <ll/stdlib.h>
#include <ll/stdio.h>
#include <ll/string.h>
#include <ll/ctype.h>

#include "kernel.h"
#include "kmem.h"
#include "format.h"
#include "logger.h"
#include "devices.h"
#include "filesys.h"
#include "unicode.h"
#include "nls.h"
#include "time.h"

extern DWORD ll_exc_table[16];
extern struct handler exc_table[32];
extern GATE IDT[256];
extern DWORD rm_irq_table[256];
extern WORD kern_CS, kern_DS;
extern CONTEXT context_save(void);
extern union gdt_entry *GDT_base;


void restore_sp(int res);
extern struct psp *current_psp;

#define EMPTY_SLOT {0, (DWORD)0xFFFFFFFF}

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

static struct symbol syscall_table[] = {
  { "cputc", (DWORD)cputc },
  { "place", (DWORD)place },
  { "getcursorxy", (DWORD)getcursorxy },
  { "get_attr", (DWORD)get_attr },
  { "message", (DWORD)message },
  { "ksprintf", (DWORD)ksprintf },
  { "fd32_abort", (DWORD)fd32_abort },
  { "restore_sp", (DWORD)restore_sp },
  { "mem_get", (DWORD)mem_get },
  { "mem_get_region", (DWORD)mem_get_region },
  { "mem_free", (DWORD)mem_free },
  { "current_psp", (DWORD)(&current_psp) },
  { "rm_irq_table", (DWORD)(&rm_irq_table) },
  { "exc_table", (DWORD)(&exc_table) },
  { "get_syscall_table", (DWORD)get_syscall_table },
  { "X86_get_CPU", (DWORD)X86_get_CPU },
  { "kern_CS", (DWORD)(&kern_CS) },
  { "kern_DS", (DWORD)(&kern_DS) },
  { "vm86_get_tss", (DWORD)vm86_get_tss },
  { "vm86_callBIOS", (DWORD)vm86_callBIOS },
  { "vm86_init", (DWORD)vm86_init },
  { "context_save", (DWORD)context_save },
  { "DOS_alloc", (DWORD)DOS_alloc },
  { "DOS_free", (DWORD)DOS_free },
  { "irq_unmask", (DWORD)irq_unmask },
  { "add_call", (DWORD)add_call },
  { "x_irq_bind", (DWORD)x_irq_bind },
  { "dos_exec", (DWORD)dos_exec },
  /* Symbols for GDT and IDT handling */
  { "IDT",       (DWORD) (&IDT)      },
  { "IDT_place", (DWORD) IDT_place   },
  { "GDT_place", (DWORD) GDT_place   },
  { "GDT_base",  (DWORD) (&GDT_base) },
  { "GDT_read",  (DWORD) GDT_read    },
  /* Symbols for libc functions */
  { "strcpy",  (DWORD) strcpy  },
  { "strncpy", (DWORD) strncpy },
  { "strlen",  (DWORD) strlen  },
  { "strcmp",  (DWORD) strcmp  },
  { "toupper", (DWORD) toupper },
  { "dcvt",    (DWORD) dcvt    },
  /* Symbols for date and time functions (from time.h) */
  { "fd32_get_date", (DWORD) fake_get_date },
  { "fd32_get_time", (DWORD) fake_get_time },
  /* Symbols for logging cunctions (from logger.h) */
  { "fd32_log_printf", (DWORD) fd32_log_printf },
#if 0
  /* Symbols for Descriptor Management services */
  { "fd32_allocate_descriptors",         (DWORD) fd32_allocate_descriptors         },
  { "fd32_free_descriptor",              (DWORD) fd32_free_descriptor              },
  { "fd32_segment_to_descriptor",        (DWORD) fd32_segment_to_descriptor        },
  { "fd32_get_selector_increment_value", (DWORD) fd32_get_selector_increment_value },
  { "fd32_get_segment_base_address",     (DWORD) fd32_get_segment_base_address     },
  { "fd32_set_segment_base_address",     (DWORD) fd32_set_segment_base_address     },
  { "fd32_set_segment_limit",            (DWORD) fd32_set_segment_limit            },
  { "fd32_set_descriptor_access_rights", (DWORD) fd32_set_descriptor_access_rights },
  { "fd32_create_alias_descriptor",      (DWORD) fd32_create_alias_descriptor      },
  { "fd32_get_descriptor",               (DWORD) fd32_get_descriptor               },
  { "fd32_set_descriptor",               (DWORD) fd32_set_descriptor               },
#endif
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
  { "fd32_dos_findfirst",     (DWORD) fd32_dos_findfirst     },
  { "fd32_dos_findnext",      (DWORD) fd32_dos_findnext      },
  { "fd32_lfn_findfirst",     (DWORD) fd32_lfn_findfirst     },
  { "fd32_lfn_findnext",      (DWORD) fd32_lfn_findnext      },
  { "fd32_lfn_findclose",     (DWORD) fd32_lfn_findclose     },
  { "fd32_gen_short_fname",   (DWORD) fd32_gen_short_fname   },
  { "fd32_build_fcb_name",    (DWORD) fd32_build_fcb_name    },
  { "fd32_expand_fcb_name",   (DWORD) fd32_expand_fcb_name   },
  { "fd32_compare_fcb_names", (DWORD) fd32_compare_fcb_names },
  { "fd32_create_subst",      (DWORD) fd32_create_subst      },
  { "fd32_terminate_subst",   (DWORD) fd32_terminate_subst   },
  { "fd32_query_subst",       (DWORD) fd32_query_subst       },
  { "fd32_chdir",             (DWORD) fd32_chdir             },
  { "fd32_truename",          (DWORD) fd32_truename          },
  { "fd32_sfn_truename",      (DWORD) fd32_sfn_truename      },
  { "utf8_fnameicmp",         (DWORD) utf8_fnameicmp         },
  /* Symbols for Unicode support (from unicode.h) */
  { "fd32_utf16to32",  (DWORD) fd32_utf16to32  },
  { "fd32_utf32to16",  (DWORD) fd32_utf32to16  },
  { "fd32_utf8to32",   (DWORD) fd32_utf8to32   },
  { "fd32_utf32to8",   (DWORD) fd32_utf32to8   },
  { "utf8_stricmp",    (DWORD) utf8_stricmp    },
  { "utf8_strupr",     (DWORD) utf8_strupr     },
  { "fd32_utf8to16",   (DWORD) fd32_utf8to16   },
  { "fd32_utf16to8",   (DWORD) fd32_utf16to8   },
  { "unicode_toupper", (DWORD) unicode_toupper },
  /* Symbols for NLS support (from nls.h) */
  { "oemcp_to_utf8",  (DWORD) oemcp_to_utf8  },
  { "utf8_to_oemcp",  (DWORD) utf8_to_oemcp  },
  { "oemcp_skipchar", (DWORD) oemcp_skipchar },
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
