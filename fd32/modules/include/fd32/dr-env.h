#ifndef __FD32_FD32_DRENV_H
#define __FD32_FD32_DRENV_H

#include <ll/i386/hw-data.h>
#include <ll/i386/hw-func.h>
#include <ll/i386/hw-instr.h>
#include <ll/i386/pic.h>
#include <ll/i386/stdio.h>
#include <ll/i386/error.h>
#include <ll/i386/stdlib.h>
#include <ll/i386/string.h>
#include <ll/ctype.h>
#include <ll/sys/ll/event.h>

#include <dr-env/bios.h>
#include <dr-env/mem.h>
#include <dr-env/datetime.h>

#include <kmem.h>
#include <kernel.h>
#include <logger.h>

#define itoa(value, string, radix) dcvt(value, string, radix, 10, 0)
#define fd32_outb(port, data)      outp(port, data)
#define fd32_outw(port, data)      outpw(port, data)
#define fd32_outd(port, data)      outpd(port, data)
#define fd32_inb                   inp
#define fd32_inw                   inpw
#define fd32_ind                   inpd
#define fd32_message               message
#define fd32_log_printf            fd32_log_printf
#define fd32_error                 error
#define fd32_kmem_get              (void *)mem_get
#define fd32_kmem_get_region       mem_get_region
#define fd32_kmem_free(m, size)    mem_free((DWORD)m, size)

/* Interrupt management */
typedef void Fd32IntHandler(int);
#define FD32_INT_HANDLER(name)     void name(int n)
#define fd32_cli                   cli
#define fd32_sti                   sti
#define fd32_master_eoi()
#define fd32_irq_bind(n, h)        l1_irq_bind(n, h); irq_unmask(n);

/* Timer-driven events */
typedef void Fd32EventCallback(void *params);
typedef int  Fd32Event;
#define FD32_EVENT_NULL ((Fd32Event) -1)
extern inline Fd32Event fd32_event_post(unsigned milliseconds, Fd32EventCallback *handler, void *params)
{
    struct timespec ts;
    int res;

    cli();
    ll_gettime(TIME_NEW, &ts);
    ts.tv_sec  += milliseconds / 1000;
    ts.tv_nsec += (milliseconds % 1000) * 1E6;
    res = event_post(ts, handler, params);
    sti();

    return res;
}
#define fd32_event_delete event_delete

#define WFC(c) while (c) fd32_cpu_idle()


#endif /* #ifndef __FD32_FD32_DRENV_H */

