# Project: modtest
# Makefile created by Dev-C++ 4.9.9.1

CPP  = g++.exe
CC   = gcc.exe
WINDRES = windres.exe
RES  = 
OBJ  = 
LIBS =  -Bstatic -nostdlib
INCS =  -I"../../oslib" -I"../../fd32/include" -I"../../fd32/modules/include/fd32"
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
