ifndef FDOS 
FDOS = ../../..
endif
ifndef OSLIB
OSLIB = ../../../../oslib
endif

include $(FDOS)/config.mk

INCL   = $(OSLIB)
LIB_PATH    = $(OSLIB)/lib/
LIB_DIR  = $(OSLIB)\lib

C_OPT += -I$(FDOS)/include -I../.. -I../../include/fd32 -I../include
ASM_OPT +=  -I$(INCL)
LINK_OPT = -Bstatic

VPATH = ../src

OBJS = keyb.o decode.o layout.o queues.o hook.o init.o

all: keybdrv.o
	$(CP) keybdrv.o ..

clean :
	$(RM) *.o

keybdrv.o: $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -r -s -o keybdrv.o

# Common rules
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
%.s : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -S $<
%.o : %.s
	$(CC) $(ASM_OPT) -c $<

