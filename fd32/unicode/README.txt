UNICODE SUPPORT LIBRARY
by Salvo Isaja

The character set used by FD32 is Unicode, also known as ISO-10646.
Unicode lets FD32 to deal with all languages in the world providing
a unique mapping for each character. This library provides to FD32
a simple but effective Unicode 3.1 support.
For further informations, read the full documentation on the web:
  http://freedos-32.sourceforge.net/unicode.html

HOW TO COMPILE FOR FREEDOS 32
Use the makefile "Makefile".
From the command line, use "make" to generate the "unicode.o" object
file to be linked with the FD32 kernel.

HOW TO COMPILE FOR THE DOS TARGET
Use the makefile "Makefile.dj".
Use "make -f Makefile.dj" to generate the "unicode.o" object file
targetted for the DJGPP libraries.

DEPENDENCIES
The unicode.o file does not need any external support.