/* ATA Driver for FD32
 * by Luca Abeni & Nils Labugt
 *
 * This is free software; see GPL.txt
 */

#ifndef __IDE_H__
#define __IDE_H__
struct ata_device;

struct ide_interface
{
    WORD command_port;
    WORD control_port;
    int irq;
    char basename;
    BYTE current_dev_bit;
    struct ata_device* dev0;
    struct ata_device* dev1;
};

/* Offsets into the command register file */
#define CMD_DATA 0
#define CMD_ERROR 1
#define CMD_FEATURES 1
#define CMD_CNT 2
#define CMD_SECT 3
#define CMD_CYL_L 4
#define CMD_CYL_H 5
#define CMD_DEVHEAD 6
#define CMD_STATUS 7
#define CMD_COMMAND 7
#define CMD_PI_COUNT_L 4
#define CMD_PI_COUNT_H 5
#define CMD_PI_INTERRUPT_REASON 3

/* Bits in the above registers and the control register */
#define ATA_STATUS_DRQ 0x08
#define ATA_STATUS_BSY 0x80
#define ATA_STATUS_ERR 0x01
#define ATA_STATUS_DRDY 0x40
#define ATA_DEVHEAD_DEV 0x10
#define ATA_DEVHEAD_L  0x40
#define ATA_SRST 0x04
#define ATAPI_IO 0x02 
#define ATAPI_CD 0x01 
#define ATAPI_REL 0x04
#define ATAPI_CHECK 0x01 

/* Comman codes for the COMMAND register */
#define ATA_CMD_IDENTIFY 0xEC
#define ATA_CMD_IDENTIFY_PD 0xA1
#define ATA_CMD_READ_MULTIPLE 0xC4
#define ATA_CMD_READ_SECTORS 0x20
#define ATA_CMD_WRITE_MULTIPLE 0xC5
#define ATA_CMD_WRITE_SECTORS 0x30
#define ATA_CMD_SET_MULTIPLE_MODE 0xC6
#define ATA_CMD_STANDBY 0x96
#define ATA_CMD_STANDBY_IMMEDIATE 0x94
#define ATA_CMD_IDLE 0x97
#define ATA_CMD_CHECK_POW_MODE 0x98
#define ATA_CMD_SET_FEATURES 0xEF
#define ATA_CMD_PACKET 0xA0
#define ATA_CMD_SLEEP 0x99
#define ATA_CMD_DEVICE_RESET 0x08

/* Timeout parameters in micro seconds */
#define MAX_WAIT_DETECT 100 * 1000
#define MAX_WAIT_1 500 * 1000

/* Error codes */
#define ATA_EGENERAL -1
#define ATA_ETOREADY -2
#define ATA_ETOIRQ -3
#define ATA_EDRQ_CLEAR -4
#define ATA_ESTATUS -5
#define ATA_ESYNTAX -6
#define ATA_EINVPARAM -7
#define ATA_EPIPRECMD -8
#define ATA_EPIPOSTCMD -9
#define ATA_EOWRFL -10
#define ATA_EPIFATAL -11
#define ATA_ESLEEP -12
#define ATA_DRQ_SET -13
#define ATA_ERANGE -14
#define ATA_ETOBUSY -15

/* Bits in the capabilities word */
#define ATA_CAPAB_LBA 0x200

#if 0
#define ATAPI_NO_DATA 0
#define ATAPI_DATA_OUT 1
#define ATAPI_DATA_IN 2
#endif

/* Global flags */
#define ATA_GFLAG_POLL 1
#define ATA_GFLAG_NWRITE 2
#define ATA_GFLAG_STANDBY_EN 4
#define ATA_GFLAG_PIO_MODE 8
#define ATA_GFLAG_BLOCK_MODE 16

/* Upper four device flags is supported pio modes 1 to 4 */
#define DEV_FLG_SUPP_PIO_BITS 28
/* Device specific flags */
#define DEV_FLG_ST_TIMER_ACTIVE 32
#define DEV_FLG_STANDBY 64
#define DEV_FLG_PI_DEV 128
#define DEV_FLG_IRQ_ON_PCMD 256
#define DEV_FLG_SLEEP 512



#endif
