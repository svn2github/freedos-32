FreeDOS-32 DPMI module (also has builtin DOS execution)

FEATURES
 * Support DOS execution in 3 ways: direct/vm86/wrapper
 *# direct mode can only run a djgpp dos-32 program in a fixed memory region (this is set when the program was linked)
 *# wrapper mode can run a djgpp dos-32 program anywhere
 *# vm86 mode can run multiple dos-32 programs in a serial way (one by one), it's planned to run most of the dos programs but still not stable.

USAGE
dpmi.o [--dos-exec=<direct|wrapper|vm86>] [--nolfn] or
dpmi.o [-X<direct|wrapper|vm86>] [--nolfn]

The default exec mode is `direct', and `nolfn' means disable the long-file-name support

