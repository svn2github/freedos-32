/* ATA Driver for FD32
 * by Luca Abeni & Nils Labugt
 *
 * This is free software; see GPL.txt
 */


#ifndef __INTERFACE_H__
#define __INTERFACE_H__

#define FD32_ATA_STANBY_IMM 12345


typedef struct ata_stanby_imm
{
    DWORD Size;
    void *DeviceId;
}
ata_stanby_imm_t;

#endif
