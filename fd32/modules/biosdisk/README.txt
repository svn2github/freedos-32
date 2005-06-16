The FreeDOS-32 BIOSDisk disk driver version 0.3
a block device driver using the BIOS disk services.
by Salvo Isaja <salvois at users.sourceforge.net>

BIOSDisk is a simple but effective driver for disk devices.
As its name suggests, it uses the ROM BIOS to access disk devices. This could
sound strange for a 32-bit protected mode operating system, but can be useful
for testing and to reduce the size of the loaded modules. Moreover, BIOSDisk
has been developed before the native driver (such as the Floppy driver and
the ATA driver), hence it was originally the only way to access block devices.
If supported by the ROM BIOS (most do), BIOSDisk can use the IBM/MS Extension
to access media greater than 8.4 GB, with Large Block Addressing (LBA).

The FreeDOS-32 BIOSDisk driver is released under the terms of the GNU General
Public License. A copy of the license is available in the file "GPL.txt" stored
in the "drivers" directory.

_______________________________________________________________________________
How to compile for FreeDOS-32
-----------------------------
Use the makefile "Makefile".
From the command line, use "make" to generate the "biosdisk.o"
object file to be used as FD32 driver.
At present, it must be loaded before any file system driver.
_______________________________________________________________________________
How to compile for the DOS target
---------------------------------
It is possible to compile BIOSDisk as a generic DOS application for testing.
Use "make -f Makefile.dj" to generate the "biosdisk.o" object file targetted
for the DJGPP libraries. This version lacks of the FD32 initialization code
which installs reflection code for VM86 mode. In order to use the DJGPP version
of BIOSDisk, call the "biosdisk_detect" function manually to initialize the
disk support (see detect.c).
_______________________________________________________________________________
Dependencies
------------
- devices.o
If the symbol BIOSDISK_FD32DEV is defined in "biosdisk.h", the Devices Engine
"fd32\devices\devices.o" is needed to register disk devices to the FD32 kernel.
_______________________________________________________________________________
Changes
-------
0.3 - Read and write operations can support an arbitrary number of sectors
      per transfer, and return the number of sectors transferred on success.
      Lots of code clean-up.
0.2 - More BIOSes supported thanks to INT 40h and INT 15h reflection to
      Virtual-8086 mode. More debugging output in the detection routine.
      Code clean-up.
