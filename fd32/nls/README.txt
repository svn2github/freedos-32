NATIONAL CODE-PAGE SUPPORT
by Salvo Isaja

Although the FD32's goal is to use Unicode to encode all strings,
things like the short file names in the FAT file system needs to
be encoded in national code pages.
At present only the American IBM code page 437 is available.

HOW TO COMPILE FOR FREEDOS 32
Use the makefile "Makefile".
From the command line, use "make" to generate the object files
to be linked (for the moment) with the FD32 kernel.

HOW TO COMPILE FOR THE DOS TARGET
Use the makefile "Makefile.dj".
Use "make -f Makefile.dj" to generate the object files targetted
for the DJGPP libraries.

DEPENDENCIES
unicode.o
The cp437.o driver uses the unicode.o driver for Unicode support.
