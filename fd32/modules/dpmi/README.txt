FreeDOS-32 DPMI module (also has builtin DOS execution)

FEATURES
 * Support DOS execution in 3 ways: direct/vm86/wrapper
 *# direct mode can only run a djgpp dos-32 program in a fixed memory region (this is set when the program was linked)
 *# wrapper mode can run a djgpp dos-32 program any linked memory address
 *# vm86 mode can run multiple dos-32 programs in a serial way (one by one), it's planned to run most of the dos programs but still not stable
 * 16-bit BIOSVGA or builtin protected-mode VGA services

USAGE
dpmi.o [--dos-exec=<direct|wrapper|vm86>] [--nolfn] [--biosvga] or
dpmi.o [-X<direct|wrapper|vm86>] [--nolfn] [--biosvga]

The default exec mode is `direct' but `vm86' is recommended because you will have much more DOS compatibilities, and `nolfn' means disable the long-file-name support, `biosvga' means enable the use int10h service offered by the vender delivered vgabios and do *not* use the builtin vga services of FD32.

NOTE
option `biosvga' only available when compiled with macro `ENABLE_BIOSVGA' defined.
