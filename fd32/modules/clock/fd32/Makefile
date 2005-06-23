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

C_OPT += -I$(FDOS)/include -I ../../include/fd32
ASM_OPT +=  -I$(INCL)
LINK_OPT = -Bstatic
VPATH = ../src

OBJS = init.o clock.o

all: clockdrv.o
	$(CP) clockdrv.o ..

clean :
	$(RM) *.o

clockdrv.o: $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -r -o clockdrv.o

# Common rules
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
%.s : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -S $<
%.o : %.s
	$(CC) $(ASM_OPT) -c $<

