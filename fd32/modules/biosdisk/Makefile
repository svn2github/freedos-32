ifndef KERNEL
KERNEL = ../..
endif
ifndef OSLIB
OSLIB = ../../../oslib
endif
ifndef MODULES
MODULES = ..
endif

include $(KERNEL)/config.mk

INCL   = $(OSLIB)
LIB_PATH    = $(OSLIB)/lib/
LIB_DIR  = $(OSLIB)\lib

C_OPT += -I$(KERNEL)/include -I$(MODULES)/include/fd32 -I$(MODULES) -I. -D__BIOSDISK__
ASM_OPT +=  -I$(INCL)
LINK_OPT = -Bstatic


OBJS = common.o detect.o init.o reflect.o request.o partscan.o

all: biosdisk.o

clean:
	$(RM) *.o

biosdisk.o: $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -r -s -o biosdisk.o

partscan.o: $(MODULES)/shared/partscan/partscan.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<

# Common rules
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
%.s : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -S $<
%.o : %.s
	$(CC) $(ASM_OPT) -c $<
