-----------------------------------------------------------------------------
FreeDOS-32 ISO 9660 Driver
by Salvo Isaja, 2005-12-04
-----------------------------------------------------------------------------
This is the README file of the preliminary ISO 9660 file system driver
for FreeDOS-32. ISO 9660 is the standard file system for CD-ROMs.

The current implementation is almost as simple as it can be. Files are
expected to be contiguous (not multi-extent), and recorded in non-interleaved
mode. Extensions such as Joliet and Rock Ridge (basically, long file names)
are not yet supported. There is no attempt to access multisession data.
Though, the driver is supposed to read most CD-ROMs.
File names are truncated to 8.3 for short file names APIs and version numbers
are stripped from file names.


COPYRIGHT NOTICE AND LICENSING TERMS
-----------------------------------------------------------------------------
The FreeDOS-32 ISO 9660 Driver version 0.2
Copyright (C) 2005  Salvatore ISAJA

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file GPL.txt; if not, write to
the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


CONTACT INFORMATIONS
-----------------------------------------------------------------------------
To contact the author send e-mail to: salvois <at> users.sourceforge.net
Please send bug reports or suggestions either to the above e-mail address,
or to the FreeDOS-32 developers' mailing list, the mailing list info page is:
   http://freedos-32.sourceforge.net/mailinglists.php
or using the bug tracker in the FreeDOS-32 SourceForge.net project page at:
   http://sourceforge.net/tracker/?atid=113749&group_id=13749&func=browse


HOW TO COMPILE THE DRIVER
-----------------------------------------------------------------------------
A Makefile for GNU tools is provided.
The driver has been developed and tested under Debian GNU/Linux.
* Typing "make -f Makefile.linux" the following binary will be created:
  "iso9660", a test application using the file system.
  Please *have* a look at "lintest.c" before trying the program, as most
  things are hard-coded.
* Typing "make" will generate the "iso9660.o" object file to be loaded as
  a module in FreeDOS-32. Load it after "cdrom.o".
Debugging output *is* enabled by default (may slow down the driver).

--EOF--
