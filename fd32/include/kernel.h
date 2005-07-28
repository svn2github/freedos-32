/* FD32 kernel
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#ifndef __KERNEL_H__
#define __KERNEL_H__

#include "format.h"

struct process_info {
  char *args;
  DWORD memlimit;
  char *name;
};
static inline char *args_get(struct process_info *p)
{
  return p->args;
}
static inline char *name_get(struct process_info *p)
{
  return p->name;
}
static inline DWORD maxmem_get(struct process_info *p)
{
  return p->memlimit;
}

void create_dll(DWORD entry, DWORD base, DWORD size);
int create_process(DWORD entry, DWORD base, DWORD size, char *name, char *args);
void fd32_abort(void);
void fd32_reboot(void);
void kernel_init();
void fd32_cpu_idle(void);

#define SUBSTITUTE 1
#define ADD 0
void *get_syscall_table(void);
int add_call(char *name, DWORD address, int mode);

int add_dll_table(char *dll_name, DWORD handle, DWORD symbol_num, struct symbol *symbol_table);
struct dll_table *get_dll_table(char *dll_name);

extern void *fd32_init_jft(int JftSize); /* Implemented in filesys\jft.c */
void fd32_free_jft(void *p, int jft_size); /* idem */

int strcasecmp(const char *s1, const char *s2); /* Implemented in kernel/strcase.c */
int strncasecmp(const char *s1, const char *s2, int n); /* Implemented in kernel/strcase.c */

#endif
