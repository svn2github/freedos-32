# FreeDOS-32 Unicode Support Library 2.0
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

OBJS   = simple_fold.o utf16towc.o utf8towc.o wctoutf16.o wctoutf8.o
OUTLIB = unicode.o

all: $(OUTLIB)

clean:
	$(RM) $(OBJS) $(OUTLIB)

$(OUTLIB): $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -o $(OUTLIB)

%.o : %.c
	$(CC) $(C_OPT) -c $<
