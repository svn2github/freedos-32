#include <dr-env.h>
#include <filesys.h>
#include <devices.h>
#include <stubinfo.h>

#include "cd-interf.h"

static char buffer[2048];

int process()
{
    int dev;
    fd32_request_t *req;
    void* cd_devid;
    cd_dev_parm_t prm;
    fd32_blockinfo_t inf;
    char dev_name[9];
    int i;
    unsigned long n;
    int res;
    int fhandle;
    char filename[12];
    fd32_blockread_t r;


    fd32_message("Searching for CD-ROM drives...\n");
    /* Search for all devices with name hd<letter> an test if they respond to FD32_ATAPI_INFO */
    for(i=0;; i++)
    {
        ksprintf(dev_name, "cdrom%i", i);
        dev = fd32_dev_search(dev_name);
        if (dev < 0)
        {
            break;
        }
        if (fd32_dev_get(dev, &req, &cd_devid, NULL, 0) < 0)
        {
            fd32_message("Error: Failed to get device data for device %s!\n", dev_name);
            return 1;
        }
        prm.Size = sizeof(cd_dev_parm_t);
        prm.DeviceId = cd_devid;
        res = req(CD_PREMOUNT, (void*)&prm);
        if(res<0)
        {
            fd32_message("Device %s could not be mounted. No medium?\nReturned %i\n", dev_name, res);
            continue;
        }
        inf.Size = sizeof(fd32_blockinfo_t);
        inf.DeviceId = cd_devid;
        res = req(FD32_BLOCKINFO, (void*)&prm);
        if(res < 0)
        {
            fd32_message("Error: Block info failed for device %s\nReturned %i\n", dev_name, res);
            return 1;
        }
        if(inf.BlockSize != 2048)
        {
            fd32_message("Error: Strange sector size %lu\n", inf.BlockSize);
            return 1;
        }
        ksprintf(filename, "%s.iso", dev_name);
        fhandle = fd32_open(filename, O_CREAT | O_TRUNC | O_WRONLY, FD32_ANONE, 0, NULL);
        if(fhandle < 0)
        {
            fd32_message("Error: Unable to open file %s\n", filename);
            return 1;
        }
        r.Size = sizeof(fd32_blockread_t);
        r.DeviceId = cd_devid;
        r.Buffer = buffer;
        r.NumBlocks = 1;
        for(n=0; n < inf.TotalBlocks; n++)
        {
            r.Start = n;
            res = req(FD32_BLOCKREAD, (void*)&r);
            if(res < 0)
            {
                fd32_message("Error: Block read failed for device %s\nReturned %i\n", dev_name, res);
                fd32_close(fhandle);
                return 1;
            }
            res = fd32_write(fhandle, buffer, 2048);
            if(res<0)
            {
                fd32_message("Error: unable to write to file\n");
                fd32_close(fhandle);
                return 1;
            }

        }
        fd32_close(fhandle);
        fd32_message("%lu sectors written to file %s\n", inf.TotalBlocks, filename);
    }
    return 0;
}


void restore_sp(int res);

extern struct psp *current_psp;
static struct psp local_psp;

void cd_test_init(struct process_info *pi)
{
    int res;
    local_psp.jft_size = 6;
    local_psp.jft = fd32_init_jft(6);
    local_psp.link = current_psp;
    current_psp = &local_psp;
    res = process();
    fd32_free_jft(current_psp->jft, current_psp->jft_size);
    current_psp = current_psp->link;
    restore_sp(res);
}

