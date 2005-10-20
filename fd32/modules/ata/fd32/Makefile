ifndef FD32
FD32 = ../../..
endif
ifndef OSLIB
OSLIB = ../../../../oslib
endif
ifndef MODULES
MODULES = ../..
endif

include $(FD32)/config.mk

VPATH = ../src
OBJS     = init.o detect.o partscan.o ata-ops.o interface.o irq.o ata-wait.o
C_OPT   += -I. -I$(FD32)/include -I$(OSLIB) -I$(MODULES)/include/fd32 -I$(MODULES) -I../include -D__ATA__
#C_OPT   = -Wall -O3 -fno-builtin -nostdinc -D__GNU__ -I. -I$(FD32)/include -I$(OSLIB) -I../../include/fd32 -I../include -D__ATA__
LINK_OPT = -Bstatic

.PHONY: all clean

all: atadrv.o
	$(CP) atadrv.o ..

atadrv.o: $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -r -s -o $@

clean :
	$(RM) *.o
