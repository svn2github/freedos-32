#include <unistd.h>
#include <stdlib.h>

#include "sys/syscalls.h"

#define FD32_SBRK_SIZE 0x2000

/* Track the sbrk, segment resizing, to be used in the garbage collecting when destroying the current process */
typedef struct sbrk_mem_track
{
  uint32_t addr;
  uint32_t size;
  struct sbrk_mem_track *next;
} sbrk_mem_track_t;

static sbrk_mem_track_t smthdr;
static sbrk_mem_track_t *psmtcur = &smthdr;

void _sbrk_free(void)
{
  for (psmtcur = smthdr.next; psmtcur != NULL; psmtcur = psmtcur->next)
  {
#ifdef __DEBUG__
  fd32_log_printf("SBRK Memory clear up, mem_free(%x %x)\n", psmtcur->addr, psmtcur->size);
#endif
    mem_free(psmtcur->addr, psmtcur->size);
  }

  smthdr.next = NULL;
  smthdr.addr = 0;
  smthdr.size = 0;
}

/*   sbrk for the mallocr implementation in newlib
   NOTE: This sbrk does not allocate continuous memory from system,
   but at the page size boundary ... (more detail)
 */
void *_sbrk(int incr)
{
  uint32_t prev_heap_end = 0;

  if (incr > 0) {
    if (smthdr.size > incr) {
      prev_heap_end = psmtcur->addr+FD32_SBRK_SIZE-smthdr.size;
      smthdr.size -= incr;
    } else {
      /* TODO: Free the unused memory if (smthdr.size != 0) */
    
      /* Get 0x2000 bytes of memory and slice it at FD32_PAGE_SIZE boundary */
      prev_heap_end = mem_get(FD32_SBRK_SIZE+sizeof(sbrk_mem_track_t));
      if (prev_heap_end == 0) {
        message("SBRK problem: cannot memget(%x %x)\n", fd32_get_current_pi()->memlimit, incr);
        mem_dump();
        return 0;
      } else {
        /* Keep the track of the allocated memory */
        sbrk_mem_track_t *psmt = (sbrk_mem_track_t *)(prev_heap_end+FD32_SBRK_SIZE);
        psmt->addr = prev_heap_end;
        psmt->size = FD32_SBRK_SIZE+sizeof(sbrk_mem_track_t);
        psmt->next = NULL;
        psmtcur->next = psmt;
        psmtcur = psmt;
        /* Save the free 2-page size in the track header */
        smthdr.size = FD32_SBRK_SIZE;
        /* Align at the FD32_PAGE_SIZE boundary */
        prev_heap_end += FD32_PAGE_SIZE-1;
        prev_heap_end &= ~(FD32_PAGE_SIZE-1);
        /* TODO: Free the unused memory prev_heap_end-psmtcur->addr */
        smthdr.size -= prev_heap_end-psmtcur->addr;
        smthdr.size -= incr;
      }
    
    }
  } else if (incr < 0) {
    message("How to decrease to the incr: %d!\n", incr);
  } else {
    /* incr == 0, get the current brk */
    prev_heap_end = psmtcur->addr;
  }
#ifdef __DEBUG__
  fd32_log_printf("SBRK: memget(%x %x)\n", prev_heap_end, incr);
#endif
  return (void *)prev_heap_end;
}
