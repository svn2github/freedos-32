# FreeDOS-32 Native Language Support (NLS) 2.0
# Makefile for the FreeDOS-32 target

ifndef FD32
FD32 = ../../fd32
endif
ifndef OSLIB
OSLIB = ../../oslib
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

OBJS = nls.o
MODS = nls.ko

all: $(MODS)

clean:
	$(RM) $(OBJS) $(MODS)

nls.ko: nls.o
	$(LD) $(LINK_OPT) $< -o $@

%.o : %.c
	$(CC) $(C_OPT) -c $<
