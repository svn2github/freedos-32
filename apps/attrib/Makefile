ifndef FDOS 
FDOS = ../../fd32
endif
ifndef OSLIB
OSLIB = ../../oslib
endif
ifndef DRIVERS
DRIVERS = ../../drivers
endif


INCL   = $(OSLIB)

C_OPT += -I $(OSLIB) -I $(DRIVERS)/include/fd32 -I ./ -I $(FDOS)/include -Wall -Wsystem-headers -Os -fno-builtin -nostdinc -D__LINUX__ 

OBJS =  attrib.o dirsrch.o

.PHONY: all clean
        
all: attrib2.com

clean:
	$(RM) *.o
	$(RM) *.com
        

attrib2.com: $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -r -s -o attrib2.com

# Common rules
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
