#ifndef __FD32_DJGPP_DRENV_H
#define __FD32_DJGPP_DRENV_H

#include <hw-data.h>
#include <dr-env/bios.h>
#include <dr-env/mem.h>
#include <dr-env/datetime.h>
#include <dr-env/events.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/segments.h>
#include <pc.h>

#define fd32_outb(port, data)      outportb(port, data)
#define fd32_outw(port, data)      outportw(port, data)
#define fd32_outd(port, data)      outportl(port, data)
#define fd32_inb                   inportb
#define fd32_inw                   inportw
#define fd32_ind                   inportl

#define fd32_message               printf
#define ksprintf                   sprintf
/* We can probably have something better here... */
#define fd32_log_printf            printf
#define fd32_error                 printf

#define fd32_kmem_get              malloc
#define fd32_kmem_get_region(a, b) unimplemented()
#define fd32_kmem_free(m, size)    free(m)

typedef void Fd32IntHandler(void);
#define FD32_INT_HANDLER(name)     void name(void)
#define fd32_cli()                 __asm__ __volatile__("cli")
#define fd32_sti()                 __asm__ __volatile__("sti")
#define fd32_master_eoi()          outportb(0x20, 0x20)
extern inline void fd32_irq_bind(unsigned irq, Fd32IntHandler *handler)
{
    unsigned           irq_state;
    _go32_dpmi_seginfo si;

    if (irq < 8) irq += _go32_info_block.master_interrupt_controller_base;
            else irq += _go32_info_block.slave_interrupt_controller_base - 8;
    irq_state = __dpmi_get_and_disable_virtual_interrupt_state();
    si.pm_offset   = (unsigned long) handler;
    si.pm_selector = _go32_my_cs();
    _go32_dpmi_allocate_iret_wrapper(&si);
    _go32_dpmi_set_protected_mode_interrupt_vector(irq, &si);
    __dpmi_get_and_set_virtual_interrupt_state(irq_state);
}


/* Why do we need this? --Salvo */
WORD app_ds;

extern inline void drv_init(void)
{
  app_ds = _my_ds();
}


#endif /* #ifndef __FD32_DJGPP_DRENV_H */

