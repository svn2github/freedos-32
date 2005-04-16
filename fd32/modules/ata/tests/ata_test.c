/* Test for the FD32 ATA Driver
 * by Nils Labugt
 *
 * This is free software; see GPL.txt
 */

#include <dr-env.h>
#include <devices.h>
#include "ata-interf.h"


static fd32_request_t *ata_request;
unsigned mtr;

int read(unsigned bpr, unsigned rqsts, fd32_blockread_t* r, void* b)
{
    int i;
    TIME start, end, t;
    
    r->NumBlocks = bpr;
    start = ll_gettime(TIME_NEW, NULL);
    for(i=0;i<rqsts;i++)
    {
        if(ata_request(FD32_BLOCKREAD, r) < 0)
        {
            fd32_message("Failed to read sectors\n");
            fd32_kmem_free(b, 1024*1024);
            return -1;
        }
        r->Start += bpr;
    }
    end = ll_gettime(TIME_NEW, NULL);
    t = end - start;
    t /= 1000;
    fd32_message( "%u sectors per request: %lums, %luKiB/s\n", bpr, t, (mtr * 1024 * 1000) / t);
    return 0;
}    

int ata_test_init(struct process_info *p)
{
    int part;
    fd32_blockread_t ata_read;
    void* ata_devid;
    void* b;
    ata_dev_parm_t sb;
    char* args;
    char c;
    
    mtr = 0;
    args = args_get(p);
    while(*args == ' ')
        args++;
    while(!(*args < '0' || *args > '9'))
    {
        c = *args;
        c -= '0';
        mtr *= 10;
        mtr += c;
        args++;
    }
    if(mtr > 1024)
    {
        fd32_message("Argument too big!\n");
        return 0;
    }
    if(mtr == 0)
    {
        fd32_message("Error:  Numer of MiBs to read must be specified.\n");
        return 0;
    }
    
    fd32_message("Searching for device \"hda\"...\n");
    part = fd32_dev_search("hda");
    if (part < 0)
    {
        fd32_message("hda not found!\n");
        return 0;
    }
    if (fd32_dev_get(part, &ata_request, &ata_devid, NULL, 0) < 0)
    {
        fd32_message("Failed to get device data!!!\n");
        return 0;
    }
    if ((b = fd32_kmem_get(1024*1024)) == NULL)
    {
        fd32_message("Failed to allocate 1MiB of memory!\n");
        return 0;
    }
    ata_read.Size = sizeof(fd32_blockread_t);
    ata_read.DeviceId = ata_devid;
    ata_read.Start = 0;
    ata_read.Buffer = b;
    fd32_message("Reading first %u sectors into memory...\n", 2048 * mtr);
    if(read(2048, 2048 * mtr / 2048, &ata_read, b) < 0)
        return 0;
    if(read(32, 2048 * mtr / 32, &ata_read, b) < 0)
        return 0;
    if(read(16, 2048 * mtr / 16, &ata_read, b) < 0)
        return 0;
    if(read(8, 2048 * mtr / 8, &ata_read, b) < 0)
        return 0;
    if(read(1, 2048 * mtr / 1, &ata_read, b) < 0)
        return 0;
    fd32_message("Testing standby...\n");
    sb.Size = sizeof(ata_dev_parm_t);
    sb.DeviceId = ata_devid;
    ata_request(FD32_ATA_STANBY_IMM, &sb);
    fd32_message("Is the drive spinning down?\n");
    return 0;
}
