/* ATA Driver for FD32
 * by Luca Abeni & Nils Labugt
 *
 * This is free software; see GPL.txt
 */

#include <dr-env.h>
#include "ata.h"
#include "disk.h"
#include "ata-detect.h"
#include "ata-ops.h"
#include "ata-wait.h"


extern int ata_global_flags;
extern int ata_max_pio_mode;
extern BYTE ata_standby_timer;
extern int ata_block_mode;

#define _DEBUG_
#define _LOGGER_

#ifdef _LOGGER_
#define MSG fd32_log_printf
#define WAIT
#else
#define MSG fd32_message
#define WAIT ata_wait(10*1000*1000)
#endif


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

static void ata_irq_enable(struct ide_interface *p)
{
    extern void ata_irq(int n);

    fd32_irq_bind(p->irq, ata_irq);
    fd32_outb(p->control_port, 0x08);
}

int ata_detect_single(int device_no, struct ide_interface* intf, struct ata_device** d, int* detected_drives_p)
{
    WORD drvdata[256];
    int res;
    BYTE b;
    int pi_device = FALSE;

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
#ifdef _DEBUG_
    MSG("IDENTIFY DEVICE command on device %i\n", device_no);
#endif
    res = pio_data_in(200 * 1000, 1, 0, (void*)drvdata, *d, ATA_CMD_IDENTIFY, FALSE);
    if(res<0)
    {
        /* PI device? */
        /* Checking the signature */
#ifdef _DEBUG_
        MSG("IDENTIFY DEVICE failed, returned %i, st=%x, err=%x\n", res, (*d)->status_reg, (*d)->error_reg);
#endif
        b = fd32_inb((*d)->interface->command_port + CMD_CYL_L);
        if(b != 0x14)
            return 0;
        b = fd32_inb((*d)->interface->command_port + CMD_CYL_H);
        if(b != 0xEB)
            return 0;
#ifdef _DEBUG_
        MSG("Signature present, executing IDENTIFY PACET DEVICE command\n");
#endif
         res = pio_data_in(200 * 1000, 1, 0, (void*)drvdata, *d, ATA_CMD_IDENTIFY_PD, FALSE);
        if(res<0)
        {
#ifdef _DEBUG_
            MSG("IDENTIFY PACKET DEVICE failed, returned %i, st=%x, err=%x\n", res, (*d)->status_reg, (*d)->error_reg);
#endif
            fd32_message("Error: Detection of PI device failed!\n");
            return 0;
        }
        pi_device = TRUE;
    }
    (*d)->first_sector = 0;
    if(detected_drives_p != NULL)
    {
        (*detected_drives_p)++;
        (*d)->drv_number = *detected_drives_p;
        /* Device name */
        ksprintf((*d)->device_name, "hd%c", intf->basename + device_no);
    }
    (*d)->interface = intf;
    (*d)->capabilities = drvdata[49];
    (*d)->multiboot_id = 0;
    string_parse((*d)->model, drvdata + 27, 20);
    (*d)->model[40] = '\0';
    string_parse((*d)->serial, drvdata + 10, 10);
    (*d)->serial[20] = '\0';
    string_parse((*d)->revision, drvdata + 23, 4);
    (*d)->revision[8] = '\0';
    /* Supported PIO modes */
    drvdata[51] &= 3;
    if(drvdata[51] > 0)
        (*d)->flags |= 1 << DEV_FLG_SUPP_PIO_BITS;
    if(drvdata[51] > 1)
        (*d)->flags |= 1 << (DEV_FLG_SUPP_PIO_BITS + 1);
    if(drvdata[53] & 2)
        (*d)->flags |= ((int)drvdata[64]) << (DEV_FLG_SUPP_PIO_BITS + 2);
    if(pi_device == FALSE)
    {
        (*d)->cyls = drvdata[1];
        (*d)->heads = drvdata[3];
        (*d)->sectors = drvdata[6];
        (*d)->bytes_per_sector = 512;
        (*d)->max_multiple_rw = drvdata[47] & 0xFF;
        if((*d)->capabilities & ATA_CAPAB_LBA)
        {
            (*d)->total_blocks = *((int*)&(drvdata[60]));
            if( drvdata[83] & (1 << 10) )
                (*d)->flags |= DEV_FLG_48BIT_LBA;
        }
        else
            (*d)->total_blocks = (*d)->cyls * (*d)->heads * (*d)->sectors;
        /* Enable block mode */
        if((ata_global_flags & ATA_GFLAG_BLOCK_MODE) && ((*d)->max_multiple_rw > ata_block_mode) )
            (*d)->max_multiple_rw = ata_block_mode;
        if((*d)->max_multiple_rw != 0)
        {
            if(ata_set_multiple( *d, (*d)->max_multiple_rw ) < 0)
                fd32_message("Error: Unable to enable block mode for %s!\n", (*d)->device_name);
            else
                (*d)->sectors_per_block = (*d)->max_multiple_rw;
        }
        /* Set standby timer */
        if(ata_global_flags & ATA_GFLAG_STANDBY_EN)
        {
            if(ata_idle( *d, ata_standby_timer) < 0)
                fd32_message("Error: Failed to enable standby timer for %s!\n", (*d)->device_name);
            else
                (*d)->flags |= DEV_FLG_ST_TIMER_ACTIVE;
        }
    }
    else
    {
        (*d)->flags |= DEV_FLG_PI_DEV;
        (*d)->general_config = drvdata[0];
        (*d)->type = ((*d)->general_config >> 8) & 0x1F;
        if( ( (*d)->general_config & (3 << 5)) == (1 << 5))
            (*d)->flags |= DEV_FLG_IRQ_ON_PCMD;
    }
    /* Set PIO mode */
    if(ata_global_flags & ATA_GFLAG_PIO_MODE)
    {
        int i = 4, j = (*d)->flags;
        j |= 1 << (DEV_FLG_SUPP_PIO_BITS - 1); /* allways supports mode 0 */
        while(i >= 0)
        {
            if(i <= ata_max_pio_mode && (j & (1 << 31)))
            {
                if(ata_set_pio_mode( *d, i ) < 0)
                    fd32_message("Error: Failed to set PIO transfer mode %i for %s!\n", i, (*d)->device_name);
                break;
            }
            i--;
            j <<= 1;
        }
    }
    return 1;
}


int ata_detect(struct ide_interface *intf, void (*disk_add)(struct ata_device *d, char *name))
{
    int detected_drives = 0;
    struct ata_device* d = NULL;

#ifdef _DEBUG_
    MSG("Staring detection: cmd=%x, ctrl=%x, irq=%u\n", (unsigned)intf->command_port, (unsigned)intf->control_port, intf->irq);
#endif
    intf->dev0 = NULL;
    intf->dev1 = NULL;
    if(ata_poll(MAX_WAIT_DETECT, &dev_is_busy, intf) < 0)
    {
#ifdef _DEBUG_
        MSG("Timeout on BSY bit, aborting detection\n");
#endif
        return 0;
    }
    delay(400);
    /* Read and discard ALT SATUS, then compare STATUS and ALT STATUS */
    fd32_inb(intf->control_port);
    if((fd32_inb(intf->command_port + CMD_STATUS) & ~0x02) != (fd32_inb(intf->control_port) & ~0x02))
    {
#ifdef _DEBUG_
        MSG("STATUS snd ALT STATUS different, aborting detection\n");
#endif
        return 0;
    }
    fd32_outb(intf->command_port + CMD_DEVHEAD, 0xF0);
    if(!(ata_global_flags & ATA_GFLAG_POLL))
        ata_irq_enable(intf);
    delay(400);
    intf->current_dev_bit = ATA_DEVHEAD_DEV;
    if(ata_detect_single(0, intf, &d, &detected_drives) == 1)
    {
        disk_add(d, d->device_name);
        intf->dev0 = d;
        d = NULL;
#ifdef _DEBUG_
        MSG("Master device detected\n");
        WAIT;
#endif
    }
    if(ata_detect_single(1, intf, &d, &detected_drives) == 1)
    {
        disk_add(d, d->device_name);
        intf->dev1 = d;
#ifdef _DEBUG_
        MSG("Slave device detected\n");
        WAIT;
#endif
    }
    else
        fd32_kmem_free((void*)d, sizeof(struct ata_device));
    return detected_drives;
}



