ifndef FD32
FD32 = ../..
endif
ifndef OSLIB
OSLIB = ../../../oslib
endif

include $(FD32)/config.mk

#VPATH = ../src
OBJS2     = pck-cmd.o fd32.o
OBJS3     = cd-test.o 
C_OPT    = -Wall -O2 -fno-builtin -nostdinc -D__GNU__ -I. -I$(FD32)/include -I$(OSLIB) -I../include/fd32 -I../ata/include
LINK_OPT = -Bstatic

.PHONY: all clean

all: cdrom.o cdtest.com

cdtest.com: $(OBJS3)
	$(LD) $(LINK_OPT) $(OBJS3) -r -s -o cdtest.com

cdrom.o: $(OBJS2)
	$(LD) $(LINK_OPT) $(OBJS2) -r -s -o cdrom.o

clean :
	$(RM) *.o
	$(RM) *.com
