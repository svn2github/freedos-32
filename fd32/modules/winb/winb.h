#ifndef __WINB_HDR__
#define __WINB_HDR__

/* Mini Win32 Basic Support
 * by Hanzac Chen
 *
 * Note: Using a compiler with w32api
 * 
 * This is free software; see GPL.txt
 */


#include <stdint.h>


/* Only used for ``import'' symbols... 
 * from fd32/include/format.h
 */
struct symbol {
  char *name;
  void *address;
};

/* Kernel process management
 * from fd32/include/kernel.h
 */
typedef struct process_info {
  uint32_t type;
  void *psp;		/* Optional DOS PSP */
  void *cds_list;	/* Under DOS this is a global array */
  char *args;
  char *filename;
  void *mem_info;
  void *jft;
  uint32_t jft_size;
} process_info_t;

process_info_t *fd32_get_current_pi(void);
int fd32_get_argv(char *filename, char *args, char ***_pargv);
int fd32_unget_argv(int _argc, char *_argv[]); /* Recover the original args and free the argv */

/* Clear-up the local heaps */
void winb_mem_clear_up(void);

/*
 * from oslib/ll/i386/error.h
 */
int message(char *fmt, ...) __attribute__((format(printf, 1, 2)));
#define error(msg) message("Error! File:%s Line:%d %s", __FILE__, __LINE__, msg)

/*
 * from oslib/ll/i386/hw-func.h
 */
uint32_t gdt_read(uint16_t sel, uint32_t *lim, uint8_t *acc, uint8_t *gran);

/*
 * from fd32/include/filesys.h
 */
int fd32_fflush(int Handle);

/* Sends formatted output from the arguments (...) to the log buffer
 * from fd32/include/logger.h
 */
int fd32_log_printf(char *fmt, ...) __attribute__ ((format(printf, 1, 2)));

/*
 * from fd32/include/kernel.h
 */
void fd32_abort(void);
int add_dll_table(char *dll_name, uint32_t handle, uint32_t symbol_num, struct symbol *symbol_array);

/*
 * from fd32/include/kmem.h
 */
uint32_t mem_get(uint32_t amount);
uint32_t mem_get_region(uint32_t base, uint32_t size);
int mem_free(uint32_t base, uint32_t size);
void mem_dump(void);

/*
 * from newlib sys/unistd.h
 */
int ftruncate(int fd, off_t length);

#define HANDLE_OF_KERNEL32 0x01
#define HANDLE_OF_USER32   0x02
#define HANDLE_OF_GDI32    0x03
#define HANDLE_OF_ADVAPI32 0x04
#define HANDLE_OF_OLEAUT32 0x05
#define HANDLE_OF_MSVCRT   0x10
/* Note: MSVCRT handles (0x10 - 0x1F) should be reserved */

void install_kernel32(void);
void install_user32(void);
void install_advapi32(void);
void install_oleaut32(void);
void install_msvcrt(void);

#endif
