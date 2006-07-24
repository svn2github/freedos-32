# FreeDOS-32 Unicode Support Library 2.1
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

OBJS   = init.o simple_fold.o utf16le.o utf16be.o utf8.o
OUTLIB = unicode.o

all: $(OUTLIB)

install: all
	$(CP) $(OUTLIB) ..

clean:
	$(RM) $(OBJS) $(OUTLIB)

$(OUTLIB): $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -o $(OUTLIB)

%.o : %.c
	$(CC) $(C_OPT) -c $<
