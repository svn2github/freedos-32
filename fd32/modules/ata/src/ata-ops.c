/* ATA Driver for FD32
 * by Luca Abeni & Nils Labugt
 *
 * This is free software; see GPL.txt
 */

#include <dr-env.h>
#include "ata.h"
#include "disk.h"
#include "ata-ops.h"
#include "ata-wait.h"

#ifdef _DEBUG_
void reg_dump(const struct ata_device* dev)
{
    BYTE b;
    b = fd32_inb(dev->interface->command_port + CMD_ERROR);
    fd32_message("ERR:0x%x ", (BYTE)b);
    b = fd32_inb(dev->interface->command_port + CMD_STATUS);
    fd32_message("ST:0x%x ", (BYTE)b);
    b = fd32_inb(dev->interface->command_port + CMD_SECT);
    fd32_message("SCT:%u ", (BYTE)b);
    b = fd32_inb(dev->interface->command_port + CMD_CYL_L);
    fd32_message("CL:%u ", (BYTE)b);
    b = fd32_inb(dev->interface->command_port + CMD_CYL_H);
    fd32_message("CH:%u ", (BYTE)b);
    b = fd32_inb(dev->interface->command_port + CMD_DEVHEAD);
    fd32_message("DH:0x%x\n", (BYTE)b);
}
#endif

int dev_is_busy(const struct ata_device* dev)
{
    return ((fd32_inb(dev->interface->control_port) & ATA_STATUS_BSY) != 0);
}


int dev_not_ready(const struct ata_device* dev)
{
    return ((fd32_inb(dev->interface->control_port) & ATA_STATUS_DRDY) == 0);
}


int device_select( unsigned long max_wait, const struct ata_device* dev )
{
    int res;
    extern int irq_reset(int i);

    if(dev_is_busy(dev))
    {
        res = ata_poll(MAX_WAIT_1, &dev_is_busy, dev);
        if(res)
            return ATA_ETOBUSY;
    }
    /* Skip device selection if already selected */
    if(dev->interface->current_dev_bit != dev->dev_bit)
    {
        fd32_outb(dev->interface->command_port + CMD_DEVHEAD, dev->dev_bit);
        dev->interface->current_dev_bit = dev->dev_bit;
        if(dev_is_busy(dev))
        {
            res = ata_poll(MAX_WAIT_1, &dev_is_busy, dev);
            if(res)
                return ATA_ETOBUSY;
        }
    }
    if(dev_not_ready(dev))
    {
        res = ata_poll(max_wait, &dev_not_ready, dev);
        if(res)
            return ATA_ETOREADY;
    }
    /* DRQ is suppsed to be cleard at this point, but unfortunately DRQ seem to be */
    /* allways set on some old drives */
#if 0
    status = fd32_inb(dev->interface->command_port + CMD_STATUS);//------------
    if((status & ATA_STATUS_DRQ) != ATA_STATUS_DRQ)
    {
        fd32_message("Error: DRQ enabled before command written");
        while(1)
            ;
    }
#endif
    irq_reset(dev->interface->irq);
    
    return 0;
}


static void write_reg_start_sect(DWORD start, const struct ata_device* dev)
{
    if(dev->capabilities & ATA_CAPAB_LBA)
    {
        fd32_outb(dev->interface->command_port + CMD_SECT, (BYTE)start);
        fd32_outb(dev->interface->command_port + CMD_CYL_L, (BYTE)(start>>8));
        fd32_outb(dev->interface->command_port + CMD_CYL_H, (BYTE)(start>>16));
        fd32_outb(dev->interface->command_port + CMD_DEVHEAD,
                  (BYTE)( ((start>>24) & 0x0F) | dev->dev_bit | ATA_DEVHEAD_L | 0xA0));
    }
    else
    {
        fd32_outb(dev->interface->command_port + CMD_SECT, (start % dev->sectors) + 1);
        start /= dev->sectors;
        fd32_outb(dev->interface->command_port + CMD_DEVHEAD, (BYTE)( (start % dev->heads) | dev->dev_bit | 0xA0 ) );
        start /= dev->heads;
        fd32_outb(dev->interface->command_port + CMD_CYL_L, (BYTE) start );
        fd32_outb(dev->interface->command_port + CMD_CYL_H, (BYTE)(start>>8));
    }
}

int pio_data_in(unsigned long max_wait, BYTE count, DWORD start, void* buffer, struct ata_device* dev, BYTE command)
{
    int res, i, j, k;
    BYTE status;
    WORD* b = (WORD*)buffer;

    res = device_select( max_wait, dev );
    if(res)
        return res;

    if(command != ATA_CMD_IDENTIFY)
    {
        write_reg_start_sect(start, dev);
        fd32_outb(dev->interface->command_port + CMD_CNT, count);
    }
    fd32_outb(dev->interface->command_port + CMD_COMMAND, command);
    do
    {
        if(dev->polled_mode)
        {
            delay(400);
            fd32_inb(dev->interface->control_port);
            if(dev_is_busy(dev))
            {
                res = ata_poll(max_wait, &dev_is_busy, dev);
                if(res)
                    return ATA_ETOBUSY;
            }
            status = fd32_inb(dev->interface->command_port + CMD_STATUS);
        }
        else
        {
            status = ata_cmd_irq(max_wait, dev->interface);
            if(status == 0xFF)
                return ATA_ETOIRQ;
        }
        if((status & ATA_STATUS_DRQ) != ATA_STATUS_DRQ)
            return ATA_EDRQ;
        /* If sectors_per_block == 0, then block mode is not enabled */
        if(dev->sectors_per_block == 0)
        {
            for (i = 0; i < 256; i++)
            {
                *b++ = fd32_inw(dev->interface->command_port + CMD_DATA);
            }
            count--;
        }
        else
        {
            k = dev->sectors_per_block;
            for(j=0; j<k; j++)
            {
                for (i = 0; i < 256; i++)
                {
                    *b++ = fd32_inw(dev->interface->command_port + CMD_DATA);
                }
                count--;
                if(count == 0)
                    break;
            }
        }
        if(status & ATA_STATUS_ERR)
        {
            dev->status_reg = status;
            dev->error_reg = fd32_inb(dev->interface->command_port + CMD_ERROR);
            return ATA_ESTATUS;
        }
        if(count == 0)
            break;
    }
    while(count);
    return 0;
}







int ata_read(struct ata_device *d, DWORD start, DWORD count, void *b)
{
    int res;
    BYTE command;
    WORD* buff = (WORD*)b;
    unsigned long max_wait;

    if(d->flags & (DEV_FLG_ST_TIMER_ACTIVE | DEV_FLG_STANDBY))
    {
        /* TODO: check timer (and DRDY?) first */
        max_wait = 30 * 1000 * 1000;
    }
    else
        max_wait = MAX_WAIT_1;
    if(start + count > d->total_blocks)
        return -1;
    start += d->first_sector;
    if(b == NULL || d == NULL)
    {
        return -1;
    }
    if(d->sectors_per_block != 0)
        command = ATA_CMD_READ_MULTIPLE;
    else
        command = ATA_CMD_READ_SECTORS;
    while(count)
    {
        if(count >= 256)
        {
            /* When we ask for 0 sectors, then we mean 256 sectors */
            res = pio_data_in(max_wait, 0, start, (void*)buff, d, command);
            count -= 256;
            buff += 256;
        }
        else
        {
            res = pio_data_in(max_wait, (BYTE)count, start, (void*)buff, d, command);
            count = 0;
        }
        if(res)
        {
            return -1;
        }
    }
    d->flags &= ~DEV_FLG_STANDBY;
    return 0;
}


int pio_data_out(unsigned long max_wait, BYTE count, DWORD start, void* buffer, struct ata_device* dev, BYTE command)
{
    int res, i, j, k;
    BYTE status;
    WORD* b = (WORD*)buffer;

    res = device_select( max_wait, dev );
    if(res)
        return res;
    write_reg_start_sect(start, dev);
    fd32_outb(dev->interface->command_port + CMD_CNT, count);
    fd32_outb(dev->interface->command_port + CMD_COMMAND, command);
    delay(400);
    if(dev_is_busy(dev))
    {
        res = ata_poll(MAX_WAIT_1, &dev_is_busy, dev);
        if(res)
            return ATA_ETOBUSY;
    }
    status = fd32_inb(dev->interface->command_port + CMD_STATUS);
    if(status & ATA_STATUS_ERR)
        return ATA_ESTATUS;
    do
    {
        if((status & ATA_STATUS_DRQ) != ATA_STATUS_DRQ)
            return ATA_EDRQ;
        /* If sectors_per_block == 0, then block mode is not enabled */
        if(dev->sectors_per_block == 0)
        {
            for (i = 0; i < 256; i++)
            {
                fd32_outw(dev->interface->command_port + CMD_DATA, *b++ );
            }
            count--;
        }
        else
        {
            k = dev->sectors_per_block;
            for(j=0; j<k; j++)
            {
                for (i = 0; i < 256; i++)
                {
                    fd32_outw(dev->interface->command_port + CMD_DATA, *b++ );
                }
                count--;
                if(count == 0)
                    break;
            }
        }
        if(dev->polled_mode)
        {
            delay(400);
            fd32_inb(dev->interface->control_port);
            if(dev_is_busy(dev))
            {
                res = ata_poll(max_wait, &dev_is_busy, dev);
                if(res)
                    return ATA_ETOBUSY;
            }
            status = fd32_inb(dev->interface->command_port + CMD_STATUS);
        }
        else
        {
            status = ata_cmd_irq(max_wait, dev->interface);
            if(status == 0xFF)
                return ATA_ETOIRQ;
        }
        if(status & ATA_STATUS_ERR)
        {
            dev->status_reg = status;
            dev->error_reg = fd32_inb(dev->interface->command_port + CMD_ERROR);
            return ATA_ESTATUS;
        }
        if(count == 0)
            break;
    }
    while(count);
    return 0;
}


int ata_write(struct ata_device *d, DWORD start, DWORD count, const void *b)
{
    int res;
    BYTE command;
    WORD* buff = (WORD*)b;
    unsigned long max_wait;

    if(d->flags & (DEV_FLG_ST_TIMER_ACTIVE | DEV_FLG_STANDBY))
    {
        /* TODO: check timer (and DRDY?) first */
        max_wait = 30 * 1000 * 1000;
    }
    else
        max_wait = MAX_WAIT_1;
    if(start + count > d->total_blocks)
        return -1;
    start += d->first_sector;
    if(b == NULL || d == NULL)
    {
        return -1;
    }
    if(d->sectors_per_block != 0)
        command = ATA_CMD_WRITE_MULTIPLE;
    else
        command = ATA_CMD_WRITE_SECTORS;
    while(count)
    {
        if(count >= 256)
        {
            res = pio_data_out(max_wait, 0, start, (void*)buff, d, command);
            count -= 256;
            buff += 256;
        }
        else
        {
            res = pio_data_out(max_wait, (BYTE)count, start, (void*)buff, d, command);
            count = 0;
        }
        if(res)
        {
            return -1;
        }
    }
    d->flags &= ~DEV_FLG_STANDBY;
    return 0;
}


/* Here follows the non-data stuff */

int command_ackn(struct ata_device* dev)
{
    int res;
    BYTE status;

    if(dev->polled_mode)
    {
        delay(400);
        fd32_inb(dev->interface->control_port);
        if(dev_is_busy(dev))
        {
            res = ata_poll(MAX_WAIT_1, &dev_is_busy, dev);
            if(res)
                return ATA_ETOBUSY;
        }
        status = fd32_inb(dev->interface->command_port + CMD_STATUS);
    }
    else
    {
        status = ata_cmd_irq(MAX_WAIT_1, dev->interface);
        if(status == 0xFF)
            return ATA_ETOIRQ;
    }
    if(status & ATA_STATUS_ERR)
    {
        dev->status_reg = status;
        dev->error_reg = fd32_inb(dev->interface->command_port + CMD_ERROR);
        return ATA_ESTATUS;
    }
    return 0;
}

#if 0
int non_data( struct ata_device* dev, BYTE command, BYTE* res_count_reg)
{
    int res;

    res = device_select( dev );
    if(res)
        return res;

    /*    if(command != ATA_CMD_IDENTIFY)
        {
            write_reg_start_sect(start, dev);
            fd32_outb(dev->interface->command_port + CMD_CNT, count);
        }*/
    fd32_outb(dev->interface->command_port + CMD_COMMAND, command);
    res = command_ackn(dev);
    if(res)
        return res;
    if(res_count_reg != NULL)
        *res_count_reg = fd32_inb(dev->interface->command_port + CMD_CNT);
    return 0;
}
#endif

int ata_set_multiple( struct ata_device* dev, BYTE sectors)
{
    int res;

    if(sectors > dev->max_multiple_rw)
        return ATA_EINVPARAM;
    res = device_select( MAX_WAIT_1, dev );
    if(res)
        return res;
    fd32_outb(dev->interface->command_port + CMD_CNT, sectors);
    fd32_outb(dev->interface->command_port + CMD_COMMAND, ATA_CMD_SET_MULTIPLE_MODE);
    return command_ackn(dev);
}

int ata_standby( struct ata_device* dev, BYTE timer)
{
    int res;

    res = device_select( MAX_WAIT_1, dev );
    if(res)
        return res;
    fd32_outb(dev->interface->command_port + CMD_CNT, timer);
    fd32_outb(dev->interface->command_port + CMD_COMMAND, ATA_CMD_STANDBY);
    return command_ackn(dev);
}

int ata_idle( struct ata_device* dev, BYTE timer)
{
    int res;

    res = device_select( MAX_WAIT_1, dev );
    if(res)
        return res;
    fd32_outb(dev->interface->command_port + CMD_CNT, timer);
    fd32_outb(dev->interface->command_port + CMD_COMMAND, ATA_CMD_IDLE);
    return command_ackn(dev);
}

int ata_standby_imm( struct ata_device* dev)
{
    int res;

    res = device_select( MAX_WAIT_1, dev );
    if(res)
        return res;
    fd32_outb(dev->interface->command_port + CMD_COMMAND, ATA_CMD_STANDBY_IMMEDIATE);
    return command_ackn(dev);
}


int ata_check_standby( struct ata_device* dev)
{
    int res;

    res = device_select( MAX_WAIT_1, dev );
    if(res)
        return res;
    fd32_outb(dev->interface->command_port + CMD_COMMAND, ATA_CMD_CHECK_POW_MODE);
    res = command_ackn(dev);
    if(res)
        return res;
    res = fd32_inb(dev->interface->command_port + CMD_CNT);
    if(res != 255)
        return 1;
    return 0;
}

int ata_set_pio_mode( struct ata_device* dev, int mode)
{
    int res;

    res = device_select( MAX_WAIT_1, dev );
    if(res)
        return res;
    fd32_outb(dev->interface->command_port + CMD_FEATURES, 3);
    fd32_outb(dev->interface->command_port + CMD_CNT, (BYTE)(mode | 0x08));
    fd32_outb(dev->interface->command_port + CMD_COMMAND, ATA_CMD_SET_FEATURES);
    return command_ackn(dev);
}