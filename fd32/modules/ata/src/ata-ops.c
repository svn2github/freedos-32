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
#include "ata-detect.h"

extern int ata_global_flags;
extern int max_pio_mode;

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

int dev_is_busy(const struct ide_interface* iface)
{
    return ((fd32_inb(iface->control_port) & ATA_STATUS_BSY) != 0);
}


int dev_not_ready(const struct ide_interface* iface)
{
    return ((fd32_inb(iface->control_port) & ATA_STATUS_DRDY) == 0);
}

int bsy_drq_is_set(const struct ide_interface* iface)
{
    return (fd32_inb(iface->control_port) & (ATA_STATUS_BSY | ATA_STATUS_DRQ));
}



int device_select( unsigned long max_wait, const struct ata_device* dev)
{
    int res;
    BYTE status;
    extern int irq_reset(int i);

    if(dev->flags & DEV_FLG_SLEEP)
    {
#if 1
        return ATA_ESLEEP;
#else

        if(ata_reset( dev ) < 0)
            return ATA_ESLEEP;
#endif

    }
    if(bsy_drq_is_set(dev->interface))
    {
        res = ata_poll(MAX_WAIT_1, &bsy_drq_is_set, dev->interface);
        if(res)
        {
            status = fd32_inb(dev->interface->command_port + CMD_STATUS);
            if(status & ATA_STATUS_DRQ)
            {
                return ATA_DRQ_SET;
            }
            return ATA_ETOBUSY;
        }
    }
    /* Skip device selection if already selected */
    if(dev->interface->current_dev_bit != dev->dev_bit)
    {
        fd32_outb(dev->interface->command_port + CMD_DEVHEAD, dev->dev_bit);
        dev->interface->current_dev_bit = dev->dev_bit;
        if(bsy_drq_is_set(dev->interface))
        {
            res = ata_poll(MAX_WAIT_1, &bsy_drq_is_set, dev->interface);
            if(res)
            {
                status = fd32_inb(dev->interface->command_port + CMD_STATUS);
                if(status & ATA_STATUS_DRQ)
                {
                    return ATA_DRQ_SET;
                }
                return ATA_ETOBUSY;
            }
        }
    }
    irq_reset(dev->interface->irq);

    return 0;
}


static void write_reg_start_count(DWORD start, WORD count, int use_48bit, const struct ata_device* dev)
{
    if(dev->capabilities & ATA_CAPAB_LBA)
    {
        if(use_48bit)
        {
            fd32_outb(dev->interface->command_port + CMD_SECT, (BYTE)(start>>24));
            fd32_outb(dev->interface->command_port + CMD_CYL_L, (BYTE)((QWORD)start>>32));
            fd32_outb(dev->interface->command_port + CMD_CYL_H, (BYTE)((QWORD)start>>40));
            fd32_outb(dev->interface->command_port + CMD_CNT, (BYTE)(count>>8));
        }
        else
            fd32_outb(dev->interface->command_port + CMD_DEVHEAD,
                      (BYTE)( ((start>>24) & 0x0F) | dev->dev_bit | ATA_DEVHEAD_L | 0xA0));

        fd32_outb(dev->interface->command_port + CMD_SECT, (BYTE)start);
        fd32_outb(dev->interface->command_port + CMD_CYL_L, (BYTE)(start>>8));
        fd32_outb(dev->interface->command_port + CMD_CYL_H, (BYTE)(start>>16));
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
    fd32_outb(dev->interface->command_port + CMD_CNT, (BYTE)count);
}

int pio_data_in(unsigned long max_wait, BYTE count, DWORD start,
                void* buffer, struct ata_device* dev, BYTE command, int use_48bit)
{
    int res, i, j, k;
    BYTE status;
    WORD* b = (WORD*)buffer;

    res = device_select( max_wait, dev);
    if(res)
        return res;

    if(command != ATA_CMD_IDENTIFY)
    {
        write_reg_start_count(start, count, use_48bit, dev);
    }
    fd32_outb(dev->interface->command_port + CMD_COMMAND, command);
    do
    {
        if(dev->polled_mode)
        {
            delay(400);
            fd32_inb(dev->interface->control_port);
            if(dev_is_busy(dev->interface))
            {
                res = ata_poll(max_wait, &dev_is_busy, dev->interface);
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
            return ATA_EDRQ_CLEAR;
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
    int use_48bit = FALSE;
    int max_per_irq;

    if(b == NULL || d == NULL)
    {
        return -1;
    }
    if(d->flags & (DEV_FLG_ST_TIMER_ACTIVE | DEV_FLG_STANDBY))
    {
        /* TODO: check timer (and DRDY?) first */
        max_wait = 30 * 1000 * 1000;
    }
    else
        max_wait = MAX_WAIT_1;
    if(start + count > d->total_blocks)
        return ATA_ERANGE;
    start += d->first_sector;
    if((start + count - 1) & 0xF0000000)
    {
        if(d->flags & DEV_FLG_48BIT_LBA)
            use_48bit = TRUE;
        else
            return ATA_ERANGE;
        if(d->sectors_per_block != 0)
            command = ATA_CMD_READ_MULTIPLE_EXT;
        else
            command = ATA_CMD_READ_SECTORS_EXT;
        max_per_irq = 256 * 256;

    }
    else
    {
        if(d->sectors_per_block != 0)
            command = ATA_CMD_READ_MULTIPLE;
        else
            command = ATA_CMD_READ_SECTORS;
        max_per_irq = 256;
    }
    while(count)
    {
        if(count >= max_per_irq)
        {
            /* When we ask for 0 sectors, then we mean 256 sectors */
            res = pio_data_in(max_wait, 0, start, (void*)buff, d, command, use_48bit);
            count -= max_per_irq;
            buff += max_per_irq;
        }
        else
        {
            res = pio_data_in(max_wait, (BYTE)count, start, (void*)buff, d, command, use_48bit);
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


int pio_data_out(unsigned long max_wait, WORD count, DWORD start,
                 void* buffer, struct ata_device* dev, BYTE command, int use_48bit)
{
    int res, i, j, k;
    BYTE status;
    WORD* b = (WORD*)buffer;

    res = device_select( max_wait, dev );
    if(res)
        return res;
    write_reg_start_count(start, count, use_48bit, dev);
    fd32_outb(dev->interface->command_port + CMD_CNT, count);
    fd32_outb(dev->interface->command_port + CMD_COMMAND, command);
    delay(400);
    if(dev_is_busy(dev->interface))
    {
        res = ata_poll(MAX_WAIT_1, &dev_is_busy, dev->interface);
        if(res)
            return ATA_ETOBUSY;
    }
    status = fd32_inb(dev->interface->command_port + CMD_STATUS);
    if(status & ATA_STATUS_ERR)
        return ATA_ESTATUS;
    do
    {
        if((status & ATA_STATUS_DRQ) != ATA_STATUS_DRQ)
            return ATA_EDRQ_CLEAR;
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
            if(dev_is_busy(dev->interface))
            {
                res = ata_poll(max_wait, &dev_is_busy, dev->interface);
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
    int use_48bit = FALSE;
    int max_per_irq;

    if(b == NULL || d == NULL)
    {
        return -1;
    }
    if(d->flags & (DEV_FLG_ST_TIMER_ACTIVE | DEV_FLG_STANDBY))
    {
        /* TODO: check timer (and DRDY?) first */
        max_wait = 30 * 1000 * 1000;
    }
    else
        max_wait = MAX_WAIT_1;
    if(start + count > d->total_blocks)
        return ATA_ERANGE;
    start += d->first_sector;
    if((start + count - 1) & 0xF0000000)
    {
        if(d->flags & DEV_FLG_48BIT_LBA)
            use_48bit = TRUE;
        else
            return ATA_ERANGE;
        if(d->sectors_per_block != 0)
            command = ATA_CMD_WRITE_MULTIPLE_EXT;
        else
            command = ATA_CMD_WRITE_SECTORS_EXT;
        max_per_irq = 256 * 256;

    }
    else
    {
        if(d->sectors_per_block != 0)
            command = ATA_CMD_WRITE_MULTIPLE;
        else
            command = ATA_CMD_WRITE_SECTORS;
        max_per_irq = 256;
    }
    while(count)
    {
        if(count >= max_per_irq)
        {
            res = pio_data_out(max_wait, 0, start, (void*)buff, d, command, use_48bit);
            count -= max_per_irq;
            buff += max_per_irq;
        }
        else
        {
            res = pio_data_out(max_wait, (BYTE)count, start, (void*)buff, d, command, use_48bit);
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
        if(dev_is_busy(dev->interface))
        {
            res = ata_poll(MAX_WAIT_1, &dev_is_busy, dev->interface);
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
    res = device_select( MAX_WAIT_1, dev);
    if(res)
        return res;
    fd32_outb(dev->interface->command_port + CMD_CNT, sectors);
    fd32_outb(dev->interface->command_port + CMD_COMMAND, ATA_CMD_SET_MULTIPLE_MODE);
    return command_ackn(dev);
}

int ata_standby( struct ata_device* dev, BYTE timer)
{
    int res;

    res = device_select( MAX_WAIT_1, dev);
    if(res)
        return res;
    fd32_outb(dev->interface->command_port + CMD_CNT, timer);
    fd32_outb(dev->interface->command_port + CMD_COMMAND, ATA_CMD_STANDBY);
    return command_ackn(dev);
}

int ata_idle( struct ata_device* dev, BYTE timer)
{
    int res;

    res = device_select( MAX_WAIT_1, dev);
    if(res)
        return res;
    fd32_outb(dev->interface->command_port + CMD_CNT, timer);
    fd32_outb(dev->interface->command_port + CMD_COMMAND, ATA_CMD_IDLE);
    return command_ackn(dev);
}

int ata_standby_imm( struct ata_device* dev)
{
    int res;

    res = device_select( MAX_WAIT_1, dev);
    if(res)
        return res;
    fd32_outb(dev->interface->command_port + CMD_COMMAND, ATA_CMD_STANDBY_IMMEDIATE);
    return command_ackn(dev);
}

int ata_sleep( struct ata_device* dev)
{
    int res;

    res = device_select( MAX_WAIT_1, dev);
    if(res)
        return res;
    fd32_outb(dev->interface->command_port + CMD_COMMAND, ATA_CMD_SLEEP);
    return command_ackn(dev);
}

int ata_sreset( struct ata_device* dev)
{
    BYTE b;
    int res;

    b = fd32_inb(dev->interface->control_port);
    fd32_outb(dev->interface->control_port, b | ATA_SRST);
    delay(8000);
    fd32_outb(dev->interface->control_port, b & ~ATA_SRST);
    ata_wait(2500);
    /* What about the DEV bit? */
    res = ata_poll(31*1000*1000, &dev_is_busy, dev->interface);
    if(res<0)
        return res;
    /* FIXME: This is a hack! What PIO modes does the host support? */
    /* On what type of bus does the controller reside? */
    if(!(ata_global_flags & ATA_GFLAG_PIO_MODE))
    {
        ata_global_flags |= ATA_GFLAG_PIO_MODE;
        max_pio_mode = 2;
    }
    if(dev->interface->dev0 != 0)
        ata_detect_single(0, dev->interface, &(dev->interface->dev0), NULL);
    if(dev->interface->dev1 != 0)
        ata_detect_single(1, dev->interface, &(dev->interface->dev1), NULL);
    return 0;
}

int ata_dev_reset( struct ata_device* dev)
{
    int res;
    BYTE status;

    if(dev->interface->current_dev_bit != dev->dev_bit)
    {
        /* Is this correct? Should we wait for the other device? */
        if(bsy_drq_is_set(dev->interface))
        {
            res = ata_poll(MAX_WAIT_1, &bsy_drq_is_set, dev->interface);
            if(res)
            {
                status = fd32_inb(dev->interface->command_port + CMD_STATUS);
                if(status & ATA_STATUS_DRQ)
                {
                    return ATA_DRQ_SET;
                }
                return ATA_ETOBUSY;
            }
        }
        fd32_outb(dev->interface->command_port + CMD_DEVHEAD, dev->dev_bit);
        dev->interface->current_dev_bit = dev->dev_bit;
    }
    fd32_outb(dev->interface->command_port + CMD_COMMAND, ATA_CMD_DEVICE_RESET);
    /* The device is not supposed to revert to defaults after this command, so we just return */
    /* without reinitializing */
    /* FIXME: proper timeout */
    /* TODO: check (return?) diagnostics results */
    return command_ackn(dev);
}

int ata_check_standby( struct ata_device* dev)
{
    int res;

    res = device_select( MAX_WAIT_1, dev);
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

    res = device_select( MAX_WAIT_1, dev);
    if(res)
        return res;
    fd32_outb(dev->interface->command_port + CMD_FEATURES, 3);
    fd32_outb(dev->interface->command_port + CMD_CNT, (BYTE)(mode | 0x08));
    fd32_outb(dev->interface->command_port + CMD_COMMAND, ATA_CMD_SET_FEATURES);
    return command_ackn(dev);
}


/* TODO: implement overlapped commands */
int ata_packet_pio( unsigned long max_wait, /* how long to wait, in us */
                    struct ata_device* dev,
                    WORD* packet,        /* the command packet, also used for returning error info */
                    int packet_size,    /* length of packet in bytes (minimum 12 bytes) */
                    WORD* buffer,
                    int max_count,        /* max number of bytes per transfer */
                    unsigned long* total_bytes, /* return actual number of bytes transfered */
                    unsigned long buffer_size) /* for safety, bytes */
{
    int res;
    BYTE status;
    int i;
    BYTE ir;
    BYTE* ebuff = (BYTE*)packet;
    int count;
    int buffer_overflow = FALSE;

    *total_bytes = 0;
    res = device_select( MAX_WAIT_1, dev);
    if(res)
    {
        ebuff[0] = 0;
        *(int*)&ebuff[4] = res;
        return res;
    }
    fd32_outb(dev->interface->command_port + CMD_FEATURES, 0);     /* no overlap or DMA */
    fd32_outb(dev->interface->command_port + CMD_CNT, 0);
    fd32_outb(dev->interface->command_port + CMD_PI_COUNT_L, 0xFF & max_count);
    fd32_outb(dev->interface->command_port + CMD_PI_COUNT_H, 0xFF & (max_count >> 8));
    fd32_outb(dev->interface->command_port + CMD_COMMAND, ATA_CMD_PACKET);
    delay(400);
    if((dev->polled_mode == 0) && (dev->flags & DEV_FLG_IRQ_ON_PCMD))
    {
        /* Only old and odd devices arrive here */
        status = ata_cmd_irq(MAX_WAIT_1, dev->interface);
        if(status == 0xFF)
        {
            ebuff[0] = 0;
            *(int*)&ebuff[4] = ATA_ETOIRQ;
            return ATA_ETOIRQ;
        }
    }
    else
    {
        if(dev_is_busy(dev->interface))
        {
            res = ata_poll(MAX_WAIT_1, &dev_is_busy, dev->interface);
            if(res)
            {
                ebuff[0] = 0;
                *(int*)&ebuff[4] = ATA_ETOBUSY;
                return ATA_ETOBUSY;
            }
        }
        status = fd32_inb(dev->interface->command_port + CMD_STATUS);
    }
    ir = fd32_inb(dev->interface->command_port + CMD_PI_INTERRUPT_REASON);
    if(((ir & 0x07) != ATAPI_CD) /*|| (status & ATA_STATUS_ERR)*/ || !(status & ATA_STATUS_DRQ))
    {
        /* return error info in the command packet buffer */
        ebuff[0] = 1;
        ebuff[1] = dev->status_reg = status;
        ebuff[2] = dev->error_reg = fd32_inb(dev->interface->command_port + CMD_ERROR);
        ebuff[3] = ir;
        *(int*)&ebuff[4] = ATA_EPIPRECMD;
        return ATA_EPIPRECMD;
    }
    /* sending command packet */
    for(i=0; i<packet_size; i+=2)
    {
        fd32_outw(dev->interface->command_port + CMD_DATA, *packet++ );
    }
    while(1)
    {
        if(dev->polled_mode)
        {
            delay(400);
            fd32_inb(dev->interface->control_port);
            if(dev_is_busy(dev->interface))
            {
                res = ata_poll(max_wait, &dev_is_busy, dev->interface);
                if(res)
                {
                    ebuff[0] = 0;
                    *(int*)&ebuff[4] = ATA_ETOBUSY;
                    return ATA_ETOBUSY;
                }
            }
            status = fd32_inb(dev->interface->command_port + CMD_STATUS);
        }
        else
        {
            status = ata_cmd_irq(max_wait, dev->interface);
            if(status == 0xFF)
            {
                ebuff[0] = 0;
                *(int*)&ebuff[4] = ATA_ETOIRQ;
                return ATA_ETOIRQ;
            }
        }
        ir = fd32_inb(dev->interface->command_port + CMD_PI_INTERRUPT_REASON);
        if(ir & ATAPI_CD)
        {
            /* If we arrive here, then there is no more data or there is an error*/

            if(!(ir & ATAPI_IO) || (status & ATAPI_CHECK) || !(status & ATA_STATUS_DRDY))
            {
                /* return error info in the command packet buffer */
                ebuff[0] = 1;
                ebuff[1] = dev->status_reg = status;
                ebuff[2] = dev->error_reg = fd32_inb(dev->interface->command_port + CMD_ERROR);
                ebuff[3] = ir;
                *(int*)&ebuff[4] = ATA_EPIPOSTCMD;
                return ATA_EPIPOSTCMD;
            }
            if(buffer_overflow == TRUE)
            {
                ebuff[0] = 0;
                *(int*)&ebuff[4] = ATA_EOWRFL;
                return ATA_EOWRFL;
            }
            /* Success! */
            return 0;
        }
        if(!(status & ATA_STATUS_DRQ))
        {
            /* return error info in the command packet buffer */
            ebuff[0] = 1;
            ebuff[1] = dev->status_reg = status;
            ebuff[2] = dev->error_reg = fd32_inb(dev->interface->command_port + CMD_ERROR);
            ebuff[3] = ir;
            *(int*)&ebuff[4] = ATA_EDRQ_CLEAR;
            return ATA_EDRQ_CLEAR;
        }
        count = fd32_inb(dev->interface->command_port + CMD_PI_COUNT_H);
        count <<= 8;
        count |= fd32_inb(dev->interface->command_port + CMD_PI_COUNT_L);
        *total_bytes += count;
        /* round up and divide */
        count++;
        count >>= 1;
        if(*total_bytes > buffer_size)
        {
            buffer_overflow = TRUE;
            if(ir & ATAPI_IO)
            {
                /* We have run out of space to store data. We choose to paly along with the device */
                /* to avoid device reset, but we will loose the extra data */
                if(*total_bytes > buffer_size + 1024*1024*16)
                {
                    /* No, this has gone too far! Abort! */
                    ebuff[0] = 0;
                    *(int*)&ebuff[4] = ATA_EPIFATAL;
                    return ATA_EPIFATAL;
                }
                for(i=0; i<count; i++)
                {
                    if(*total_bytes - (count-i)*2 < buffer_size)
                        *buffer++ = fd32_inw(dev->interface->command_port + CMD_DATA);
                    else
                        fd32_inw(dev->interface->command_port + CMD_DATA);
                }
            }
            else
            {
                /* The device wants more data than we have */
                /* We choose abort, but we write what we have first */
                /* TODO */
                ebuff[0] = 0;
                *(int*)&ebuff[4] = ATA_EPIFATAL;
                return ATA_EPIFATAL;
            }
        }
        else
        {
            if(ir & ATAPI_IO)
            {
                for(i=0; i<count; i++)
                {
                    *buffer++ = fd32_inw(dev->interface->command_port + CMD_DATA);
                }
            }
            else
            {
                for(i=0; i<count; i++)
                {
                    fd32_outw(dev->interface->command_port + CMD_DATA, *buffer++ );
                }
            }
        }
    }
}

