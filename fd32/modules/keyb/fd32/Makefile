# Standard library for X/COFF applications
# Makefile for GNU MAKE & GCC 2.8.0

ifndef FD32
FD32 = ../../../fd32
endif

BASE = $(FD32)/oslib

include $(BASE)/config.mk

INCL   = $(BASE)
LIB_PATH    = $(BASE)/lib/
LIB_DIR  = $(BASE)\lib

C_OPT += -I$(INCL) -I$(FD32)/include -I../include -I../../include/fd32
ASM_OPT +=  -I$(INCL)
LINK_OPT = -Bstatic

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

