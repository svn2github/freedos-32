WARNING: THIS IS A SUPERFICIALLY TESTED ALPHA VERSION!! TEST AT YOUR OWN RISK!


The following is the command line options implemented so far:

o "--poll" Polling in stead of interrupts.
o "--disable-write" Disable writing to all ATA devices for safer testing.
o "--standby <n>" Enter standby automatically after n minutes of inactivity.
  Max 20min. "--standby 0" disables the standby timer.
o "--max-pio-mode <n>" Sets maximum PIO mode. n in the range 0 to 4,
  where 4 is fastest. Old ISA bus systems don't handle modes higher than 2.
  The driver only checks what the drive itself supports.
  If this option is omitted, then the setting done by the BIOS is left
  untouched, witch is safest.
o "--block-mode <n>" This limits the number of sectors in block mode (the
  number of sectors per interrupt).
  Without this option, the driver will use block mode with the maximum number
  of sectors that the drive supports.
  There is no guarantee that the drive supports n sector block mode,
  even if the drive supports a mode with more than n sectors. In that case,
  block mode will be disabled.
  Use "--block-mode 0" to disable block mode. If only few sectors are
  transfered per request, then block mode may be slower.


Examples:

The atatest.com program takes the number of MiBs to read as an argument.
High number on old and slow drive will be boring. Small number and new
drive with big buffer (cache) will give misleading numbers. On very old
and slow drives, the maximum sustained transfer rate of the drive will
be limiting. This program should probably use "hda" instead of "hda1",
but you can change that yourself.
