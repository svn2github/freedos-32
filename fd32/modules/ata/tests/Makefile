ifndef FD32
FD32 = ../../../fd32
endif
ifndef OSLIB
OSLIB = ../../../oslib
endif

include $(FD32)/config.mk

VPATH = ../src
OBJS1     = ata_test.o 
C_OPT    = -Wall -Os -fno-builtin -nostdinc -D__GNU__ -I. -I$(FD32)/include -I$(OSLIB) -I../../include/fd32 -I../include
LINK_OPT = -Bstatic

.PHONY: all clean

all: atatest.com
	$(CP) atatest.com ..

atatest.com: $(OBJS1)
	$(LD) $(LINK_OPT) $(OBJS1) -r -s -o atatest.com

clean :
	$(RM) *.o
	$(RM) *.com
