/**************************************************************************
 * FreeDOS 32 BIOSDisk Driver                                             *
 * Disk drive support via BIOS                                            *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2002, Salvatore Isaja                               *
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


void biosdisk_init(void)
{
  extern void int0x15(void); /* Cassette... why? */
  extern void int0x56(void); /* IRQ6 relocated by software - Floppy */
  extern void int0x76(void); /* IRQ14 - Primary IDE controller      */
  extern void int0x77(void); /* IRQ15 - Secondary IDE controller    */

  message("Initing BIOSDisk...\n");

  irq_unmask(14);
  irq_unmask(15);
  irq_unmask(6);

  vm86_init();

  IDT_place(0x56, int0x56);
  IDT_place(0x15, int0x15);

  IDT_place(0x76, int0x76);
  IDT_place(0x77, int0x77);

  biosdisk_detect();
  message("BIOSDisk initialized.\n");
}
