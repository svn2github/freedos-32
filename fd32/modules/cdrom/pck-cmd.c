/***************************************************************************
 *   CD-ROM driver for FD32                                                *
 *   Copyright (C) 2005 by Nils Labugt                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include <dr-env.h>
#include <devices.h>
#include <ata-interf.h>
#include <errno.h>
#include "pck-cmd.h"
#include "../block/block.h" /* FIXME! */

#define _DEBUG_
#define LOG_MSG fd32_log_printf

DWORD endianness4(DWORD n)
{
    return (n & 0x00FF0000)>>8 | (n & 0x0000FF00)<<8 | n<<24 | n>>24;
}

WORD endianness2(WORD n)
{
    return n<<8 | n>>8;
}

/* Retrieve error information from the device */
static int cd_request_sense(struct cd_device* d, struct cd_sense* s)
{
    atapi_pc_parm_t pcp;
    BYTE packet[16];
    unsigned long tb;
    int res;

    pcp.Size = sizeof(atapi_pc_parm_t);
    pcp.DeviceId = d->device_id;
    pcp.MaxWait = 1 * 1000 * 1000; /* this is relatively arbitrary */
    pcp.PacketSize = d->cmd_size;
    pcp.Buffer = (WORD*)s;
    pcp.BufferSize = sizeof(struct cd_sense);
    pcp.MaxCount = sizeof(struct cd_sense);
    pcp.TotalBytes = &tb;
    pcp.Packet = (WORD*)&packet;
    /* Prepare command packet */
    memset((void*)packet, 0, 16);
    packet[0] = 0x03;
    packet[4] = sizeof(struct cd_sense); /* allocation length */
    res = d->req(FD32_ATAPI_PACKET, (void*)&pcp);
#ifdef _DEBUG_

    if(res<0)
        LOG_MSG("REQUEST SENSE cmd failed, returning %i, s=0x%x,e=0x%x,i=0x%x\n",
                res, ((char*)&packet)[1], ((char*)&packet)[2], ((char*)&packet)[3]);
#endif

    return res;
}

/* Diagnose a check condition */
static int cd_check(struct cd_device* d, struct cd_sense* s, int* result)
{
    int res;

    res = cd_request_sense(d, s);
    if(res<0)
    {
        /* Reset probably needed here */
        d->flags &=  ~CD_FLAG_IS_VALID;
        d->flags |= CD_FLAG_NEED_RESET;
        *result = -EIO;
#ifdef _DEBUG_

        LOG_MSG("SENSE failed, returning %i\n", res);
#endif

        return 0;
#if 0

        if(err->error & 0x04) /* Command aborted? */
        {
            /* TODO */
            return CD_EGENERAL;
        }
        if(err->error & 0x02) /* End of media? */
            return CD_EEND;
        if(err->error & 0x01) /* Illegal length? */
            return CD_ELENGHT;
        sense_key = err->error >> 4;
        if(sense_key == 6) /* Media change, device reset etc */
            return CD_EMEDIACHANGE;
        /* TODO */
        return CD_EGENERAL;
#endif

    }
#ifdef _DEBUG_
    LOG_MSG("SENSE: k=0x%x,asc=0x%x,q=0x%x,ec=0x%x\n", s->sense_key, s->asc, s->asc_q, s->error_code);
#endif

    *result = -BLOCK_ERROR(s->sense_key, (s->asc << 8) | s->asc_q);
    switch (s->sense_key & 0x0F)
    {
    case 0:    /* No Sense (no error) */
    case 1:    /* Recovered Error (We just ignore this condition for now) */
        {
            *result = 0;
            break;
        }
    case 2:    /* Not Ready */
        {
            if (s->asc == 0x04)
            {
                switch (s->asc_q)
                {
                case 1:
                case 4:
                case 5:
                case 6:
                case 7:
                case 8:
                    d->flags |= CD_FLAG_IN_PROGRESS;
                }
            }
            break;
        }
    case 6:    /* Unit attention */
        {
#if 1
            if(s->asc == 0x5E) /* CHECKME! */
                break;
#endif

            d->flags &=  ~CD_FLAG_IS_VALID;
            break;
        }
    case 0xB:    /* Aborted Command */
        {
            /* TODO */
            d->flags |= CD_FLAG_RETRY;
            break;
        }
    }
    return 0;
}

/* Error handler */
static int cd_err_handl_1(struct cd_device* d, struct packet_error* err, int err_code, int* result)
{
    int res1, res2;
    struct cd_sense s;

    if(err->flags == 0) /* Byte 1 to 3 invalid? */
    {
        /* TODO: timeout etc */
        return -EIO;
    }
    if(err->status & 1) /* A check condition? */
    {
        res1 = cd_check(d, &s, &res2);
        if(res1<0)
        {
#ifdef _DEBUG_
            LOG_MSG("Error unhandled\n");
#endif

            return -EIO;
        }
        else
            return res2;
    }
    else
    {
        /* ?? */
#ifdef _DEBUG_
        LOG_MSG("No check condition\n");
#endif

        return -EIO;
    }
    /* TODO */
    return -EIO;
}


static int cd_read10(struct cd_device* d, DWORD start, WORD blocks, char* buffer, unsigned long buffer_size)
{
    atapi_pc_parm_t pcp;
    struct packet_read10 packet;
    unsigned long tb;
    int res, res1, res2;

    pcp.Size = sizeof(atapi_pc_parm_t);
    pcp.DeviceId = d->device_id;
    pcp.MaxWait = 30 * 1000 * 1000; /* this is relatively arbitrary */
    pcp.PacketSize = d->cmd_size;
    pcp.Buffer = (WORD*)buffer;
    pcp.BufferSize = buffer_size;
    pcp.MaxCount = 2048; /* check this! */
    pcp.TotalBytes = &tb;
    pcp.Packet = (WORD*)&packet;
    /* Prepare command packet */
    packet.opcode = 0x28;
    packet.lba = endianness4(start);
    packet.transfer_length = endianness2(blocks);
    packet.res = 0;
    packet.pad1 = 0;
    packet.pad2 = 0;
    packet.flags1 = 0;
    packet.flags2 = 0; /* check this */
    res = d->req(FD32_ATAPI_PACKET, (void*)&pcp);
    if(res<0)
    {
#ifdef _DEBUG_
        LOG_MSG("READ(10) cmd failed, returning %i, s=0x%x,e=0x%x,i=0x%x\n",
                res, ((char*)&packet)[1], ((char*)&packet)[2], ((char*)&packet)[3]);
#endif

        res1 = cd_err_handl_1(d, (struct packet_error*)&packet, res, &res2);
        if(res1 < 0)
        {
            /* TODO */
            return res1;
        }
        else
            return res2;
    }
    return 0;
}


static void private_timed_out(void *p)
{
    *((int*)p) = TRUE;
}


static void cd_wait(DWORD us)
{
    volatile int tout;

    tout = FALSE;
    fd32_event_post(us, private_timed_out, (void*)&tout);
    WFC(!tout);
}

int cd_read(struct cd_device* d, DWORD start, DWORD blocks, char* buffer)
{
    int res;
    unsigned long buff_size;
    volatile int tout;
    int tout_event;

    tout = FALSE;
    tout_event = fd32_event_post(d->tout_read_us, private_timed_out, (void *)&tout);
    /* Instead of this calculation we should realy have the caller pass this value */
    buff_size = d->bytes_per_sector * blocks;
    if(start > 0xFFFF)
        return -EINVAL;
    while(1)
    {
        d->flags &= ~(CD_FLAG_RETRY | CD_FLAG_IN_PROGRESS);
        res = cd_read10(d, start, blocks, buffer, buff_size);
        if(res<0)
        {
#ifdef _DEBUG_
            LOG_MSG("READ10 failed, returning %i, flags=0x%x\n", res, (unsigned)d->flags);
#endif

            if( d->flags & CD_FLAG_RETRY)
            {
#ifdef _DEBUG_
                cd_wait(2 * 1000 * 1000);
#else

                cd_wait(100 * 1000);
#endif

                res = cd_read10(d, start, blocks, buffer, buff_size);
                break;
            }
            if( (d->flags & CD_FLAG_IN_PROGRESS) && (tout == FALSE) )
            {
#ifdef _DEBUG_
                cd_wait(2 * 1000 * 1000);
#else

                cd_wait(100 * 1000);
#endif

                continue;
            }
        }
        break;
    }
    fd32_cli();
    if (!tout)
        fd32_event_delete(tout_event);
    fd32_sti();
    return res;
}

/* Get medium capacity */
static int cd_read_capacity(struct cd_device* d, DWORD* blocks, DWORD* block_size)
{
    atapi_pc_parm_t pcp;
    BYTE packet[16];
    unsigned long tb;
    int res, res2;
    DWORD buffer[2];

    pcp.Size = sizeof(atapi_pc_parm_t);
    pcp.DeviceId = d->device_id;
    pcp.MaxWait = 30 * 1000 * 1000; /* this is relatively arbitrary */
    pcp.PacketSize = d->cmd_size;
    pcp.Buffer = (WORD*)buffer;
    pcp.BufferSize = 8;
    pcp.MaxCount = 8;
    pcp.TotalBytes = &tb;
    pcp.Packet = (WORD*)&packet;
    /* Prepare command packet */
    memset((void*)packet, 0, 16);
    packet[0] = 0x25;
    res = d->req(FD32_ATAPI_PACKET, (void*)&pcp);
    if(res<0)
    {
#ifdef _DEBUG_
        LOG_MSG("READ CAPACITY cmd failed, returning %i, s=0x%x,e=0x%x,i=0x%x\n",
                res, packet[1], packet[2], packet[3]);
#endif

        if(cd_err_handl_1(d, (struct packet_error*)&packet, res, &res2) < 0)
        {
            /* TODO */
            return -EIO;
        }
        else
            return res2;
    }
    if(*blocks != NULL)
        *blocks = endianness4(buffer[0]) + 1;
    if(block_size != NULL)
        *block_size = endianness4(buffer[1]);
    return 0;
}


int cd_premount( struct cd_device* d )
{
    int res1, res2;
    int i;
    struct cd_sense s;
    ata_dev_parm_t p;

#ifdef _DEBUG_

    LOG_MSG("Entering premount\n");
#endif

    if(d->flags & CD_FLAG_NEED_RESET)
    {
#ifdef _DEBUG_
        LOG_MSG("Soft reset!\n");
#endif

        p.Size = sizeof(ata_dev_parm_t);
        p.DeviceId = d->device_id;
        res1 = d->req(REQ_ATA_DRESET, (void*)&p);
        if(res1 < 0)
        {
#ifdef _DEBUG_
            LOG_MSG("Device reset failed!");
#endif

            res1 = d->req(REQ_ATA_SRESET, (void*)&p);
            if(res1 < 0)
            {
#ifdef _DEBUG_
                LOG_MSG("Software reset failed!!! Returned %i\n", res1);
#endif

                return -EIO;
            }
        }
        d->flags &= ~CD_FLAG_NEED_RESET;
    }
    /* This should problably be done differently. We may loose important info here... */
    res1 = 0;
    for(i=0;; i++)
    {
        if(i >= 10) /* 10 a bit much? */
        {
            if(res1<0)
                return res1;
            else
                return res2;
        }
        res1 = cd_check( d, &s, &res2);
        /* TODO: test if reset needed */
        /* TODO: test to see if we should return here */
        if(res1 < 0)
        {
            i++;     /* Let faild sense count twice as much. */
            continue;
        }
        if(res2 == 0)
            break;
    }
#ifdef _DEBUG_
    LOG_MSG("Reading capacity...");
#endif

    res1 = cd_read_capacity(d, &(d->total_blocks), &(d->bytes_per_sector));
    if(res1 < 0)
    {
#ifdef _DEBUG_
        LOG_MSG("failed returning %i\n", res1);
#endif

        return res1;
    }
    else
    {
#ifdef _DEBUG_
        LOG_MSG("blocks=%lu bytes/sect=%lu\n", d->total_blocks, d->bytes_per_sector);
#endif

        d->flags |= CD_FLAG_IS_VALID;
        return 0;
    }
}



