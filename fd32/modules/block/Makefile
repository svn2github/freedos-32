# FreeDOS-32 Block Device Manager
# Makefile for the FreeDOS-32 target

ifndef FD32
FD32 = ../..
endif
ifndef OSLIB
OSLIB = ../../../oslib
endif
ifndef PORTAB
PORTAB = ../include/fd32
endif

include $(FD32)/config.mk

INCL     = $(OSLIB)
LIB_PATH = $(OSLIB)/lib/
LIB_DIR  = $(OSLIB)\lib

C_OPT +=  -I$(FD32)/include -I$(OSLIB) -I$(PORTAB) -Wall -O3
LINK_OPT = -Bstatic -r -s

OBJS = block.o
MODS = block.o

all: $(MODS)

clean:
	$(RM) *.o

# Module with single object, Common compile and link
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) -c $< $(LINK_OPT) -o tmp.o
	$(LD) $(LINK_OPT) tmp.o -r -s -o $@
	$(RM) tmp.o
