# Standard library for X/COFF applications
# Makefile for GNU MAKE & GCC 2.8.0

ifndef FD32
FD32 = ../../fd32
endif

BASE = $(FD32)/oslib

include $(BASE)/config.mk

INCL   = $(BASE)
LIB_PATH    = $(BASE)/lib/
LIB_DIR  = $(BASE)\lib

C_OPT += -I$(FD32)/include -I../include/fd32
ASM_OPT +=  -I$(INCL)
LINK_OPT = -Bstatic

OBJS = common.o detect.o extdisk.o partscan.o request.o stddisk.o\
	hand.o reflect.o init.o

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

