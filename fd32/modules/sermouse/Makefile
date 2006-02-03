# The FreeDOS32 Serial Mouse Driver (sermouse.o)
# Makefile for the FD32 target
# by Salvo Isaja
# --
# See README.txt for informations, and COPYING.txt licensing terms.

ifndef FD32
FD32 = ../..
endif
ifndef OSLIB
OSLIB = ../../../oslib
endif
ifndef PORTAB
PORTAB = ../include/fd32
endif

include $(FD32)/config.mk

OBJS     = temp.o
C_OPT    = -Wall -O3 -s -fno-builtin -nostdinc -D__GNU__ -I. -I.. -I../include -I$(FD32)/include -I$(OSLIB) -I$(PORTAB)
LINK_OPT = -Bstatic

.PHONY: all clean

all: sermouse.o

temp.o: sermouse.c
	$(CC) -c $(C_OPT) sermouse.c -o $@

sermouse.o: $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -r -s -o $@

clean :
	del *.o

