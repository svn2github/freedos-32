ifndef OSLIB
OSLIB = ../../oslib
endif

include $(OSLIB)/config.mk

INCL   = $(OSLIB)
LIB_PATH    = $(OSLIB)/lib/
LIB_DIR  = $(OSLIB)\lib

C_OPT +=  -I../include $(DEBUG)
LINK_OPT = -Bstatic

OBJS =  drives.o fs.o jft.o truename.o

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

