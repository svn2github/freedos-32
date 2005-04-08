/* ATA Driver for FD32
 * by Luca Abeni & Nils Labugt
 *
 * This is free software; see GPL.txt
 */

#ifndef __ATA_DETECT_H__
#define __ATA_DETECT_H__

int ata_detect(struct ide_interface *i, void (*disk_add)(struct ata_device *d, char *name));
int ata_detect_single(int device_no, struct ide_interface* intf, struct ata_device** d, int* detected_drives_p);

#endif
