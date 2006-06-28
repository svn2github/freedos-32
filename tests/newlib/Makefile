# Project: newlib tests

CPP  = g++
CC   = gcc
RES  = 
OBJ  = 
LIBS = -lc -lm -lfd32
LFLAGS = -Bstatic -L"../../apps/newlib/lib"
INCS = -I"../../apps/newlib/include"
CFLAGS = $(INCS) -nostdinc -mno-stack-arg-probe -finline-functions -ffreestanding -fexpensive-optimizations -O3

.PHONY: all all-before all-after clean clean-custom

all: all-before hello.com all-after


clean: clean-custom
	rm -f *.com


# Common rules
%.com : %.o
	$(LD) $(LFLAGS) ../../apps/newlib/lib/crt0.o $< $(LIBS) -r -s -o $@
%.o : %.c
	$(CC) $(CFLAGS) $(C_OUTPUT) -c $<
%.s : %.c
	$(CC) $(CFLAGS) $(C_OUTPUT) -S $<
