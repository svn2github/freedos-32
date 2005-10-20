#include <dr-env.h>
#include <filesys.h>
#include <devices.h>
#include <block/block.h>

#include "cd-interf.h"

static char buffer[2048];
unsigned long sectors_to_read;

int process()
{
    BlockMediumInfo bmi;
    const char* dev_name;
    BlockOperations* bops;
    void* iterator = NULL;
    void* cd_devid;
    unsigned long n;
    int res;
    int fhandle;
    char filename[12];


    fd32_message("Searching for CD-ROM drives...\n");
    /* Search for all devices with name hd<letter> an test if they respond to FD32_ATAPI_INFO */
    while ((dev_name = block_enumerate(&iterator)) != NULL)
    {
        if (strncasecmp(dev_name, "cdrom", 5))
        {
            continue;
        }
        if (block_get(dev_name, BLOCK_OPERATIONS_TYPE, &bops, &cd_devid) < 0)
        {
            fd32_message("Error: Failed to get device data for device %s!\n", dev_name);
            return 1;
        }
        res = bops->open(cd_devid);
        if(res<0)
        {
            fd32_message("Device %s could not be mounted. No medium?\nReturned %i\n", dev_name, res);
            continue;
        }
        res = bops->get_medium_info(cd_devid, &bmi);
        if(res < 0)
        {
            fd32_message("Error: Block medium info failed for device %s\nReturned %i\n", dev_name, res);
            return 1;
        }
        if(bmi.block_bytes != 2048)
        {
            fd32_message("Error: Strange sector size %u\n", bmi.block_bytes);
            return 1;
        }
        fd32_message("Disk contains %llu sectors\n", bmi.blocks_count);
        if(sectors_to_read >= bmi.blocks_count)
        {
            ksprintf(filename, "%s.iso", dev_name);
            sectors_to_read = bmi.blocks_count;
        }
        else
        {
            ksprintf(filename, "%s.dat", dev_name);
        }
        fd32_message("Going to write %lu bytes to file %s...\n", sectors_to_read * 2048, filename);
        fhandle = fd32_open(filename, O_CREAT | O_TRUNC | O_WRONLY, FD32_ANONE, 0, NULL);
        if(fhandle < 0)
        {
            fd32_message("Error: Unable to open file %s\n", filename);
            return 1;
        }
        for(n=0; n < sectors_to_read; n++)
        {
            fd32_log_printf("[CDTEST] Write sector %lu/%lu\n", n, sectors_to_read);
            res = bops->read(cd_devid, buffer, n, 1, 0);
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
        fd32_message("%lu sectors written to file %s\n", sectors_to_read, filename);
    }
    return 0;
}


void cd_test_init(struct process_info *pi)
{
    int res;
    char* args;
    char c;

    sectors_to_read = 0;
    args = args_get(pi);
    while(*args == ' ')
        args++;
    if(args[0] == '-' && args[1] == 'a')
        sectors_to_read = 0xFFFFFFFF;
    else
    {
        while(!(*args < '0' || *args > '9'))
        {
            c = *args;
            c -= '0';
            sectors_to_read *= 10;
            sectors_to_read += c;
            args++;
        }
    }
    if(sectors_to_read == 0)
    {
        fd32_message("Error:  Numer of sectors to read per drive must be specified, or \"-a\" for all.\n");
        return;
    }
    pi->jft_size = 6;
    pi->jft = fd32_init_jft(6);
    res = process();
    fd32_free_jft(pi->jft, pi->jft_size);
}

