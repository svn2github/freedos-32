# Standard library for X/COFF applications
# Makefile for GNU MAKE & GCC 2.8.0

ifndef FD32
FD32 = ..
endif
ifndef DRIVERS
DRIVERS = ../../drivers
endif

BASE = $(FD32)/oslib

include $(BASE)/config.mk

INCL   = $(BASE)
LIB_PATH    = $(BASE)/lib/
LIB_DIR  = $(BASE)\lib

C_OPT += -I$(FD32)/include -I$(DRIVERS)/include/fd32
ASM_OPT +=  -I$(INCL)
LINK_OPT = -Bstatic

OBJS =  case.o convers.o utf8.o utf16.o

all: unicode.o

clean:
	$(RM) *.o

unicode.o: $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -r -s -o unicode.o

# Common rules
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
%.s : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -S $<
%.o : %.s
	$(CC) $(ASM_OPT) -c $<

