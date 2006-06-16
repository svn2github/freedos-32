/* FD32 kernel
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#ifndef __EXEC_H__
#define __EXEC_H__

#include <ll/i386/hw-data.h>
#include "format.h"

#define NORMAL_PROCESS	0
#define DLL_PROCESS		1
#define VM86_PROCESS	2
#define RESIDENT		0x8000

/* Kernel Process management */
typedef struct process_info {
  struct process_info *prev;
  DWORD type;
  void *psp;		/* Optional DOS PSP */
  void *cds_list;	/* Under DOS this is a global array */
  char *args;
  char *filename;
  DWORD memlimit;
  void *jft;
  DWORD jft_size;
  void *cpu_context; /* CPU context for switching to another process */
} process_info_t;

process_info_t *fd32_get_current_pi(void);
void fd32_set_current_pi(process_info_t *ppi);
int fd32_get_argv(char *filename, char *args, char ***_pargv);
int fd32_unget_argv(int _argc, char *_argv[]); /* Recover the original args and free the argv */

typedef union process_params {
  struct {
    DWORD entry;
    DWORD base;
    DWORD size;
    WORD fs_sel;
  } normal;

  struct {
    void *prev_cpu_context;
    void *in_regs;
    void *out_regs;
    void *seg_regs;
    WORD ip;
    WORD sp;
  } vm86;
} process_params_t;
int fd32_create_process(process_info_t *ppi, process_params_t *pparams);
int fd32_exec_process(struct kern_funcs *kf, int file, struct read_funcs *rf, char *filename, char *args);
DWORD fd32_load_process(struct kern_funcs *kf, int file, struct read_funcs *rf, DWORD *exec_space, DWORD *image_base, DWORD *size);

void restore_sp(int res) __attribute__ ((noreturn)); /* Turn back the previous running state, after running a program */

#endif
