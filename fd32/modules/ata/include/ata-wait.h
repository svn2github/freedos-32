/* ATA Driver for FD32
 * by Luca Abeni & Nils Labugt
 *
 * This is free software; see GPL.txt
 */

#ifndef __ATA_WAIT_H__
#define __ATA_WAIT_H__

void ata_wait(DWORD us);
void delay(unsigned ns);
BYTE ata_cmd_irq(unsigned long max_wait, struct ide_interface *p);
int ata_poll(DWORD max_wait, int (*test)(const struct ata_device*), const struct ata_device* d);
int detect_poll(unsigned long max_wait, struct ide_interface* intf);

#endif
