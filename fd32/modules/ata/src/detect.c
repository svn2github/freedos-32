/* ATA Driver for FD32
 * by Luca Abeni & Nils Labugt
 *
 * This is free software; see GPL.txt
 */

#include <dr-env.h>
#include <devices.h>
#include "ata.h"
#include "disk.h"
#include "ata-detect.h"
#include "ata-ops.h"
#include "ata-wait.h"

extern int ata_global_flags;
extern int max_pio_mode;
extern BYTE standby_timer;
extern int block_mode;

static void string_parse(char *dst, WORD *src, int len)
{
    int k;

    for (k = 0; k < len; k++)
    {
        dst[2 * k] = (src[k] >> 8) & 0xFF;
        dst[2 * k + 1] = src[k] & 0xFF;
    }
    dst[2 * len] = 0;
}



int ata_detect_single(int device_no, struct ide_interface* intf, struct ata_device** d, int* detected_drives_p)
{
    WORD drvdata[256];
    int res;
    if(*d == NULL)
    {
        *d = fd32_kmem_get(sizeof(struct ata_device));
        if (*d == NULL)
        {
            fd32_message("Unable to allocate disk structure!\n");

            return 0;
        }
    }
    if(device_no == 1)
        (*d)->dev_bit = ATA_DEVHEAD_DEV;
    else
        (*d)->dev_bit = 0;
    (*d)->interface = intf;
    if(ata_global_flags & ATA_GFLAG_POLL)
        (*d)->polled_mode = TRUE;
    else
        (*d)->polled_mode = FALSE;
    (*d)->sectors_per_block = 0;
    (*d)->flags = 0;
    res = pio_data_in(100 * 1000, 1, 0, (void*)drvdata, *d, ATA_CMD_IDENTIFY);
    if(res<0)    /* TODO: check for packet device here */
        return 0;
    (*d)->first_sector = 0;
    (*detected_drives_p)++;
    (*d)->drv_number = *detected_drives_p;
    (*d)->cyls = drvdata[1];
    (*d)->heads = drvdata[3];
    (*d)->sectors = drvdata[6];
    (*d)->bytes_per_sector = 512;
    (*d)->type = FD32_BIGEN;
    (*d)->interface = intf;
    (*d)->capabilities = drvdata[49];
    (*d)->max_multiple_rw = drvdata[47] & 0xFF;
    (*d)->multiboot_id = 0;
    string_parse((*d)->model, drvdata + 27, 20);
    (*d)->model[40] = '\0';
    string_parse((*d)->serial, drvdata + 10, 10);
    (*d)->serial[20] = '\0';
    string_parse((*d)->revision, drvdata + 23, 4);
    (*d)->revision[8] = '\0';
    if((*d)->capabilities & ATA_CAPAB_LBA)
        (*d)->total_blocks = *((int*)&(drvdata[60]));
    else
        (*d)->total_blocks = (*d)->cyls * (*d)->heads * (*d)->sectors;
    /* Supported PIO modes */
    drvdata[51] &= 3;
    if(drvdata[51] > 0)
        (*d)->flags |= 1 << DEV_FLG_SUPP_PIO_BITS;
    if(drvdata[51] > 1)
        (*d)->flags |= 1 << (DEV_FLG_SUPP_PIO_BITS + 1);
    if(drvdata[53] & 2)
        (*d)->flags |= ((int)drvdata[64]) << (DEV_FLG_SUPP_PIO_BITS + 2);
    /* Device name */
    ksprintf((*d)->device_name, "hd%c", intf->basename + device_no);
    /* Enable block mode */
    if((ata_global_flags & ATA_GFLAG_BLOCK_MODE) && ((*d)->max_multiple_rw > block_mode) )
        (*d)->max_multiple_rw = block_mode;
    if((*d)->max_multiple_rw != 0)
    {
        if(ata_set_multiple( *d, (*d)->max_multiple_rw ) < 0)
            fd32_message("Error: Unable to enable block mode for %s!\n", (*d)->device_name);
        else
            (*d)->sectors_per_block = (*d)->max_multiple_rw;
    }
    /* Set PIO mode */
    if(ata_global_flags & ATA_GFLAG_PIO_MODE)
    {
        int i = 4, j = (*d)->flags;
        j |= 1 << (DEV_FLG_SUPP_PIO_BITS - 1); /* allways supports mode 0 */
        while(i >= 0)
        {
            if(i <= max_pio_mode && (j & (1 << 31)))
            {
                if(ata_set_pio_mode( *d, i ) < 0)
                    fd32_message("Error: Failed to set PIO transfer mode %i for %s!\n", i, (*d)->device_name);
                break;
            }
            i--;
            j <<= 1;
        }
    }
    /* Set standby timer */
    if(ata_global_flags & ATA_GFLAG_STANDBY_EN)
    {
        if(ata_idle( *d, standby_timer) < 0)
            fd32_message("Error: Failed to enable standby timer for %s!\n", (*d)->device_name);
        else
            (*d)->flags |= DEV_FLG_ST_TIMER_ACTIVE;
    }
    return 1;
}


int ata_detect(struct ide_interface *intf, void (*disk_add)(struct ata_device *d, char *name))
{
    int detected_drives = 0;
    struct ata_device* d = NULL;
    if(detect_poll(intf) < 0)
        return 0;
    delay(400);
    /* Read and discard ALT SATUS, then compare STATUS and ALT STATUS */
    fd32_inb(intf->control_port);
    if(fd32_inb(intf->command_port + CMD_STATUS) != fd32_inb(intf->control_port))
        return 0;
    fd32_outb(intf->command_port + CMD_DEVHEAD, 0xF0);
    if(!(ata_global_flags & ATA_GFLAG_POLL))
        ata_irq_enable(intf);
    delay(400);
    intf->current_dev_bit = ATA_DEVHEAD_DEV;
    if(ata_detect_single(0, intf, &d, &detected_drives) == 1)
    {
        disk_add(d, d->device_name);
        d = NULL;
    }
    if(ata_detect_single(1, intf, &d, &detected_drives) == 1)
    {
        disk_add(d, d->device_name);
    }
    else
        fd32_kmem_free((void*)d, sizeof(struct ata_device));
    return detected_drives;
}



