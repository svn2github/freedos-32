#ifndef __SYSCALLS__
#define __SYSCALLS__

// FIXME: get rid of this!
#include "stdint.h"

#define FD32_FAALL    0x3F
#define FD32_FRNONE   0x00
#define FD32_ARDONLY  0x01

#define FD32_PAGE_SIZE 0x1000
 
typedef struct fd32_date 
{
  uint16_t Year;
  uint8_t Day;
  uint8_t Mon;
  uint8_t weekday;
}
fd32_date_t;

typedef struct fd32_time 
{
  uint8_t Min;
  uint8_t Hour;
  uint8_t Hund;
  uint8_t Sec;
}
fd32_time_t;

/* Kernel process management
 * from fd32/include/kernel.h
 */
typedef struct process_info {
  uint32_t type;
  void *psp;		/* Optional DOS PSP */
  void *cds_list;	/* Under DOS this is a global array */
  char *args;
  char *filename;
  uint32_t memlimit;
  void *jft;
  uint32_t jft_size;
} process_info_t;

process_info_t *fd32_get_current_pi(void);
void fd32_set_current_pi(process_info_t *ppi);
int fd32_get_argv(char *filename, char *args, char ***_pargv);
int fd32_unget_argv(int _argc, char *_argv[]); /* Recover the original args and free the argv */

int message(char *fmt,...) __attribute__((format(printf,1,2)));
int fd32_log_printf(char *fmt, ...) __attribute__ ((format(printf,1,2)));

void fd32_abort(void);
void restore_sp(int res);

int mem_get_region(uint32_t base, uint32_t size);
uint32_t mem_get(uint32_t amount);
int mem_free(uint32_t base, uint32_t size);
void mem_dump(void);

int fd32_get_dev_info(int fd);
int fd32_get_date(fd32_date_t *);
int fd32_get_time(fd32_time_t *);

void *fd32_init_jft(int JftSize);
void fd32_free_jft(void *p, int jft_size);
int fd32_unlink(char *FileName, uint32_t Flags);
int fd32_open  (char *FileName, uint32_t Mode, uint16_t Attr, uint16_t AliasHint, int *Result);
int fd32_close (int Handle);
int fd32_read  (int Handle, void *Buffer, int Size);
int fd32_write (int Handle, void *Buffer, int Size);
int fd32_lseek (int Handle, long long int Offset, int Origin, long long int *Result);

#endif
