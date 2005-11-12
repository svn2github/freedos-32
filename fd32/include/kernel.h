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

void fd32_abort(void);
void fd32_reboot(void);
void fd32_cpu_idle(void);
void kernel_init();

#define SUBSTITUTE 1
#define ADD 0
void *get_syscall_table(void);
int add_call(char *name, DWORD address, int mode);

int add_dll_table(char *dll_name, DWORD handle, DWORD symbol_num, struct symbol *symbol_table);
struct dll_table *get_dll_table(char *dll_name);

void *fd32_init_jft(int jft_size); /* Implemented in filesys\jft.c */
void fd32_free_jft(void *p, int jft_size); /* idem */

#endif
