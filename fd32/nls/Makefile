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

OBJS = cp437.o

all: $(OBJS)

clean:
	$(RM) *.o

%.o : %.c
	$(REDIR) $(CC) $(C_OPT) -s -c $<

