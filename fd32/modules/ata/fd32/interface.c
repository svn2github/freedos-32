/* ATA Driver for FD32
 * by Luca Abeni & Nils Labugt
 *
 * This is free software; see GPL.txt
 */

#include <dr-env.h>
#include <errors.h>
#include <devices.h>
#include "ata.h"
#include "disk.h"
#include "ata-ops.h"
#include "partscan.h"
#include "ata-interf.h"

extern int ata_global_flags;
//#define _DEBUG_

#ifdef _DEBUG_
void block_dump(unsigned char* b, int n)
{
    int i;

    for(i=0;i<n;i++)
    {
        if((i & 0x1F) == 0)
        {
            fd32_message("\n");
        }
        else if((i & 0x7) == 0)
        {
            fd32_message("-");
        }
        fd32_message("%2x", (unsigned)(*b++));
    }
    fd32_message("\n");
}
#endif

/*static*/ int ata_request(DWORD f, void *params)
{
    struct ata_device *d;

#ifdef _DEBUG_

    message("/// %lu ", f);
    int res;
#endif

    switch (f)
    {
    case FD32_MEDIACHANGE:
        return FALSE;		/* No removable devices, for now... */
    case FD32_BLOCKREAD:
        {
            fd32_blockread_t *x;

            x = (fd32_blockread_t *) params;
            if (x->Size < sizeof(fd32_blockread_t))
            {
                return FD32_EFORMAT;
            }
            d = (struct ata_device *) x->DeviceId;

#ifdef _DEBUG_

            fd32_message("read:%lx,%lu,%lu,%lu\n", (DWORD)d, x->Start, x->NumBlocks, (DWORD)x->Buffer);
            res = ata_read(d, x->Start, x->NumBlocks, x->Buffer);
            //            block_dump((unsigned char*)(x->Buffer), 512);
            return res;
#else

            return ata_read(d, x->Start, x->NumBlocks, x->Buffer);
#endif

        }
    case FD32_BLOCKWRITE:
        {
            fd32_blockwrite_t *x;

            if(ata_global_flags & ATA_GFLAG_NWRITE) /* TODO: move this to ata_write and device flags */
                return -1;
            x = (fd32_blockwrite_t *) params;
            if (x->Size < sizeof(fd32_blockwrite_t))
            {
                return FD32_EFORMAT;
            }
            d = (struct ata_device *) x->DeviceId;
#ifdef _DEBUG_

            fd32_message("write:%lx,%lu,%lu,%lu\n", (DWORD)d, x->Start, x->NumBlocks, (DWORD)x->Buffer);
            res = ata_read(d, x->Start, x->NumBlocks, x->Buffer);
            //            block_dump((unsigned char*)(x->Buffer), 512);
            return res;
#else

            return ata_write(d, x->Start, x->NumBlocks, x->Buffer);
#endif

        }
    case FD32_BLOCKINFO:
        {
            fd32_blockinfo_t *x;

            x = (fd32_blockinfo_t *) params;
            if (x->Size < sizeof(fd32_blockinfo_t))
            {
                return FD32_EFORMAT;
            }
            d = (struct ata_device *) x->DeviceId;
            x->BlockSize = d->bytes_per_sector;
            x->TotalBlocks = d->total_blocks;
            if(d->type == 0xFFFFFFFF)
                x->Type = FD32_BIGEN;
            else
                x->Type = d->type;
            x->MultiBootId = d->multiboot_id;
#ifdef _DEBUG_

            fd32_message("blinf:%lx,%lu,%lu,%lu\n", (DWORD)d, x->BlockSize, x->TotalBlocks, x->Type);
#endif

            return 0;
        }
    case FD32_ATA_STANBY_IMM:
        {

            ata_dev_parm_t *x;

            x = (ata_dev_parm_t *) params;
            if (x->Size < sizeof(ata_dev_parm_t))
            {
                return FD32_EFORMAT;
            }
            d = (struct ata_device *) x->DeviceId;
            d->flags |= DEV_FLG_STANDBY;
            return ata_standby_imm( d );
        }
    case FD32_ATA_SLEEP:
        {
            /* WARNING! Reset is neccessary to wake up from sleep mode */
            ata_dev_parm_t *x;

            x = (ata_dev_parm_t *) params;
            if (x->Size < sizeof(ata_dev_parm_t))
            {
                return FD32_EFORMAT;
            }
            d = (struct ata_device *) x->DeviceId;
            d->flags |= DEV_FLG_SLEEP;
            return ata_sleep( d );
        }
    case FD32_ATA_SRESET:
        {
            /* Soft reset of device */
            ata_dev_parm_t *x;

            x = (ata_dev_parm_t *) params;
            if (x->Size < sizeof(ata_dev_parm_t))
            {
                return FD32_EFORMAT;
            }
            d = (struct ata_device *) x->DeviceId;
            return ata_sreset( d );
        }

    }
    return FD32_EINVAL;
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
                return FD32_EFORMAT;
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
                return FD32_EFORMAT;
            }
            d = (struct ata_device *) x->DeviceId;
            return ata_packet_pio( x->MaxWait, d, x->Packet, x->PacketSize,
                                   x->Buffer, x->MaxCount, x->TotalBytes, x->BufferSize);
        }
    case FD32_ATA_SRESET:
    case FD32_ATA_SLEEP:
    case FD32_ATA_STANBY_IMM:
        {
            return ata_request( f, params );
        }
    }
    return FD32_EINVAL;
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
        fd32_dev_register(ata_request, d, name);
        ata_scanpart(d, name);
    }
}
