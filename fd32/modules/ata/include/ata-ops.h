/* ATA Driver for FD32
 * by Luca Abeni & Nils Labugt
 *
 * This is free software; see GPL.txt
 */

#ifndef __ATA_OPS_H__
#define __ATA_OPS_H__

int ata_read(struct ata_device *d, DWORD start, DWORD count, void *b);
int ata_write(struct ata_device *d, DWORD start, DWORD count, const void *b);
int pio_data_in(unsigned long max_wait, BYTE count, DWORD start, void* buffer, struct ata_device* dev, BYTE command);
int ata_set_multiple( struct ata_device* dev, BYTE sectors);
int ata_standby( struct ata_device* dev, BYTE timer);
int ata_idle( struct ata_device* dev, BYTE timer);
int ata_standby_imm( struct ata_device* dev);
int ata_check_standby( struct ata_device* dev);
int ata_set_pio_mode( struct ata_device* dev, int mode);
int ata_sleep( struct ata_device* dev);
int ata_sreset( struct ata_device* dev);
int ata_packet_pio( unsigned long max_wait, /* how long to wait, in us */
                    struct ata_device* dev,
                    WORD* packet,        /* the command packet, also used for returning error info */
                    int packet_size,    /* length of packet in bytes (minimum 12 bytes) */
                    WORD* buffer,
                    int max_count,        /* max number of bytes per transfer */
                    unsigned long* total_bytes, /* return actual number of bytes transfered */
                    unsigned long buffer_size); /* for safety, bytes */
int dev_is_busy(const struct ide_interface* iface);

#endif
