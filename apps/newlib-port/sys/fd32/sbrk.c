#include <unistd.h>
#include <stdlib.h>

#include "sys/syscalls.h"

void *_sbrk(int incr)
{
  struct process_info *ppi = fd32_get_current_pi();
  char *prev_heap_end = NULL;
  unsigned int memlimit = ppi->memlimit;

  if (memlimit != 0) {
    prev_heap_end = (char *)memlimit;
    if (incr > 0) {
      if (mem_get_region(memlimit, incr) == -1) {
        message("Ermmm... SBRK problem: cannot memget(%x %x)\n", memlimit, incr);
        mem_dump();
        return 0;
      }
    } else if (incr < 0) {
      mem_free(memlimit + incr, -incr);
    }
    /* Change the memlimit */
    ppi->memlimit += incr;
  } else {
    message("sbrk error: Memory Limit == 0!\n");
    fd32_abort();
  }
  
  return prev_heap_end;
}
