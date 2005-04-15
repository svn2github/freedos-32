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
 
 
/* Unspecified error */
#define CD_ERR_GENERAL -1
/* End of medium */
#define CD_ERR_END -2
#define CD_ERR_LENGTH -3
/* Media may have changed */
#define CD_ERR_MEDIA_CHANGE -4
/* Device is not available before premount is called */
#define CD_ERR_UNMOUNTED -5
/* Device is in progress of becoming ready */
#define CD_ERR_NR_PROGRESS -6
/* Not ready, initializing command requiered */
#define CD_ERR_NR_INIT_REQ -7
/* Not ready, manual intervention needed */
#define CD_ERR_NR_MANUAL_INTERV_REQ -8
/* Not ready, some form of operation in progress */
#define CD_ERR_NR_OP_IN_PROGR -9
/* Not ready */
#define CD_ERR_NR -10
/* Address (LBA etc) out of range */
#define CD_ERR_ADDR_RANGE -11
/* No reference position, medium may be upside down */
#define CD_ERR_NO_REF_POS -12
/* Invalid request, unspecified */
#define CD_ERR_INVALID_REQ -13
/* Fatal error, software reset may be needed */
#define CD_ERR_FATAL -14
/* Incompatible or unknown format */
#define CD_ERR_MEDIUM_FORMAT -15
/* Hardware failure in the device */
#define CD_ERR_HARDWARE_FAILURE -16
/* No medium present */
#define CD_ERR_NO_MEDIUM -17
/* Medium error, unrecovered read error */
#define CD_ERR_UNREC_READ -18
/* Medium error, defect list */
#define CD_ERR_DEFECT_LIST -19
/* Medium error */
#define CD_ERR_MEDIUM -20
/* Device has been reset */
#define CD_ERR_RESET -21
/* Device has entered low power mode */ 
#define CD_ERR_POWER_MODE -22
/* Unspecified unit attention condition */
#define CD_ERR_UNIT_ATTENTION -23
/* Command was aborted */
#define CD_ERR_ABORTED_CDM -24


#define CD_FLAG_MOUNTED 1
#define CD_FLAG_FATAL_ERROR 2
#define CD_FLAG_IN_PROGRESS 4
#define CD_FLAG_RETRY 8

/* The ILI bit in sense data byte 2 */
#define CD_ILI 0x20

struct cd_device
{
    void* device_id;
    fd32_request_t *req;
    char in_name[4];
    char out_name[8];
    int cmd_size;
    int type;
    int flags;
    DWORD total_blocks;
    DWORD bytes_per_sector;
    DWORD multiboot_id;
    DWORD tout_read_us;
};

struct packet_read10 
{
    BYTE opcode;
    BYTE flags1;
    DWORD lba;
    BYTE res;
    WORD transfer_length;
    BYTE flags2;
    /* 4 + 2 bytes padding */
    WORD pad1;
    DWORD pad2;
} __attribute__ ((__packed__));

struct packet_error
{
    BYTE flags;
    BYTE status;
    BYTE error;
    BYTE int_reason;
    int ret_code;
} __attribute__ ((__packed__));

struct cd_sense
{
    BYTE error_code;
    BYTE segment_number;
    BYTE sense_key;
    DWORD information;
    BYTE add_sense_length;
    DWORD cmd_specific;
    BYTE asc;
    BYTE asc_q;
    BYTE fruc;
    BYTE sk_sp[3];
} __attribute__ ((__packed__));

int cd_premount( struct cd_device* d );
int cd_read(struct cd_device* d, DWORD start, DWORD blocks, char* buffer);
