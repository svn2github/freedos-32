# Project: modtest
# Makefile created by Dev-C++ 4.9.9.1

CPP  = g++.exe
CC   = gcc.exe
WINDRES = windres.exe
RES  = 
OBJ  = 
LIBS =  -Bstatic -nostdlib
INCS =  -I"../../oslib"  -I"../../fd32/include"  -I"../../drivers/include/fd32" 
CXXINCS =  -I"../../oslib"  -I"../../fd32/include"  -I"../../drivers/include/fd32" 
CXXFLAGS = $(CXXINCS) -fexpensive-optimizations -O3 -nostdlib
CFLAGS = $(INCS) -nostdinc -mno-stack-arg-probe -finline-functions -ffreestanding -fexpensive-optimizations -O3 -nostdlib

.PHONY: all all-before all-after clean clean-custom

all: all-before modtest.com contest.com pcitest.com all-after


clean: clean-custom
	rm -f *.com


# Common rules
%.com : %.o
	$(LD) $(LIBS) $< -r -s -o $@
%.o : %.c
	$(CC) $(CFLAGS) $(C_OUTPUT) -c $<
%.s : %.c
	$(CC) $(CFLAGS) $(C_OUTPUT) -S $<
