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
  uint32_t address;
};

/* Program Segment Prefix
 * from fd32/include/stubinfo.h
 */
struct psp {
  /* FD/32 stuff */
  struct psp *link;
  uint32_t memlimit;
  uint32_t old_stack;
  uint32_t DOS_mem_buff;
  uint32_t info_sel;

  void *dta;      /* Under DOS this pointer is stored in the SDA */
  void *cds_list; /* Under DOS this is a global array            */

  /* Gap */
  uint8_t gap[16];

  /* The following fields must start at offset 2Ch (44) */
  uint16_t environment_selector;
  uint8_t  reserved_3[4];
  uint16_t jft_size; /* Offset 32h (50) */
  void *jft;      /* Offset 34h (52) */
  uint8_t  reserved[66]; /* Just used as padding */
  /* Offset 80h (128): this is also the start of the default DTA */
  uint8_t  command_line_len;
  uint8_t  command_line[127];
} __attribute__ ((packed)) ;

/*
 * from oslib/ll/i386/error.h
 */
int message(char *fmt, ...) __attribute__((format(printf, 1, 2)));
#define error(msg) message("Error! File:%s Line:%d %s", __FILE__, __LINE__, msg)

/* Sends formatted output from the arguments (...) to the log buffer
 * from fd32/include/logger.h
 */
int fd32_log_printf(char *fmt, ...) __attribute__ ((format(printf, 1, 2)));

/*
 * from fd32/include/kernel.h
 */
int add_dll_table(char *dll_name, uint32_t handle, uint32_t symbol_num, struct symbol *symbol_array);

/*
 * from fd32/include/kmem.h
 */
uint32_t mem_get(uint32_t amount);
uint32_t mem_get_region(uint32_t base, uint32_t size);
int mem_free(uint32_t base, uint32_t size);


void install_kernel32(void);
void install_msvcrt(void);

#endif
