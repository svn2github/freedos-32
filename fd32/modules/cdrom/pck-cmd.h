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
 
#if 0
#define CD_EOFF -3000

#define CD_ERR_GENERAL		CD_EOFF-1	/* Unspecified error */
#define CD_ERR_END		CD_EOFF-2	/* End of medium */
#define CD_ERR_LENGTH		CD_EOFF-3
#define CD_ERR_MEDIA_CHANGE	CD_EOFF-4	/* Media may have changed */
#define CD_ERR_UNMOUNTED	CD_EOFF-5	/* Device is not available before premount is called */
#define CD_ERR_NR_PROGRESS	CD_EOFF-6	/* Device is in progress of becoming ready */
#define CD_ERR_NR_INIT_REQ	CD_EOFF-7	/* Not ready, initializing command requiered */
#define CD_ERR_NR_MANUAL_INTERV_REQ CD_EOFF-8	/* Not ready, manual intervention needed */
#define CD_ERR_NR_OP_IN_PROGR	CD_EOFF-9	/* Not ready, some form of operation in progress */
#define CD_ERR_NR		CD_EOFF-10	/* Not ready */
#define CD_ERR_ADDR_RANGE 	CD_EOFF-11	/* Address (LBA etc) out of range */
#define CD_ERR_NO_REF_POS 	CD_EOFF-12	/* No reference position, medium may be upside down */
#define CD_ERR_INVALID_REQ 	CD_EOFF-13	/* Invalid request, unspecified */
#define CD_ERR_FATAL 		CD_EOFF-14	/* Fatal error, software reset may be needed */
#define CD_ERR_MEDIUM_FORMAT 	CD_EOFF-15	/* Incompatible or unknown format */
#define CD_ERR_HARDWARE_FAILURE CD_EOFF-16 	/* Hardware failure in the device */
#define CD_ERR_NO_MEDIUM 	CD_EOFF-17	/* No medium present */
#define CD_ERR_UNREC_READ 	CD_EOFF-18	/* Medium error, unrecovered read error */
#define CD_ERR_DEFECT_LIST 	CD_EOFF-19	/* Medium error, defect list */
#define CD_ERR_MEDIUM 		CD_EOFF-20	/* Medium error */
#define CD_ERR_RESET 		CD_EOFF-21	/* Device has been reset */
#define CD_ERR_POWER_MODE 	CD_EOFF-22	/* Device has entered low power mode */
#define CD_ERR_UNIT_ATTENTION 	CD_EOFF-23 	/* Unspecified unit attention condition */
#define CD_ERR_ABORTED_CDM 	CD_EOFF-24	/* Command was aborted */
#endif


#define CD_FLAG_IS_OPEN 1
#define CD_FLAG_IS_VALID 2
#define CD_FLAG_NEED_RESET 4
#define CD_FLAG_IN_PROGRESS 8
#define CD_FLAG_RETRY 16
#define CD_FLAG_EXTRA_ERROR_INFO 32

/* The ILI bit in sense data byte 2 */
#define CD_ILI 0x20

#define SENSE_KEY_HARDWARE 4

#define CD_CLEAR_FLAG_NO_RESET 1
#define CD_CLEAR_FLAG_NO_SRESET 2
#define CD_CLEAR_FLAG_RETRY_FAILED_SENSE 4
#define CD_CLEAR_FLAG_REPORT_SENSE 8


struct cd_device
{
    void* device_id;
    fd32_request_t *req;
    char in_name[4];
    char out_name[8];
    int cmd_size;
    int type;
    int type2;  /* For use by the block interface. */
    int flags;
    DWORD total_blocks;
    DWORD bytes_per_sector;
    DWORD multiboot_id;
    DWORD tout_read_us;
    struct cd_error_info errinf;
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
    BYTE sense_key;     /* remember to AND with 0x0F!*/
    DWORD information;
    BYTE add_sense_length;
    DWORD cmd_specific;
    BYTE asc;
    BYTE asc_q;
    BYTE fruc;
    BYTE sk_sp[3];
} __attribute__ ((__packed__));

int cd_clear( struct cd_device* d, int flags );
int cd_read(struct cd_device* d, DWORD start, DWORD blocks, char* buffer);
int cd_test_unit_ready(struct cd_device* d);
int cd_extra_error_report(struct cd_device* d, struct cd_error_info** ei);

