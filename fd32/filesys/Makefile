ifndef OSLIB
OSLIB = ../../oslib
endif
ifndef DRIVERS
DRIVERS = ../../drivers
endif

include $(OSLIB)/config.mk

INCL   = $(OSLIB)
LIB_PATH    = $(OSLIB)/lib/
LIB_DIR  = $(OSLIB)\lib

C_OPT +=  -I../include $(DEBUG) -I$(DRIVERS)/include/fd32
LINK_OPT = -Bstatic

OBJS =  dir.o drives.o fs.o find.o jft.o names.o truename.o wildcard.o

all: filesys.o

clean:
	$(RM) *.o

filesys.o: $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -r -s -o filesys.o

# Common rules
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
%.s : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -S $<
%.o : %.s
	$(CC) $(ASM_OPT) -c $<

