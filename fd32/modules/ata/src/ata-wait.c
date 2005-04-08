/* ATA Driver for FD32
 * by Luca Abeni & Nils Labugt
 *
 * This is free software; see GPL.txt
 */

#include <dr-env.h>
#include "ata.h"
#include "disk.h"
#include "ata-wait.h"



static void private_timed_out(void *p)
{
    *((int *)p) = 1;
}

void ata_wait(DWORD us)
{
    volatile int tout;
    int evt;

    tout = 0;
    evt = fd32_event_post(us, private_timed_out, (void *)&tout);

    WFC(!tout);
    /* We do not delete the event, because the timeout _always_ expires! */
}

/* TODO: This needs to be rewritten. Move into kernel? */
void delay(unsigned ns)
{
    TIME start, current;
    TIME us = ns / 1000;
    us +=4;
    current = start = ll_gettime(TIME_NEW, NULL);
    while(current - start < us)
        current = ll_gettime(TIME_NEW, NULL);
}

BYTE ata_cmd_irq(unsigned long max_wait, struct ide_interface *p)
{
    int tout_event;
    volatile int tout;
    extern int irq_signaled(int i);

    tout = 0;
    tout_event = fd32_event_post(max_wait, private_timed_out, (void *)&tout);
    WFC(!tout && !irq_signaled(p->irq));
    fd32_cli();    /* in case driver preempted */
    if (!tout)
    {
        fd32_event_delete(tout_event);
        fd32_sti();
        return fd32_inb(p->command_port + CMD_STATUS);
    }
    fd32_sti();
    return 0xFF;
}



int ata_poll(unsigned long max_wait, int (*test)(const struct ata_device*), const struct ata_device* d)
{
    int tout_event;
    volatile int tout;

    tout = 0;
    tout_event = fd32_event_post(max_wait, private_timed_out, (void *)&tout);
    while (!tout)
    {
        //        fd32_cpu_idle();
        if(!test(d))
            break;
    }
    fd32_cli();
    if (!tout)
    {
        fd32_event_delete(tout_event);
        fd32_sti();
        return 0;
    }
    fd32_sti();
    return -1;
}

int detect_poll(unsigned long max_wait, struct ide_interface* intf)
{
    int tout_event;
    volatile int tout;

    tout = 0;
    tout_event = fd32_event_post(max_wait, private_timed_out, (void *)&tout);
    while (!tout)
    {
        if(!(fd32_inb(intf->command_port + CMD_STATUS) & ATA_STATUS_BSY))
            break;
    }
    fd32_cli();
    if (!tout)
    {
        fd32_event_delete(tout_event);
        fd32_sti();
        return 0;
    }
    fd32_sti();
    return -1;
}
