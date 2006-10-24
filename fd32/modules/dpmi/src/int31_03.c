/* DPMI Driver for FD32: handler for INT 0x31, Service 0x03
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include<ll/i386/hw-data.h>
#include<ll/i386/string.h>
#include<ll/i386/error.h>

#include <kmem.h>
#include <logger.h>

#include "dpmi.h"
#include "rmint.h"
#include "ldtmanag.h"
#include "int31_03.h"

void int31_0300(union regs *r)
{
  DWORD base;
  int res;

#ifdef __DEBUG__
  DWORD limit;
  BYTE f = (BYTE)(r->d.ebx >> 8);
  fd32_log_printf("[DPMI]: Simulate RM interrupt (INT 0x%x)\n",
	  r->h.bl);
  fd32_log_printf("    Real Mode Call Structure @ 0x%x:0x%lx\n",
	  r->x.es, r->d.edi);
#endif

  fd32_get_segment_base_address(r->x.es, &base);

#ifdef __DEBUG__
  fd32_get_segment_limit(r->x.es, &limit);
  fd32_log_printf("    ES: base=0x%lx, limit=0x%lx\n", base, limit);
#endif

  /* NOTE: The real-mode stack if SS:SP != 0:0 */
  res = fd32_real_mode_int(r->h.bl, base + r->d.edi);
  if (res != 0) {
      r->x.ax = res;
      SET_CARRY;
  } else {
      CLEAR_CARRY;
  }
}

static tRMCBTrack rmcbtrack_top = {NULL, NULL};

tRMCBTrack *fd32_get_rm_callback(DWORD dosmem_addr)
{
  tRMCBTrack *p;

  for (p = rmcbtrack_top.next; p != NULL; p = p->next)
    if ((DWORD)p->dosmem_addr == dosmem_addr)
      return p;

  return NULL;
}

extern void _fd32_rm_callback(); /* 16bit DPMI rmcallback */
extern void _fd32_rm_callback_end();

void int31_0303(union regs *r)
{
  DWORD addr;
  int res;
  tRMCBTrack *t = (tRMCBTrack *)mem_get(sizeof(tRMCBTrack));
  /* TODO: This must be completely implemented */
  const DWORD size = (DWORD)_fd32_rm_callback_end - (DWORD)_fd32_rm_callback;
  BYTE *p = (BYTE *)dosmem_get(size);
  memcpy(p, _fd32_rm_callback, size);
  t->dosmem_addr = p;
  t->dosmem_size = size;

#ifdef __DEBUG__
  fd32_log_printf("[DPMI]: Allocate Real Mode CallBack Address\n");
  fd32_log_printf("    Real Mode Call Structure @ 0x%x:0x%lx\n",
                  (WORD)r->d.ees, r->d.edi);
  fd32_log_printf("    Procedure to Call @ 0x%x:0x%lx\n", (WORD)r->d.eds, r->d.esi);
#endif

  if ( (res = fd32_get_segment_base_address(r->x.ds, &addr)) >= 0) {
    t->cs = r->x.ds;
    t->eip = r->d.esi;

    if ( (res = fd32_get_segment_base_address(r->x.es, &addr)) >= 0) {

      t->es = r->x.es;
      t->edi = r->d.edi;
      /* Keep the track of ... */
      t->next = rmcbtrack_top.next;
      rmcbtrack_top.next = t;

      r->d.ecx = (DWORD)p>>4;
      r->d.edx = (DWORD)p&0x0F;
      r->d.eax = r->d.eax & 0xFFFF0000;

      CLEAR_CARRY;
    }
  }

  dpmi_return(res, r);
}

void int31_0304(union regs *r)
{
  tRMCBTrack *p;
  tRMCBTrack *q;
  /* TODO: Debug a DOS/32A program, why it doesn't call this service when terminating */
#ifdef __DEBUG__
  fd32_log_printf("[DPMI]: Free Real Mode CallBack Address\n");
  fd32_log_printf("    Real Mode CallBack Address: 0x%x:0x%lx\n",
                  r->d.ecx, r->d.edx);
#endif

  for (q = &rmcbtrack_top, p = rmcbtrack_top.next; p != NULL; q = p, p = p->next)
    if ((DWORD)p->dosmem_addr == (r->x.cx<<4)+r->x.dx)
    {
      q->next = p->next;
      dosmem_free ((DWORD)p->dosmem_addr, p->dosmem_size);
      mem_free ((DWORD)p, sizeof(tRMCBTrack));
      CLEAR_CARRY;
      break;
    }

  /* TODO: This must be completely implemented too*/
  if (p == NULL) {
    r->x.ax = DPMI_INVALID_CALLBACK_ADDRESS;
    SET_CARRY;
  }
}
