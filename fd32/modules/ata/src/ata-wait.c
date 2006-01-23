/* ATA Driver for FD32
 * by Luca Abeni & Nils Labugt
 *
 * This is free software; see GPL.txt
 */

#include <dr-env.h>
#include "ata.h"
#include "disk.h"
#include "ata-wait.h"
#include "pit/pit.h"


static void private_timed_out(void *p)
{
    *((int *)p) = 1;
}

void ata_wait(DWORD us)
{
    volatile int tout;
    void *evt;

    tout = 0;
    evt = pit_event_register(us, private_timed_out, (void *)&tout);

    WFC(!tout);
    /* We do not delete the event, because the timeout _always_ expires! */
}

BYTE ata_cmd_irq(unsigned long max_wait, struct ide_interface *p)
{
    void *tout_event;
    volatile int tout;
    extern int irq_signaled(int i);

    tout = 0;
    tout_event = pit_event_register(max_wait, private_timed_out, (void *)&tout);
    WFC(!tout && !irq_signaled(p->irq));
    fd32_cli();    /* in case driver preempted */
    if (!tout)
    {
        pit_event_cancel(tout_event);
        fd32_sti();
        return fd32_inb(p->command_port + CMD_STATUS);
    }
    fd32_sti();
    return 0xFF;
}



int ata_poll(unsigned long max_wait, int (*test)(const struct ide_interface*), const struct ide_interface* iface)
{
    void *tout_event;
    volatile int tout;

    tout = 0;
    tout_event = pit_event_register(max_wait, private_timed_out, (void *)&tout);
    while (!tout)
    {
        //        fd32_cpu_idle();
        if(!test(iface))
            break;
    }
    fd32_cli();
    if (!tout)
    {
        pit_event_cancel(tout_event);
        fd32_sti();
        return 0;
    }
    fd32_sti();
    return -1;
}

