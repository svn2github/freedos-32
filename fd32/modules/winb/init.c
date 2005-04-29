#include <unistd.h>
#include <sys/syslimits.h>
#include "winb.h"


#define FD32_PAGE_SIZE 0x1000
#define FD32_SBRK_SIZE 0x2000

/* The truncate function changes the size of filename to length. */
int ftruncate(int fd, off_t length)
{
  return -1;
}

/* Track the sbrk, segment resizing, to be used in the garbage collecting when destroying the current process */
typedef struct sbrk_mem_track
{
  uint32_t addr;
  uint32_t size;
  struct sbrk_mem_track *next;
} sbrk_mem_track_t;

static sbrk_mem_track_t smthdr;
static sbrk_mem_track_t *psmtcur = &smthdr;

static void winb_mem_clear_up(void)
{
  for (psmtcur = smthdr.next; psmtcur != NULL; psmtcur = psmtcur->next)
  {
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] Memory clear up, mem_free(%x %x)\n", psmtcur->addr, psmtcur->size);
#endif
    mem_free(psmtcur->addr, psmtcur->size);
  }
}

/* sbrk for the mallocr implementation in newlib
 * (temporary implemented sbrk 'til the libfd32 is mature)
 */
void *sbrk(int incr)
{
  extern struct psp *current_psp;
  uint32_t prev_heap_end = 0;

  /* TODO: Get rid of assiging the clear_up function everytime */
  current_psp->mem_clear_up = winb_mem_clear_up;

  if (incr > 0) {
    if (smthdr.size > incr) {
      prev_heap_end = psmtcur->addr+FD32_SBRK_SIZE-smthdr.size;
      smthdr.size -= incr;
    } else {
      /* TODO: Free the unused memory if (smthdr.size != 0) */
    
      /* Get 0x2000 bytes of memory and slice it at FD32_PAGE_SIZE boundary */
      prev_heap_end = mem_get(FD32_SBRK_SIZE+sizeof(sbrk_mem_track_t));
      if (prev_heap_end == 0) {
        message("[WINB] SBRK problem: cannot memget(%x %x)\n", current_psp->memlimit, incr);
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
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] SBRK: memget(%x %x)\n", prev_heap_end, incr);
#endif
  return (void *)prev_heap_end;
}

long sysconf(int parameter)
{
  long res;
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] SYSCONF parameter: %x\n", parameter);
#endif
  switch (parameter)
  {
    case _SC_ARG_MAX:
      res = ARG_MAX;
      break;
    case _SC_PAGESIZE:
      res = FD32_PAGE_SIZE;
      break;
    default:
      res = -1;
      break;
  }
  
  return res;
}

static int isspecial(double d)
{
  /* IEEE standard number rapresentation */
  register struct IEEEdp {
    unsigned long manl:32;
    unsigned long manh:20;
    unsigned long exp:11;
    unsigned long sign:1;
  } *ip = (struct IEEEdp *) &d;

  if (ip->exp != 0x7ff)
    return (0);
  if (ip->manh || ip->manl) {
    return (1);
  } else if (ip->sign) {
    return (2);
  } else {
    return (3);
  }
}

int isinf(double x)
{
  return (isspecial(x) > 1);
}

int isnan(double x)
{
  return (isspecial(x) == 1);
}

void winbase_init(void)
{
  message("Initing WINB module ...\n");
  message("Current time: %d\n", time(NULL));
  install_kernel32();
  install_user32();
  install_advapi32();
  install_oleaut32();
  install_msvcrt();
}
