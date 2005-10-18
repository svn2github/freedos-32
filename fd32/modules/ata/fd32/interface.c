/* ATA Driver for FD32
 * by Luca Abeni & Nils Labugt
 *
 * This is free software; see GPL.txt
 */

#include <dr-env.h>
#include <errno.h>
#include <devices.h>
#include "ata.h"
#include "disk.h"
#include "ata-ops.h"
#include "partscan.h"
#include "ata-interf.h"
#include "../../block/block.h" /* FIXME! */


extern int ata_global_flags;
//#define _DEBUG_


static int ata_bi_open(void *handle);
static int ata_bi_close(void *handle);
static int ata_bi_get_device_info(void *handle, BlockDeviceInfo *buf);
static int ata_bi_get_medium_info(void *handle, BlockMediumInfo *buf);
static ssize_t ata_bi_read(void *handle, void *buffer, uint64_t start, size_t count, int flags);
static ssize_t ata_bi_write(void *handle, const void *buffer, uint64_t start, size_t count, int flags);
static int ata_bi_stub(void *handle);
static int ata_request(int function, ...);

static unsigned ref_counter;
static struct BlockOperations block_ops =
    {
        .request = &ata_request,
        .open = &ata_bi_open,
        .revalidate = &ata_bi_stub,
        .close = &ata_bi_close,
        .get_device_info = &ata_bi_get_device_info,
        .get_medium_info = &ata_bi_get_medium_info,
        .read = &ata_bi_read,
        .write = &ata_bi_write,
        .sync = &ata_bi_stub,
        .test = &ata_bi_stub
    };

int ata_bi_open(void *handle)
{
    struct ata_device *d = handle;

    /* This is mostly a stub for now */
    if(d->flags & DEV_FLG_IS_OPEN)
        return -EBUSY;
    d->flags |= DEV_FLG_IS_OPEN;
    return 0;
}


int ata_bi_close(void *handle)
{
    struct ata_device *d = handle;

    d->flags &= ~DEV_FLG_IS_OPEN;
    return 0;
}

int ata_bi_get_device_info(void *handle, BlockDeviceInfo *buf)
{
    struct ata_device *d = handle;

    buf->flags = d->type;
    buf->multiboot_id = d->multiboot_id;
    return 0;
}

int ata_bi_get_medium_info(void *handle, BlockMediumInfo *buf)
{
    struct ata_device *d = handle;

    if(!(d->flags & DEV_FLG_IS_OPEN))
        return -EBUSY;
    buf->blocks_count = (uint64_t)d->total_blocks;
    buf->block_bytes = d->bytes_per_sector;
    return 0;
}

ssize_t ata_bi_read(void *handle, void *buffer, uint64_t start, size_t count, int flags)
{
    int res;
    struct ata_device *d = handle;

    if(!(d->flags & DEV_FLG_IS_OPEN))
        return -EBUSY;
    res = ata_read(d, start, count, buffer);
    /*
        If ata_read returns an error code, then most likely a reset is required.
        This should not happen if both software and hardware is in order, except
        in the case of sleep. I think this reset should normaly happen in this
        driver, but this is not implemented yet. In the mean time, just return
        -EIO.
    */
    if(res<0)
    {
        if(res == ATA_ERANGE)
            return -BLOCK_ERROR(BLOCK_SENSE_ILLREQ, 0);
        else
            return -EIO;
    }
    return res;
}

ssize_t ata_bi_write(void *handle, const void *buffer, uint64_t start, size_t count, int flags)
{
    int res;
    struct ata_device *d = handle;

    if(!(d->flags & DEV_FLG_IS_OPEN))
        return -EBUSY;
    res = ata_write(d, start, count, buffer);
    /* See the comment for the read function above. */
    if(res<0)
    {
        if(res == ATA_ERANGE)
            return -BLOCK_ERROR(BLOCK_SENSE_ILLREQ, 0);
        else
            return -EIO;
    }
    return res;
}


int ata_bi_stub(void *handle)
{
    return 0;
}


/*static*/
int ata_request(int function, ...)
{
    va_list parms;
    struct ata_device *d;
    va_start(parms,function);

#ifdef _DEBUG_

    message("/// %lu ", f);
    int res;
#endif

    switch (function)
    {
    case REQ_GET_OPERATIONS:
        {
            int type = va_arg(parms, int);
            void **operations = va_arg(parms, void**);
            if(type != BLOCK_OPERATIONS_TYPE)
                return -EINVAL;
            if(operations != NULL)
                *operations = &block_ops;
            ref_counter++;
            return 0;
        }
    case REQ_GET_REFERENCES:
        {
            return ref_counter;
        }
    case REQ_RELEASE:
        {
            ref_counter--;
            return 0;
        }
    case REQ_ATA_STANBY_IMM:
        {

            d = va_arg(parms, struct ata_device*);
            d->flags |= DEV_FLG_STANDBY;
            return ata_standby_imm( d );
        }
    case REQ_ATA_SLEEP:
        {
            /* WARNING! Reset is neccessary to wake up from sleep mode */
            d = va_arg(parms, struct ata_device*);
            d->flags |= DEV_FLG_SLEEP;
            return ata_sleep( d );
        }
    case REQ_ATA_SRESET:
        {
            /* Soft reset of device */
            d = va_arg(parms, struct ata_device*);
            return ata_sreset( d );
        }
    }
    return -ENOTSUP;
}

static int atapi_request(DWORD f, void *params)
{
    struct ata_device *d;

    switch (f)
    {
    case FD32_ATAPI_INFO:
        {

            atapi_info_t *x;

            x = (atapi_info_t *) params;
            if (x->Size < sizeof(atapi_info_t))
            {
                return -EINVAL;
            }
            d = (struct ata_device *) x->DeviceId;
            x->PIDevType = d->type;
            if(d->general_config & 1)
                x->CmdSize = 16;
            else
                x->CmdSize = 12;
            return 0;
        }
    case FD32_ATAPI_PACKET:
        {

            atapi_pc_parm_t *x;

            x = (atapi_pc_parm_t *) params;
            if (x->Size < sizeof(atapi_pc_parm_t))
            {
                return -EINVAL;
            }
            d = (struct ata_device *) x->DeviceId;
            return ata_packet_pio( x->MaxWait, d, x->Packet, x->PacketSize,
                                   x->Buffer, x->MaxCount, x->TotalBytes, x->BufferSize);
        }
    case REQ_ATA_DRESET:
        {
            /* Soft reset of device */
            ata_dev_parm_t *x;

            x = (ata_dev_parm_t *) params;
            if (x->Size < sizeof(ata_dev_parm_t))
            {
                return -EINVAL;
            }
            d = (struct ata_device *) x->DeviceId;
            return ata_dev_reset( d );
        }
    case REQ_ATA_SRESET:
    case REQ_ATA_SLEEP:
    case REQ_ATA_STANBY_IMM:
        {
            ata_dev_parm_t *x;

            x = (ata_dev_parm_t *) params;
            if (x->Size < sizeof(ata_dev_parm_t))
            {
                return -EINVAL;
            }
            d = (struct ata_device *) x->DeviceId;
            return ata_request( f, d );
        }
    }
    return -ENOTSUP;
}


void disk_add(struct ata_device *d, char *name)
{
    fd32_message("[%s]\n", name);
    fd32_message("Model: %s\n", d->model);
    fd32_message("Serial: %s\n", d->serial);
    fd32_message("Revision: %s\n", d->revision);
    if(d->flags & DEV_FLG_PI_DEV)
    {
        fd32_message("Packet Interface device\n");
        fd32_dev_register(atapi_request, d, name);
    }
    else
    {
        if(d->capabilities & ATA_CAPAB_LBA)
        {
            fd32_message("Device supports LBA\n");
            fd32_message("Total number of blocks: %lu\n", d->total_blocks);
        }
        else
        {
            fd32_message("Cylinders: %lu\n", d->cyls);
            fd32_message("Heads: %lu\n", d->heads);
            fd32_message("Sectors per track: %lu\n", d->sectors);
        }
        if(d->sectors_per_block != 0)
            fd32_message("Using %u sectors block mode\n", (unsigned)d->sectors_per_block);
        if(ata_global_flags & ATA_GFLAG_NWRITE)
            fd32_message("Writing disabled!\n");
        d->type = BLOCK_DEVICE_INFO_TGENERIC | BLOCK_DEVICE_INFO_WRITABLE;
        block_register(name, ata_request, d);
        ata_scanpart(d, name);
    }
}
