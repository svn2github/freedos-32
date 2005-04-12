/* ATA Driver for FD32
 * by Luca Abeni & Nils Labugt
 *
 * This is free software; see GPL.txt
 */


#ifndef __INTERFACE_H__
#define __INTERFACE_H__

#define FD32_ATA_STANBY_IMM 12345
#define FD32_ATAPI_INFO     12346
#define FD32_ATAPI_PACKET   12347
#define FD32_ATA_SLEEP      12348
#define FD32_ATA_SRESET      12349

typedef struct ata_dev_parm
{
    DWORD Size;
    void *DeviceId;
}
ata_dev_parm_t;

typedef struct atapi_info
{
    DWORD Size;
    void *DeviceId;
    int CmdSize;
    int PIDevType;
}
atapi_info_t;

typedef struct atapi_pc_parm
{
    DWORD Size;
    void *DeviceId;
    unsigned long MaxWait;
    WORD* Packet;
    int PacketSize;
    WORD* Buffer;
    unsigned long BufferSize;
    int MaxCount;
    unsigned long* TotalBytes;
}
atapi_pc_parm_t;

#endif
