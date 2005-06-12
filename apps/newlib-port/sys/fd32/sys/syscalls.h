#ifndef __SYSCALLS__
#define __SYSCALLS__

// FIXME: get rid of this!
#include "sys/stdint.h"


#define FD32_FAALL    0x3F
#define FD32_FRNONE   0x00
#define FD32_ARDONLY  0x01

struct psp {
  struct psp *link;
  uint32_t memlimit;
  uint32_t old_stack;
  uint32_t DOS_mem_buff;
  uint32_t info_sel;
  void *dta;
  void *cds_list;
  uint8_t gap[16];
  uint16_t  environment_selector;
  uint8_t  reserved_3[4];
  uint16_t  jft_size;
  void *jft;
  uint8_t  reserved_4[24];
  uint8_t  int21_retf[3];
  uint8_t  reserved_5[9];
  uint8_t  default_FCB_1[16];
  uint8_t  default_FCB_2[20];
  uint8_t  command_line_len;
  uint8_t  command_line[127];
}
__attribute__ ((packed)) tPsp;
 
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

struct process_info {
  char *args;
  uint32_t memlimit;
  char *name;
};


int  message(char *fmt,...) __attribute__((format(printf,1,2)));
void fd32_abort(void);
void restore_sp(int res);
int mem_get_region(uint32_t base, uint32_t size);
uint32_t mem_get(uint32_t amount);
int mem_free(uint32_t base, uint32_t size);
int dos_exec(char *filename, uint32_t env_segment, char * args,
	uint32_t fcb1, uint32_t fcb2, uint16_t *return_value);
void *fd32_init_jft(int JftSize);
void fd32_free_jft(void *p, int jft_size);
int  fd32_log_printf(char *fmt, ...) __attribute__ ((format(printf,1,2)));
int fd32_get_dev_info(int fd);
void mem_dump(void);
int fd32_get_date(fd32_date_t *);
int fd32_get_time(fd32_time_t *);
int  fd32_unlink(char *FileName, uint32_t Flags);
int  fd32_open             (char *FileName, uint32_t Mode, uint16_t Attr, uint16_t AliasHint, int *Result);
int  fd32_close            (int Handle);
int  fd32_read             (int Handle, void *Buffer, int Size);
int  fd32_write            (int Handle, void *Buffer, int Size);
int  fd32_lseek            (int Handle, long long int Offset, int Origin, long long int *Result);

#endif
