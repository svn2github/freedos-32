/* FD32 kernel
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#ifndef __EXEC_H__
#define __EXEC_H__

int exec_process(struct kern_funcs *p, int file, struct read_funcs *parser,
		char *cmdline, char *args);

#endif
