/*********************************************************************
 * FreeDOS 32 log file writer version 1.1                            *
 * Performs a raw copy from a physical memory buffer to a file.      *
 * by Salvo Isaja                                                    *
 *                                                                   *
 * Copyright (C) 2001-2003, Salvatore Isaja                          *
 *                                                                   *
 * This program is free software; you can redistribute it and/or     *
 * modify it under the terms of the GNU General Public License       *
 * as published by the Free Software Foundation; either version 2    *
 * of the License, or (at your option) any later version.            *
 *                                                                   *
 * This program is distributed in the hope that it will be useful,   *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of    *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the     *
 * GNU General Public License for more details.                      *
 *                                                                   *
 * You should have received a copy of the GNU General Public License *
 * along with this program; see the file COPYING.txt;                *
 * if not, write to the Free Software Foundation, Inc.,              *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA           *
 *********************************************************************/

/******************************************************************
 * This DPMI program is designed to be compiled with DJGPP.       *
 * DJGPP is a free, 32-bit, DPMI compliant, compiler for DOS.     *
 * You can get DJGPP at http://www.delorie.com/djgpp              *
 * See the Makefile for informations on how to build the program. *
 ******************************************************************/
#include <dpmi.h>
#include <sys/movedata.h>
#include <sys/segments.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    FILE          *f;
    char          *buf;
    unsigned long  addr, size;
    int            sel;
    __dpmi_meminfo mi;

    printf("FreeDOS32 log file writer version 1.1\n");
    if (argc < 3)
    {
        printf("Usage: FD32LOG <absolute buffer address> <buffer size in bytes>\n\n");
        printf("Example. Let's suppose FD32 printed this when quitting:\n");
        printf("           Log buffer allocated at 2E0000h (65536 bytes)\n");
        printf("           Current position is 2E050Bh, 1291 bytes written\n");
        printf("         Then to dump the log buffer to the fd32.log file use:\n");
        printf("           fd32log 0x2E0000 1291\n\n");
        printf("See README.txt for more informations, and COPYING.txt for licensing issues.\n");
        return -1;
    }
    addr = strtol(argv[1], NULL, 0);
    size = strtol(argv[2], NULL, 0);

    mi.handle  = 0;
    mi.size    = size;
    mi.address = addr;
    if (__dpmi_physical_address_mapping(&mi) < 0)
    {
        printf("Unable to map log buffer to physical memory. Aborted.\n");
        return -1;
    }
    __dpmi_lock_linear_region(&mi);
    if ((sel = __dpmi_allocate_ldt_descriptors(1)) < 0)
    {
        printf("Unable to allocate an LDT descriptor for the log buffer. Aborted.\n");
        return -1;
    }
    if (__dpmi_set_segment_base_address(sel, mi.address) < 0)
    {
        printf("Unable to set base address for the log buffer's selector. Aborted.\n");
        __dpmi_free_ldt_descriptor(sel);
        return -1;
    }
    if (__dpmi_set_segment_limit(sel, mi.size - 1) < 0)
    {
        printf("Unable to set segment limit for log buffer's selector. Aborted.\n");
        __dpmi_free_ldt_descriptor(sel);
        return -1;
    }
    buf = (char *) malloc(size);
    if (!buf)
    {
        printf("Unable to allocate a user-space buffer to copy the log buffer. Aborted.\n");
        __dpmi_free_ldt_descriptor(sel);
        return -1;
    }

    printf("Writing fd32.log from data at address %lXh (%lu bytes)...\n", addr, size);
    movedata(sel, 0, _my_ds(), (unsigned) buf, size);
    f = fopen("fd32.log","wb");
    if (f)
    {
        if (fwrite(buf, size, 1, f) < 1)
            printf("Error while writing log buffer in the fd32.log file.\n");
        fclose(f);
        printf("Done.");
    }
    else printf("Unable to create the fd32.log file. Aborted.\n");
    free(buf);
    __dpmi_free_ldt_descriptor(sel);
    return 0;
}
