ifndef FD32
FD32 = ../../../fd32
endif
ifndef OSLIB
OSLIB = ../../../oslib
endif

include $(FD32)/config.mk

VPATH = ../src
OBJS     = init.o detect.o partscan.o ata-ops.o interface.o irq.o ata-wait.o
C_OPT   = -Wall -O3 -fno-builtin -nostdinc -D__GNU__ -I. -I$(FD32)/include -I$(OSLIB) -I../../include/fd32 -I../include -D__ATA__
#C_OPT    = -Wall -O3 -nostdinc -D__GNU__ -I. -I$(FD32)/include -I$(OSLIB) -I../../include/fd32 -I../include
LINK_OPT = -Bstatic

.PHONY: all clean

all: atadrv.o
	$(CP) atadrv.o ..

atadrv.o: $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -r -s -o $@

clean :
	$(RM) *.o
