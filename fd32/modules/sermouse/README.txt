The FreeDOS 32 Serial Mouse Driver
by Salvo Isaja

The driver is released under the terms of the GNU General Public License.
The license is in the file COPYING.txt.

The driver supports serial mice using the following protocols:
- Standard Microsoft, 2 buttons
- Extended Microsoft/Logitech, 3 buttons (not tested!)
- Extended Microsoft, 3 buttons + mouse wheel (not tested!)
- Mouse System (not always autotedectable)

Use "make sermouse.o" to build the serial mouse driver.
Use "make moustest.o" to build a simple test program to load as a module.
Use "make" (without arguments) or "make all" to build both.
Use "make clean" to remove rebuildable files.

Load "sermouse.o" as a module in FD32 to enable serial mouse support.
Load "moustest.o" after "sermouse.o".

NOTE: At present the driver supports only ONE mouse, connected
      to port 3F8h, IRQ 4 (this is almost always COM1 under DOS).
