#ifndef __FD32_DRENV_H
#define __FD32_DRENV_H

#include <hw-data.h>
#include <dr-env/bios.h>
#include <dr-env/mem.h>
#include <dr-env/datetime.h>

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

#define fd32_cli()                 __asm__ __volatile__("cli")
#define fd32_sti()                 __asm__ __volatile__("sti")


/* Why do we need this? --Salvo */
WORD app_ds;

extern inline void drv_init(void)
{
  app_ds = _my_ds();
}


/* This works only for the master PIC --Salvo */
extern inline int fd32_set_handler(int irq, void *handler)
{
  __dpmi_paddr handler_address;
  __dpmi_version_ret version;

  handler_address.selector = _my_cs();
  handler_address.offset32 = (DWORD)handler;
  __dpmi_get_version(&version);

  return __dpmi_set_protected_mode_interrupt_vector(version.master_pic + irq, &handler_address);
}


#endif /* #ifndef __FD32_DRENV_H */

