ifndef FDOS 
FDOS = ../..
endif
ifndef OSLIB
OSLIB = ../../../oslib
endif

include $(FDOS)/config.mk

INCL   = $(OSLIB)
LIB_PATH    = $(OSLIB)/lib/
LIB_DIR  = $(OSLIB)\lib

C_OPT += -I$(FDOS)/include -I ../include/fd32
ASM_OPT +=  -I$(INCL)
LINK_OPT = -Bstatic

OBJS = init.o

all: consdrv.o

install: all
	$(CP) consdrv.o ..

clean :
	$(RM) *.o

consdrv.o: $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -r -s -o consdrv.o

# Common rules
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
%.s : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -S $<
%.o : %.s
	$(CC) $(ASM_OPT) -c $<

