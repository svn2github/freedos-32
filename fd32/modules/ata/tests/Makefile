ifndef FD32
FD32 = ../../../fd32
endif
ifndef OSLIB
OSLIB = ../../../oslib
endif

include $(FD32)/config.mk

VPATH = ../src
OBJS1     = ata_test.o 
OBJS2     = pck-cmd.o fd32.o
OBJS3     = cd-test.o 
C_OPT    = -Wall -Os -fno-builtin -nostdinc -D__GNU__ -I. -I$(FD32)/include -I$(OSLIB) -I../../include/fd32 -I../include
LINK_OPT = -Bstatic

.PHONY: all clean

all: atatest.com cdrom.o cdtest.com
	$(CP) atatest.com ..
	$(CP) cdtest.com ..
	$(CP) cdrom.o ..

atatest.com: $(OBJS1)
	$(LD) $(LINK_OPT) $(OBJS1) -r -s -o atatest.com

cdtest.com: $(OBJS3)
	$(LD) $(LINK_OPT) $(OBJS3) -r -s -o cdtest.com

cdrom.o: $(OBJS2)
	$(LD) $(LINK_OPT) $(OBJS2) -r -s -o cdrom.o

clean :
	$(RM) *.o
	$(RM) *.com
