FD32 FILE SYSTEM LAYER
by Salvo Isaja

The File System Layer is the part of FD32 which provides support
for file system services. It is implemented as a set of functions
that call the appropriate services of the installed file system
drivers or character device drivers. Also, the File System Layer
manages the Job File Table (JFT), mapping open files for each
process in open files in each file system driver installed or
into character devices.

HOW TO COMPILE FOR FREEDOS 32
Use the makefile "Makefile".
From the command line, use "make" to generate the "filesys.o"
object file to be linked to the FD32 kernel.

HOW TO COMPILE FOR THE DOS TARGET
Use the makefile "Makefile.dj".
Use "make -f Makefile.dj" to generate the "filesys.o" object
file targetted for the DJGPP libraries.

DEPENDENCIES
- unicode.o
- devices.o
- cp437.o
The File System Layer uses the Devices Engine (devices.o) for
devices management, the Unicode Support Library (unicode.o) and
the local code page support (only cp437.o at present) to deal
with file name strings.