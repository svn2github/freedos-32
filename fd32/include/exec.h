/* FD32 kernel
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#ifndef __EXEC_H__
#define __EXEC_H__

#include "format.h"
int dos_exec(char *filename, DWORD env_segment, char * args, DWORD fcb1, DWORD fcb2, WORD *return_value);
int fd32_exec_process(struct kern_funcs *kf, int file, struct read_funcs *rf, char *filename, char *args);
int fd32_create_process(DWORD entry, DWORD base, DWORD size, WORD fs_sel, char *filename, char *args);
DWORD fd32_load_process(struct kern_funcs *kf, int file, struct read_funcs *rf, DWORD *exec_space, DWORD *image_base, int *s);

#endif
