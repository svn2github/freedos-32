# Standard library for X/COFF applications
# Makefile for GNU MAKE & GCC 2.8.0

BASE = ../oslib
include $(BASE)/config.mk

INCL   = $(BASE)
LIB_PATH    = $(BASE)/lib/
LIB_DIR  = $(BASE)\lib

C_OPT += -I$(INCL) -I../include
ASM_OPT += -I$(INCL)
LINK_OPT = -Bstatic

OBJS = call.o pmm.o run.o exch.o abort.o process.o ldtmanag.o filesys.o exec.o \
	 syscalls.o logger.o devices.o

all: kernel.o

clean :
	$(RM) *.o

kernel.o: $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -r -o kernel.o

# Common rules
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
%.s : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -S $<
%.o : %.s
	$(CC) $(ASM_OPT) -c $<

