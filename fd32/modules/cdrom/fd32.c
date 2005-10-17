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
#include <errno.h>
#include <ata-interf.h>
#include "cd-interf.h"
#include "pck-cmd.h"
#include "../block/block.h" /* FIXME! */

static int cd_bi_open(void *handle);
static int cd_bi_revalidate(void *handle);
static int cd_bi_close(void *handle);
static int cd_bi_get_device_info(void *handle, BlockDeviceInfo *buf);
static int cd_bi_get_medium_info(void *handle, BlockMediumInfo *buf);
static ssize_t cd_bi_read(void *handle, void *buffer, uint64_t start, size_t count, int flags);
static ssize_t cd_bi_write(void *handle, const void *buffer, uint64_t start, size_t count, int flags);
static int cd_bi_sync(void *handle);
static int cd_request(int function, ...);

static unsigned ref_counter;
static struct BlockOperations block_ops =
    {
        .request = &cd_request,
        .open = &cd_bi_open,
        .revalidate = &cd_bi_revalidate,
        .close = &cd_bi_close,
        .get_device_info = &cd_bi_get_device_info,
        .get_medium_info = &cd_bi_get_medium_info,
        .read = &cd_bi_read,
        .write = &cd_bi_write,
        .sync = &cd_bi_sync
    };




static int cd_bi_open(void *handle)
{
    int res;
    struct cd_device *d = handle;

    if(d->flags & CD_FLAG_IS_OPEN)
        return -EBUSY;
    /* Since we are still in alpha, and since a need for reset is a sign of a
        bug (on functional hardware), don't mask it with reset */
    res = cd_clear( d, CD_CLEAR_FLAG_NO_RESET );
    if(!(res<0))
        d->flags |= CD_FLAG_IS_OPEN;
    return res;
}

static int cd_bi_revalidate(void *handle)
{
    struct cd_device *d = handle;

    return cd_clear( d, CD_CLEAR_FLAG_NO_RESET );
}

static int cd_bi_close(void *handle)
{
    struct cd_device *d = handle;

    d->flags &= ~CD_FLAG_IS_OPEN;
    return 0;
}

static int cd_bi_get_device_info(void *handle, BlockDeviceInfo *buf)
{
    struct cd_device *d = handle;

    buf->flags = d->type2;
    buf->multiboot_id = d->multiboot_id;
    return 0;
}

static int cd_bi_get_medium_info(void *handle, BlockMediumInfo *buf)
{
    struct cd_device *d = handle;

    if(!((d->flags & CD_FLAG_IS_OPEN) & (d->flags & CD_FLAG_IS_VALID )))
        return -EBUSY;
    buf->blocks_count = (uint64_t)d->total_blocks;
    buf->block_bytes = d->bytes_per_sector;
    return 0;
}

static ssize_t cd_bi_read(void *handle, void *buffer, uint64_t start, size_t count, int flags)
{
    struct cd_device *d = handle;

    if(!((d->flags & CD_FLAG_IS_OPEN) & (d->flags & CD_FLAG_IS_VALID )))
        return -EBUSY;
    /* We ignore the flags parameter for now since we have no cache yet. */
    return cd_read(d, start, count, buffer);
}

static ssize_t cd_bi_write(void *handle, const void *buffer, uint64_t start, size_t count, int flags)
{
    /* Will we ever use the block interface to write to a CD? */
    return -ENOTSUP;
}

static int cd_bi_sync(void *handle)
{
    return 0;
}


static int cd_request(int function, ...)
{
    va_list parms;
    struct cd_device *d;
    va_start(parms,function);
    switch (function)
    {
        case REQ_CD_TEST:
        {
            d = va_arg(parms, struct cd_device*);
            return cd_test_unit_ready(d);
        }
        case REQ_CD_EXTENDED_ERROR_REPORT:
        {
            d = va_arg(parms, struct cd_device*);
            struct cd_error_info** ei = va_arg(parms, struct cd_error_info**);
            return cd_extra_error_report(d, ei);
        }
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
        /* Set timeout parameter(s) */
    case REQ_CD_SET_TO:
        {
            d = va_arg(parms, struct cd_device*);
            d->tout_read_us = va_arg(parms, uint32_t);
            return 0;
        }
    }
    return -ENOTSUP; /* ? */
}


int cd_init(struct process_info *p)
{
    int dev;
    fd32_request_t *req;
    void* ata_devid;
    atapi_info_t inf;
    char* args;
    char c;
    char dev_name[4] = "hd_";
    struct cd_device* d;
    int i;

    args = args_get(p);
    ref_counter = 0;
    fd32_message("[CDROM] Searching for CD-ROM drives...\n");
    /* Search for all devices with name hd<letter> an test if they respond to FD32_ATAPI_INFO */
    for(c='a', i=0; c<='z'; c++)
    {
        dev_name[2] = c;
        dev = fd32_dev_search(dev_name);
        if (dev < 0)
        {
            continue;
        }
        if (fd32_dev_get(dev, &req, &ata_devid, NULL, 0) < 0)
        {
            fd32_message("[CDROM] Failed to get device data for device %s!\n", dev_name);
            continue;
        }
        inf.Size = sizeof(atapi_info_t);
        inf.DeviceId = ata_devid;
        if(req(FD32_ATAPI_INFO, (void*)&inf) < 0)
            continue;
        d = (struct cd_device*)fd32_kmem_get(sizeof(struct cd_device));
        if (d == NULL)
        {
            fd32_message("[CDROM] Unable to allocate disk structure!\n");
            return 0;
        }
        d->device_id = ata_devid;
        d->req = req;
        /* Name fits in an int */
        *((int*)(d->in_name)) = *((int*)dev_name);
        /* TODO: check type field */
        d->type = inf.PIDevType;
        d->type2 = BLOCK_DEVICE_INFO_TCDDVD | BLOCK_DEVICE_INFO_REMOVABLE | BLOCK_DEVICE_INFO_MEDIACHG;
        /* Size of command packet, usually 12, bu may be 16 */
        d->cmd_size = inf.CmdSize;
        d->flags = 0;
        d->multiboot_id = 0;
        d->tout_read_us = 30 * 1000 * 1000;
        ksprintf(d->out_name, "cdrom%i", i);
        fd32_message("[CDROM] ATAPI device %s becomes block device %s\n", d->in_name, d->out_name);
        if(block_register(d->out_name, cd_request, d) < 0)
            fd32_message("[CDROM] failed to register device!\n");
        i++;
    }
    if(i == 0)
        fd32_message("[CDROM] No CD-ROM or DVD drives found!\n");
    return 0;
}
