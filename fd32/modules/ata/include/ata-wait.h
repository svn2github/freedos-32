/* ATA Driver for FD32
 * by Luca Abeni & Nils Labugt
 *
 * This is free software; see GPL.txt
 */

#ifndef __ATA_WAIT_H__
#define __ATA_WAIT_H__

void ata_wait(DWORD us);
BYTE ata_cmd_irq(unsigned long max_wait, struct ide_interface *p);
int ata_poll(unsigned long max_wait, int (*test)(const struct ide_interface*), const struct ide_interface* iface);

#endif
