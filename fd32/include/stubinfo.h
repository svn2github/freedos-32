/* FD32 kernel
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#ifndef __STUBINFO_H__
#define __STUBINFO_H__

#include <ll/i386/hw-data.h>

struct stubinfo {
  char magic[16];
  DWORD size;
  DWORD minstack;
  DWORD memory_handle;
  DWORD initial_size;
  WORD minkeep;
  WORD ds_selector;
  WORD ds_segment;
  WORD psp_selector;
  WORD cs_selector;
  WORD env_size;
  char basename[8];
  char argv0[16];
  char dpmi_server[16];
};


/* Program Segment Prefix */
struct psp {
  /*
  WORD int20;
  WORD program_end;
  BYTE reserved_1;
  BYTE dispactcher[5];
  DWORD program_termination_code;
  DWORD break_handler_routine;
  DWORD critical_error_handler;
  BYTE reserved_2[22];
  */
  /* FD/32 stuff */
  struct psp *link;
  DWORD memlimit;
  DWORD old_stack;
  DWORD DOS_mem_buff;
  DWORD info_sel;

  void *dta;      /* Under DOS this pointer is stored in the SDA */
  void *cds_list; /* Under DOS this is a global array            */

  /* Gap */
  BYTE gap[16];

  /* The following fields must start at offset 2Ch (44) */
  WORD  environment_selector;
  BYTE  reserved_3[4];
  WORD  jft_size; /* Offset 32h (50) */
  void *jft;      /* Offset 34h (52) */
  BYTE  reserved_4[24];
  BYTE  int21_retf[3];
  BYTE  reserved_5[9];
  BYTE  default_FCB_1[16];
  BYTE  default_FCB_2[20];
  /* Offset 80h (128): this is also the start of the default DTA */
  BYTE  command_line_len;
  BYTE  command_line[127];
}
__attribute__ ((packed)) tPsp;


#endif /* __STUBINFO_H__ */

