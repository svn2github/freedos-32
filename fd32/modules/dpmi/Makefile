# Standard library for X/COFF applications
# Makefile for GNU MAKE & GCC 2.8.0

BASE = ../../oslib
include $(BASE)/config.mk

INCL   = $(BASE)
LIB_PATH    = $(BASE)/lib/
LIB_DIR  = $(BASE)\lib

#DEBUG = -D__DEBUG__

C_OPT += -I$(INCL) -I../../include -I ./include $(DEBUG)
ASM_OPT +=-I$(INCL)
LINK_OPT = -Bstatic

OBJS = handlers.o chandler.o dpmimem.o dpmiexc.o rmint.o ldtmanag.o init.o \
	int31_00.o int31_02.o int31_03.o int31_04.o int31_05.o \
	int31_06.o int31_0e.o int10.o int21.o

VPATH = src

all: dpmi.o

clean :
	$(RM) *.o

dpmi.o: $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -r -o dpmi.o

# Common rules
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
%.s : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -S $<
%.o : %.s
	$(CC) $(ASM_OPT) -c $<

