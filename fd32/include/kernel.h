/* FD32 kernel
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#ifndef __KERNEL_H__
#define __KERNEL_H_

int dos_exec(char *filename, DWORD env_segment, DWORD cmd_tail,
	DWORD fcb1, DWORD fcb2, WORD *return_value);

int create_process(DWORD entry, DWORD base, DWORD size, char *name);
void fd32_abort(void);
void kernel_init();

#define SUBSTITUTE 1
#define ADD 0
void *get_syscall_table(void);
int add_call(char *name, DWORD address, int mode);
void fd32_cpu_idle(void);
#endif
