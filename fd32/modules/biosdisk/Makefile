ifndef FDOS 
FDOS = ../../fd32
endif
ifndef OSLIB
OSLIB = ../../oslib
endif

include $(FDOS)/config.mk

INCL   = $(OSLIB)
LIB_PATH    = $(OSLIB)/lib/
LIB_DIR  = $(OSLIB)\lib

C_OPT += -I$(FDOS)/include -I ../include/fd32
ASM_OPT +=  -I$(INCL)
LINK_OPT = -Bstatic


OBJS = common.o detect.o extdisk.o partscan.o request.o stddisk.o\
	reflect.o init.o

all: biosdisk.o

clean:
	$(RM) *.o

biosdisk.o: $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -r -s -o biosdisk.o

# Common rules
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
%.s : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -S $<
%.o : %.s
	$(CC) $(ASM_OPT) -c $<

