/* FD32 kernel
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#ifndef __KERNEL_H__
#define __KERNEL_H_

int create_process(DWORD entry, DWORD base, DWORD size, char *name);
void fd32_abort(void);
void kernel_init();

#define SUBSTITUTE 1
#define ADD 0
void *get_syscall_table(void);
int add_call(char *name, DWORD address, int mode);
#endif
