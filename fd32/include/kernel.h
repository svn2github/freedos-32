/* FD32 kernel
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#ifndef __KERNEL_H__
#define __KERNEL_H_

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

int dos_exec(char *filename, DWORD env_segment, char * args,
	DWORD fcb1, DWORD fcb2, WORD *return_value);

int create_process(DWORD entry, DWORD base, DWORD size, char *name, char *args);
void fd32_abort(void);
void fd32_reboot(void);
void kernel_init();

#define SUBSTITUTE 1
#define ADD 0
void *get_syscall_table(void);
int add_call(char *name, DWORD address, int mode);
void fd32_cpu_idle(void);

struct symbol;
int add_dll_table(char *dll_name, DWORD handle,
		DWORD symbol_num, struct symbol *symbol_array);
struct dll_table *get_dll_table(char *dll_name);


/* FIXME: Move these guys somewhere else? */
WORD stubinfo_init(DWORD base, DWORD image_end, DWORD mem_handle,
		char *filename, char *args);
struct read_funcs;
struct kern_funcs;
DWORD load_process(struct kern_funcs *p, int file, struct read_funcs *parser, DWORD *e_s, DWORD *image_base, int *s);

extern void *fd32_init_jft(int JftSize); /* Implemented in filesys\jft.c */
void fd32_free_jft(void *p, int jft_size); /* idem */
#endif
