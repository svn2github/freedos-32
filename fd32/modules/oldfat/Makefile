ifndef KERNEL
KERNEL = ../..
endif
ifndef OSLIB
OSLIB = ../../../oslib
endif
ifndef MODULES
MODULES = ..
endif
ifndef PORTAB
PORTAB = ../include/fd32
endif

include $(KERNEL)/config.mk

INCL   = $(OSLIB)
LIB_PATH    = $(OSLIB)/lib/
LIB_DIR  = $(OSLIB)\lib

C_OPT += -I$(KERNEL)/include -I$(MODULES) -I$(PORTAB)
ASM_OPT +=  -I$(INCL)
LINK_OPT = -Bstatic

OBJS =  attrib.o blockio.o creat.o dir.o fat12.o fat16.o fat32.o fatreq.o\
	lfn.o mount.o open.o readdir.o readwrit.o support.o fatinit.o wildcard.o

all: fat.o

clean:
	$(RM) *.o

fat.o: $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -r -s -o fat.o

# Common rules
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
%.s : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -S $<
%.o : %.s
	$(CC) $(ASM_OPT) -c $<

