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

OBJS = coff.o elf.o common.o

all: dynalink.o

clean :
	$(RM) *.o

dynalink.o: $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -r -o dynalink.o

# Common rules
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
%.s : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -S $<
%.o : %.s
	$(CC) $(ASM_OPT) -c $<

