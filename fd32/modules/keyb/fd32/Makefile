ifndef OS 
OS = ../../../fd32
endif
ifndef OSLIB
OSLIB = ../../../oslib
endif

include $(OS)/config.mk

INCL   = $(OSLIB)
LIB_PATH    = $(OSLIB)/lib/
LIB_DIR  = $(OSLIB)\lib

C_OPT += -I$(OS)/include -I ../../include/fd32 -I ../include
ASM_OPT +=  -I$(INCL)
LINK_OPT = -Bstatic


#C_OPT += -I$(INCL) -I$(FD32)/include -I../include -I../../include/fd32

VPATH = ../src

OBJS = keyb.o decode.o queues.o init.o

all: keybdrv.o
	$(CP) keybdrv.o ..

clean :
	$(RM) *.o

keybdrv.o: $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -r -o keybdrv.o

# Common rules
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
%.s : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -S $<
%.o : %.s
	$(CC) $(ASM_OPT) -c $<

