BIOSDISK DISK DRIVER version 0.2
by Salvo Isaja

BIOSDisk is a simple but effective driver for disk devices.
As its name suggests, it uses the ROM BIOS to access disk devices.
This could sound strange for a 32-bit protected mode operating
system: BIOSDisk has been developed to have disk support while
waiting for 32-bit disk drivers. If supported by the ROM BIOS
(most BIOSes do), BIOSDisk can use the IBM/MS Extension to access
media greater than 8.4 GB, with Large Block Addressing (LBA).

--------------------------------------------------------------
  HOW TO COMPILE FOR FREEDOS 32
--------------------------------------------------------------
Use the makefile "Makefile".
From the command line, use "make" to generate the "biosdisk.o"
object file to be used as FD32 driver.
At present, it has to be loaded before any file system driver.

--------------------------------------------------------------
  HOW TO COMPILE FOR THE DOS TARGET
--------------------------------------------------------------
Use the makefile "Makefile.dj".
Use "make -f Makefile.dj" to generate the "biosdisk.o" object
file targetted for the DJGPP libraries. This version lacks of
the FD32 initialization code which installs reflection code
for VM86 mode. In order to use the DJGPP version of BIOSDisk,
call the "biosdisk_detect" function manually to initialize the
disk support (see detect.c).

--------------------------------------------------------------
  DEPENDENCIES
--------------------------------------------------------------
- devices.o
If the symbol BIOSDISK_FD32DEV is defined in "biosdisk.h", the
Devices Engine "fd32\devices\devices.o" is needed to register
disk devices to the FD32 kernel.

--------------------------------------------------------------
  CHANGES
--------------------------------------------------------------
0.2 - More BIOSes supported thanks to INT 40h and INT 15h
      reflection to Virtual-8086 mode.
      More debugging output in the detection routine.
      Code clean-up.

--------------------------------------------------------------
  LICENSE AND DISCLAIMER
--------------------------------------------------------------
FreeDOS 32 BIOSDisk Driver
Copyright (C) 2001-2003, Salvatore Isaja

The FreeDOS 32 BIOSDisk Driver is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The FreeDOS 32 BIOSDisk Driver is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with the FreeDOS32 BIOSDisk Driver; see the file COPYING;
if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
