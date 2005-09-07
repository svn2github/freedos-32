/* FD32 kernel
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#ifndef __KERNEL_H__
#define __KERNEL_H__

#include "devices.h"
#include "format.h"

/* Kernel File handling */
struct kernel_file {
  fd32_request_t *request;
  void *file_id;
};
int fd32_kernel_open(const char *filename, DWORD mode, WORD attr, WORD alias_hint, struct kernel_file *f);
int fd32_kernel_close(int id);
int fd32_kernel_read(int id, void *buffer, int len);
int fd32_kernel_seek(int id, int pos, int whence);

/* Kernel Process management */
struct process_info {
  struct process_info *prev_P;
  void *psp;      /* Optional DOS PSP */
  void *cds_list; /* Under DOS this is a global array            */
  char *args;
  DWORD memlimit;
  char *name;
  void *jft;
  WORD  jft_size;
};
struct process_info *fd32_get_current_pi(void);
void fd32_set_current_pi(struct process_info *ppi);
int fd32_get_argv(char *filename, char *args, char ***_pargv);
int fd32_unget_argv(int _argc, char *_argv[]); /* Recover the original args and free the argv */

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

#endif
