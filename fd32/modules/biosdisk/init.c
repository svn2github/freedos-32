/**************************************************************************
 * FreeDOS 32 BIOSDisk Driver                                             *
 * Disk drive support via BIOS                                            *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2003, Salvatore Isaja                               *
 *                                                                        *
 * This is "init.c" - Initialization code for the FreeDOS32 kernel        *
 *                    by Luca Abeni                                       *
 *                                                                        *
 *                                                                        *
 * This file is part of the FreeDOS32 BIOSDisk Driver.                    *
 *                                                                        *
 * The FreeDOS32 BIOSDisk Driver is free software; you can redistribute   *
 * it and/or modify it under the terms of the GNU General Public License  *
 * as published by the Free Software Foundation; either version 2 of the  *
 * License, or (at your option) any later version.                        *
 *                                                                        *
 * The FreeDOS32 BIOSDisk Driver is distributed in the hope that it will  *
 * be useful, but WITHOUT ANY WARRANTY; without even the implied warranty *
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with the FreeDOS32 BIOSDisk Driver; see the file COPYING;        *
 * if not, write to the Free Software Foundation, Inc.,                   *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA                *
 **************************************************************************/

#include <ll/i386/hw-func.h>
#include <ll/i386/pic.h>
#include <ll/i386/x-bios.h>
#include <ll/i386/error.h>
#include "biosdisk.h"

#define VM86_STACK_SIZE 10000

extern void biosdisk_timer(void *p);
extern void biosdisk_reflect(unsigned intnum, BYTE dummy);

void biosdisk_init(struct process_info *p)
{
    char *c, *cmdline;
    int done;
    int want_hd, want_fd;
    DWORD buffer;
    
    message("Initing BIOSDisk...\n");

    /* Parse the command line */
    cmdline = args_get(p);
    c = cmdline;
    done = 0;
    want_fd = 1; want_hd = 1;
    while (!done) {
        if (*c == 0) {
            done = 1;
	} else {
            if ((*c =='-') && (*(c + 1) == '-')) {
                if (strcmp(c + 2, "nofloppy") == 0) {
                    want_fd = 0;
		    message("Floppy Disabled\n");
	        }
                if (strcmp(c + 2, "nohd") == 0) {
                    want_hd = 0;
		    message("HD Disabled\n");
	        }
	    }
	    c++;
	}
    }
    buffer = dosmem_get(VM86_STACK_SIZE);
    vm86_init((LIN_ADDR)buffer, VM86_STACK_SIZE);

    if (want_fd) {
        /* In some bioses, INT 0x13 calls INT 0x40 */
        l1_int_bind(0x40, biosdisk_reflect);

	/* Set handler for the floppy interrupt, and enable it... */
        l1_irq_bind(6, biosdisk_reflect);
        irq_unmask(6);
    }
    if (want_hd) {
	/* Set handlers for the hd interrupts, and enable them... */
        l1_irq_bind(15, biosdisk_reflect);
        l1_irq_bind(14, biosdisk_reflect);
        irq_unmask(14);
        irq_unmask(15);
    }
    /* Are these needed for both floppies and hard disks? */
    l1_int_bind(0x15, biosdisk_reflect);
    l1_int_bind(0x1C, biosdisk_reflect);

    /* This emulates the RM PIT interrupt, for the BIOS */
    fd32_event_post(18200, biosdisk_timer, NULL);

    biosdisk_detect(want_fd, want_hd);
    
    message("BIOSDisk initialized.\n");
}
