ifndef FD32
FD32 = ../newlib-port/sys/fd32
endif
ifndef NEWLIBC
NEWLIBC = ../newlib
endif

include config.mk

OBJS = execve.o exit.o io.o isatty.o sbrk.o stubs.o unlink.o time.o
C_OPT += -I$(NEWLIBC)/include -I$(FD32)/include -I$(FD32)/sys
LINK_OPT = -Bstatic
VPATH = $(FD32)

.PHONY: all clean

all: crt0.o libfd32.a

libfd32.a: $(OBJS)
	$(AR) cru libfd32.a $(OBJS)

clean:
	$(RM) *.o
	$(RM) *.a
