#include <unistd.h>
#include <stdlib.h>

#include <syscalls.h>

void *_sbrk(int incr)
{
  char *prev_heap_end = NULL;
  int res;
  extern DWORD mem_limit;

  if (mem_limit != 0) {
    prev_heap_end = (char *)mem_limit;
    if (incr > 0) {
      res = mem_get_region(mem_limit, incr);
      if (res == -1) {
        message("Ermmm... SBRK problem: cannot memget(%lx %x)\n",
			mem_limit, incr);
        mem_dump();
        return 0;
      }
    } else if (incr < 0) {
      mem_free(mem_limit + incr, -incr);
    }
    mem_limit += incr;
  } else {
    message("sbrk error: Memory Limit == 0!\n");
    fd32_abort();
  }
  
  return prev_heap_end;
}
