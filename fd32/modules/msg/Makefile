ifndef OSLIB
OSLIB = ../../../oslib
endif

include $(OSLIB)/config.mk

INCL   = $(OSLIB)
LIB_PATH    = $(OSLIB)/lib/
LIB_DIR  = $(OSLIB)\lib

#DEBUG = -D__DEBUG__

C_OPT += -I../../include -I ./include $(DEBUG)
LINK_OPT = -Bstatic

OBJS = init.o sysmsgs.o

VPATH = src

all: msgq.o

clean :
	$(RM) *.o

msgq.o: $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -r -s -o msgq.o

# Common rules
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
%.s : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -S $<
%.o : %.s
	$(CC) $(ASM_OPT) -c $<

