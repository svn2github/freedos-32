/* ATA Driver for FD32
 * by Luca Abeni & Nils Labugt
 *
 * This is free software; see GPL.txt
 */

#include <dr-env.h>

static volatile int signaled[8];

void ata_irq(int n)
{
    signaled[n - PIC2_BASE] = 1;

//     fd32_message("Irq %d   %x %x ", n - PIC2_BASE, (unsigned)&signaled[n - PIC2_BASE], (unsigned)&signaled[0]); 
//    fd32_slave_eoi();
}

int irq_signaled(int i)
{
    if (signaled[i - 8])
    {
        signaled[i - 8] = 0;
//        message("Yes! %i %x %x ",i - 8, (unsigned)&signaled[i - 8], (unsigned)&signaled[0]);
        return 1;
    }
//    message("No %i %x %x ",i - 8, (unsigned)&signaled[i - 8], (unsigned)&signaled[0]);
    return 0;
}

void irq_reset(int i)
{
    signaled[i - 8] = 0;
//    message("Reset %i %x %x ",i - 8, (unsigned)&signaled[i - 8], (unsigned)&signaled[0]);

}
