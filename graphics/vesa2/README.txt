The FreeDOS32 VESA2 Graphics Library
by Salvo Isaja
_______________________________________________________________________________

Copyright (C) 2003, Salvatore Isaja, all rights reserved.
Released under the terms of the GNU General Public License (GNU GPL).
See the comments at the beginning of source files and the COPYING.txt
file for further informations.
_______________________________________________________________________________

A simple but effective graphics library for high-res 32-bit color modes.

It features:
- Mode switching to any resolution using VESA2 Bios Extensions
- Video memory access through VESA2 Linear Frame Buffer
- Only 32-bit per pixel modes are used, in order to allow transparency effects
- Drawing filled boxes
- Drawing filled boxes with transparency
- Copying rectangular regions between linear frame buffers
- Rendering text strings using bitmap fonts in PCF format (of X-Window System)
- Displaying bitmap images with transparency
- Capturing bitmap images from a linear frame buffer
- Clipping draw operations to a specified rectangle

It does not feature:
- Any kind of hardware acceleration (that is graphics-card dependent)
- Support for 8- (indexed), 15-, 16- or 24-bit color modes
- Drawing primitives such as lines, circles, etc.

Important note:
At present the FD32 VESA2 Graphics Library can be targetted only for DOS,
using DJGPP libraries. In order to make it usable in FD32, only the mode
switching code (that uses DPMI to interface with the BIOS) has to be changed.

Dependencies:
In order to use all services of the FD32 VESA2 Graphics Library, the following
libraries are to be linked with graphics applications:
- PCF font library (pcffont.c and pcffont.h)
- BMP image library (bmpimage.c and bmpimage.h)
They allow to load X-Window little-endian PCF fonts and Windows 32-bit BMP
images in data structures handled by the FD32 VESA2 Graphics Library.

Building the library:
To compile for the DOS target, use the Makefile in the vesa2 directory.
Use "make libvesa2.a" to build the library to be linked with applications.
Use "make" or "make all" to build the library and two test programs:
- test.exe displays something in 1024x768x32bpp mode and exits
- profile.exe has code profiling enable to let you check for bottle-necks
Use "make clean" to delete rebuildable files.

If you have an MMX processor, you can define the __MMX__ constant in order
to take advantage of parallel computing of transparency components.


End of the README.txt file.