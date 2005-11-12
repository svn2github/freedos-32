/* ATA Driver for FD32
 * by Luca Abeni & Nils Labugt
 *
 * This is free software; see GPL.txt
 */

#include <dr-env.h>
#include "ata.h"
#include "disk.h"
#include "ata-detect.h"
#include "ata-wait.h"

extern unsigned ata_ref_counter;

int ata_global_flags;
static char* ata_args;
BYTE ata_standby_timer;
int ata_max_pio_mode;
int ata_block_mode;

struct ide_interface iface[4] =
    {
        {
            .command_port = 0x1F0,
            .control_port = 0x3F6,
            .basename = 'a',
            .irq = 14
        },
        {
            .command_port = 0x170,
            .control_port = 0x376,
            .basename = 'c',
            .irq = 15
        },
        {
            .command_port = 0x1E8,
            .control_port = 0x3EE,
            .basename = 'e',
            .irq = 11        /* alternate 12 or 9 */
        },
        {
            .command_port = 0x168,
            .control_port = 0x36E,
            .basename = 'g',
            .irq = 10        /* alternate 12 or 9 */
        }
#if 0
        {
            .control_port = 0x1E0,
            .basename = 'i'
        },
        {
            .control_port = 0x160,
            .basename = 'm'
        }
#endif
    };

                                                                                
           
           
/* TODO: define arguments on a per-device basis */
static char* arg_names[] =
    {
        "--poll",
        "--disable-write",
        "--standby",
        "--max-pio-mode",
        "--block-mode",
        "'\0'"
    };

static int ata_fetch_number(char** b, unsigned long* n)
{
    char* p = *b;
    unsigned char c;
    unsigned long k = 0;
    int i = 0;
    while(*p ==' ' || *p == '\t')
        p++;
    while(!(*p < '0' || *p > '9'))
    {
        c = *p;
        c -= '0';
        k *= 10;
        k += c;
        p++;
        i++;
    }
    *b = p;
    *n = k;
    return i;
}


static int ata_parse_args(char* args)
{
    int result = 0;
    int no_match, i, j;
    char* p;
    char c;
    unsigned long n;

    while(*args != '\0')
    {
        while(*args ==' ' || *args == '\n' || *args =='\r' || *args == '\t')
            args++;
        if(*args == '\0')
            break;
        p = args;
        while(*p != '\0' && *p !=' ' && *p != '\n' && *p !='\r' && *p != '\t')
            p++;
        c = *p;
        *p = '\0';
        i = 0;
        no_match = FALSE;
        while(1)
        {
            if(*(arg_names[i]) == '\0')
            {
                no_match = FALSE;
                break;
            }
            if(!strcmp(args, arg_names[i]))
                break;
            i++;
        }
        *p = c;
        args = p;
        if(no_match)
        {
            result = ATA_ESYNTAX;
            continue;
        }
        switch(i)
        {
        case 0:
            ata_global_flags |= ATA_GFLAG_POLL;
            break;
        case 1:
            ata_global_flags |= ATA_GFLAG_NWRITE;
            break;
        case 2:
            ata_global_flags |= ATA_GFLAG_STANDBY_EN;
            j = ata_fetch_number(&args, &n);
            if( j > 4 )
            {
                ata_standby_timer = 0;
                result = ATA_ESYNTAX;
                break;
            }
            if(j == 0 || n > 20)
                n = 20;            /* 20 minutes default */
            n *= 12;
            ata_standby_timer = (BYTE) n;
            break;
        case 3:
            ata_global_flags |= ATA_GFLAG_PIO_MODE;
            j = ata_fetch_number(&args, &n);
            if( j > 1 )
            {
                ata_max_pio_mode = 0;
                result = ATA_ESYNTAX;
                break;
            }
            if(j == 0 || n > 4)
                n = 0;            /* PIO mode 0 default */
            ata_max_pio_mode = n;
            break;
        case 4:
            j = ata_fetch_number(&args, &n);
            if( j > 3 || n > 255 )
            {
                result = ATA_ESYNTAX;
                break;
            }
            if(j == 0)
                break;
            if(n == 1)
                n = 0;
            ata_global_flags |= ATA_GFLAG_BLOCK_MODE;
            ata_block_mode = n;
            break;
        }

    }
    return result;
}


int ata_init(process_info_t *p)
{
    int res;
    void disk_add(struct ata_device *d, char *name);

    ata_ref_counter = 0;
    ata_global_flags = 0;
    ata_args = p->args;
    res = ata_parse_args(ata_args);
    if(res<0)
    {
        fd32_message("WARNING: Syntax error in argument(s)\n");
        ata_wait(5*1000*1000);
    }

    fd32_message("ATA Driver Initialization\n");
    res = ata_detect(iface, disk_add);
    fd32_message("%d devices detected on interface 0\n", res);
    res = ata_detect(iface + 1, disk_add);
    fd32_message("%d devices detected on interface 1\n", res);
    return 0;
}

