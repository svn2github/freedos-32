Some short notes about how to compile native applications for FD/32

I am currently using newlib (version 1.13.0), and I've been able to
compile some simple applications with a DJGPP crosscompiler for Linux.

I also tried to compile native applications in ELF format, but dynalink
has problems with the resulting files (I still have to understand why).
The architecture name to use as target for ELF applications would be
i586-pc-elf.

libfd32 is very incomplete... If you get unresolved symbols when loading
the application, you will probably need to define them in libfd32. In that
case, please ask me (lucabe72@email.it).


My crosscompiler is for architecture i586-pc-msdosdjgpp (so, cc is
i586-pc-msdosdjgpp-gcc)

1) tar xvzf newlib-1.13.0.tar.gz
2) mkdir Build; cd Build
3) ../newlib-1.13.0/configure --target=i586-pc-msdosdjgpp --prefix=<install dir>
4) make; make install
5) get libfd32, and configure it (cp mk/djgpp.mk config.mk)
6) Copy your djgpp include files into libfd32, and patch them...
   For example, I use
	cp -r /home/luca/devel/Cross/local/i586-pc-msdosdjgpp/include include
	cd include
	patch -p0 < ../include.diff
	cd ..
   (the patch might not be needed...)
7) build libfd32:
	OSLIB=<oslib dir> FD32=<fd32 dir> NEWLIBC=<install dir> make
8) and install it:
	cp crt0.o <install dir>/i586-pc-msdosdjgpp/lib
	cp libfd32.a <install dir>/i586-pc-msdosdjgpp/lib


Now, you are ready to build native applications...

To do so, your CFLAGS must include
-fno-builtin -nostdinc -I<libfd32 dir>/include -I<install dir>/i586-pc-msdosdjgpp/include
and the final linking step must be
i586-pc-msdosdjgpp-ld <install dir>/i586-pc-msdosdjgpp/lib/crt0.o <obj files> -L <install dir>/i586-pc-msdosdjgpp/lib -lc -lfd32 -r -o <program name>
Example:
586-pc-msdosdjgpp-gcc -Wall -O -fno-builtin -nostdinc -I /home/luca/kernel/tmp/libfd32/include -I /home/luca/kernel/tmp/i586-pc-msdosdjgpp/include -c hello.c
i586-pc-msdosdjgpp-ld /home/luca/kernel/tmp/i586-pc-msdosdjgpp/lib/crt0.o hello.o -L /home/luca/kernel/tmp/i586-pc-msdosdjgpp/lib -lc -lfd32 -r -o hello.com
