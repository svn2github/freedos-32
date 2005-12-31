# The FreeDOS32 Floppy Driver
# Makefile for the FD32 target
# by Salvo Isaja
# ----------------------------------------
# - "make" (without arguments), "make all" or "make floppy.o" build
#   the object file to be loaded as FD32 module.
#
# - "make clean" removes rebuildable files.

ifndef FD32
FD32 = ../..
endif
ifndef MODULES
MODULES = ..
endif
ifndef OSLIB
OSLIB = ../../../oslib
endif
ifndef PORTAB
PORTAB = ../include/fd32
endif

include $(FD32)/config.mk

INCL     = $(OSLIB)
OBJS     = fdc.o bootsect.o blockio.o fd32.o
C_OPT   += -I. -I$(FD32)/include -I$(MODULES) -I$(OSLIB) -I$(PORTAB)
LINK_OPT = -Bstatic

.PHONY: all clean

all: floppy.o floppytest.o

fdc.o: fdc.c fdc.h
	$(CC) -c $(C_OPT) fdc.c -o $@

bootsect.o: bootsect.c fdc.h
	$(CC) -c $(C_OPT) bootsect.c -o $@

blockio.o: blockio.c blockio.h fdc.h
	$(CC) -c $(C_OPT) blockio.c -o $@

fd32.o: fd32/fd32.c blockio.h fdc.h
	$(CC) -c $(C_OPT) fd32/fd32.c -o $@

floppy.o: $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -r -s -o $@

floppytest.o: floppytest.c
	$(CC) -c $(C_OPT) floppytest.c -o $@

clean :
	$(RM) *.o
