/* ATA Driver for FD32
 * by Luca Abeni & Nils Labugt
 *
 * This is free software; see GPL.txt
 */

#ifndef __PART_SCAN_H__
#define __PART_SCAN_H__

int ata_scanpart(struct ata_device *d, const char *name);


#endif
