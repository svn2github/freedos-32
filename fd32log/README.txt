FreeDOS32 log file writer version 1.1
Performs a raw copy from a physical memory buffer to a file.
by Salvo Isaja

-------------------------------------------------------------------
    USAGE
-------------------------------------------------------------------
The FD32 kernel can store debugging output in a physical memory
buffer. You can use the log file writer to dump the content of
that memory buffer in a text file, called fd32.log
This program has been developed to dump the FD32 log buffer, but
can be used to dump any physical memory content to a file.

Start FD32 using the X eXtender (for example, using the GOFD32.BAT
batch file provided in the binary release). At the end of your FD32
session, you can type "exit" at the command prompt to quit FD32.
Right before quitting, a message like the following is displayed:
    Log buffer allocated at 2E0000h (65536 bytes)
    Current position is 2E050Bh, 1291 bytes written

This tells you where the log buffer is allocated in physical memory,
and how much bytes have been written in it.

To dump its content in the fd32.log file, from the DOS command
prompt (regular DOS, after quitting FD32), type:
    fd32log <absolute buffer address> <buffer size in bytes>
For example, if you got the above example message, type:
    fd32log 0x2E0000 1291

Note that if, when quitting FD32, you get a message like:
    FD32 debug logger configured for Bochs. No log buffer allocated.
You cannot use the log file writer, because the FD32 debugging
output is not stored in a physical memory buffer, but is sent to the
console window of the Bochs emulator (http://bochs.sourceforge.net)
to take advantage of its real-time debugging capabilities.
You have to recompile the FD32 kernel to change this behaviour,
commenting out the following line from the file fd32/kernel/logger.c:
    #define __BOCHS_DBG__

Being the FD32 log file writer a DPMI program, a DPMI server is
needed to run it. The free 32-bit DPMI server CWSDPMI is included.
You can get the source code of CWSDPMI at http://www.delorie.com/djgpp
If you get a message like the following when running the program:
    Warning: Could not create C:\CWSDPMI.SWP
don't worry, the log file writer will work without problems.


-------------------------------------------------------------------
    COMPILATION
-------------------------------------------------------------------
The source code for the FD32 log file writer is included.
It is a DPMI program, designed to be compiled with the DJGPP compiler
under DOS. DJGPP is a free, 32-bit, DPMI compliant, compiler.
You can get DJGPP for free at http://www.delorie.com/djgpp
See http://freedos-32.sourceforge.net/dosbuild.html for more
informations on how to get and set up DJGPP.
From the command line type:
    make
(without arguments) to build fd32log.exe from fd32log.c.
See the Makefile internals for the details.


-------------------------------------------------------------------
    COPYRIGHT AND LICENSE
-------------------------------------------------------------------
FreeDOS32 log file writer version 1.1
Copyright (C) 2001-2003, Salvatore Isaja

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING.txt;
if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA  02111-1307  USA