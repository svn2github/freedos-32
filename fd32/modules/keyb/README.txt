FreeDOS-32 keyboard driver


FEATURES
 * Support country/region layout
 * 

USAGE
keybdrv.o [--file-name=<layouts_library_file>] [--layout-name=<US|CA|DK...>] or
keybdrv.o [-F<layouts_library_file>] [-L<US|CA|DK...>]

The default file-name is A:\fd32\keyboard.sys
            layout-name is US


NOTE
The layouts are from FD-KEYB:
http://www.ibiblio.org/pub/micro/pc-stuff/freedos/files/dos/keyb/kblayout/kpdos21x.zip


________________________________________

QUEUE
head - - - - tail
There two cursors, one is the head and the other is tail
Initially, the head = 0, tail = 1, when putting a byte,
the head = 0, tail = 2

at the mean time, after putting a byte to the keyqueue,
bios_da.keyb_bufhead = 1
bios_da.keyb_buftail = 2

TODO
* Support key-(combination)-command, rename the hook.c to cmd.c
* Only handle the raw queue in the keyboard driver?
