# Project: modtest
# Makefile created by Dev-C++ 4.9.9.1

CPP  = g++.exe
CC   = gcc.exe
WINDRES = windres.exe
RES  = 
OBJ  = modtest.o $(RES)
LINKOBJ  = modtest.o $(RES)
LIBS =  -L"D:/dev-cpp/lib" -Bstatic -r  -nostdlib -s 
INCS =  -I"../oslib"  -I"../fd32/include"  -I"../drivers/include/fd32" 
CXXINCS =  -I"../oslib"  -I"../fd32/include"  -I"../drivers/include/fd32" 
BIN  = modtest.exe
CXXFLAGS = $(CXXINCS)   -fexpensive-optimizations -O3 -nostdlib
CFLAGS = $(INCS) -nostdinc -mno-stack-arg-probe -finline-functions -ffreestanding -D__WIN32__   -fexpensive-optimizations -O3 -nostdlib

.PHONY: all all-before all-after clean clean-custom

all: all-before modtest.exe all-after


clean: clean-custom
	rm -f $(OBJ) $(BIN)

$(BIN): $(OBJ)
	$(CC) $(LINKOBJ) -o "modtest.exe" $(LIBS)

modtest.o: modtest.c ../drivers/include/fd32/dr-env.h   ../oslib/ll/i386/hw-data.h ../oslib/ll/i386/defs.h   ../oslib/ll/i386/sel.h ../oslib/ll/i386/hw-func.h   ../oslib/ll/i386/hw-instr.h ../oslib/ll/i386/hw-io.h   ../oslib/ll/i386/pic.h ../oslib/ll/i386/stdio.h ../oslib/ll/stdarg.h   ../oslib/ll/sys/types.h ../oslib/ll/i386/error.h   ../oslib/ll/i386/cons.h ../oslib/ll/i386/stdlib.h   ../oslib/ll/i386/string.h ../oslib/ll/i386/mem.h ../oslib/ll/ctype.h   ../oslib/ll/sys/ll/event.h ../oslib/ll/sys/ll/time.h   ../oslib/ll/sys/ll/ll-data.h ../drivers/include/fd32/dr-env/bios.h   ../oslib/ll/i386/x-bios.h ../drivers/include/fd32/dr-env/mem.h   ../fd32/include/kmem.h ../drivers/include/fd32/dr-env/datetime.h   ../fd32/include/kernel.h ../fd32/include/logger.h   ../fd32/include/errors.h
	$(CC) -c modtest.c -o modtest.o $(CFLAGS)
