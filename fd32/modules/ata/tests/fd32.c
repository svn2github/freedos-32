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
#include <devices.h>
#include <ata-interf.h>
#include "pck-cmd.h"
#include "cd-interf.h"



static int cd_request(DWORD f, void *params)
{
    struct cd_device *d;
    switch (f)
    {
    case FD32_BLOCKREAD:
        {
            fd32_blockread_t *x = (fd32_blockread_t*) params;
            ;
            if (x->Size < sizeof(fd32_blockread_t))
            {
                return -EINVAL;
            }
            d = (struct cd_device*) x->DeviceId;
            if(d->flags & CD_FLAG_MOUNTED)
            {
                return cd_read(d, x->Start, x->NumBlocks, x->Buffer);
            }
            else
                return CD_ERR_UNMOUNTED;
        }
    case FD32_BLOCKINFO:
        {
            fd32_blockinfo_t *x = (fd32_blockinfo_t*) params;
            if (x->Size < sizeof(fd32_blockinfo_t))
            {
                return -EINVAL;
            }
            d = (struct cd_device*) x->DeviceId;
            if(d->flags & CD_FLAG_MOUNTED)
            {
                x->BlockSize = d->bytes_per_sector;
                x->TotalBlocks = d->total_blocks;
                x->Type = FD32_BICD;
                x->MultiBootId = d->multiboot_id;
                return 0;
            }
            else
                return CD_ERR_UNMOUNTED;
        }
    /* Call this before trying to access any of the other commands */
    case CD_PREMOUNT:
        {
            cd_dev_parm_t *x = (cd_dev_parm_t*) params;
            if (x->Size < sizeof(cd_dev_parm_t))
            {
                return -EINVAL;
            }
            d = (struct cd_device*) x->DeviceId;
            return cd_premount( d );
        }
    /* Device info in addition to the block info */
    case CD_DEV_INFO:
        {
            cd_dev_info_t *x = (cd_dev_info_t*) params;
            if (x->Size < sizeof(cd_dev_info_t))
            {
                return -EINVAL;
            }
            d = (struct cd_device*) x->DeviceId;
            *(int*)&(x->name[0]) = *(int*)&(d->in_name[0]);
            x->type = d->type;
            x->multiboot_id = d->multiboot_id;
            return 0;
        }
    /* Set timeout parameter(s) */
    case CD_SET_TO:
        {
            cd_set_tout_t *x = (cd_set_tout_t*) params;
            if (x->Size < sizeof(cd_set_tout_t))
            {
                return -EINVAL;
            }
            d = (struct cd_device*) x->DeviceId;
            d->tout_read_us = x->tout_read_us;
            return 0;
        }
    }
    return -ENOTSUP;
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
        /* Size of command packet, usually 12, bu may be 16 */
        d->cmd_size = inf.CmdSize;
        d->flags = 0;
        d->multiboot_id = 0;
        d->tout_read_us = 30 * 1000 * 1000;
        ksprintf(d->out_name, "cdrom%i", i);
        fd32_message("[CDROM] ATAPI device %s becomes block device %s\n", d->in_name, d->out_name);
        if(fd32_dev_register(cd_request, d, d->out_name) < 0)
            fd32_message("[CDROM] failed to register device!\n");
        i++;
    }
    if(i == 0)
        fd32_message("[CDROM] No CD-ROM or DVD drives found!\n");
    return 0;
}
