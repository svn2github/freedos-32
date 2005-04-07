ifndef FD32
FD32 = ../fd32
endif
ifndef OSLIB
OSLIB = ../oslib
endif
ifndef NEWLIBC
NEWLIBC = ../newlib
endif

include config.mk

OBJS     = execve.o exit.o io.o isatty.o sbrk.o stubs.o unlink.o time.o
C_OPT +=  -I$(NEWLIBC)/include -I$(FD32)/include -I$(OSLIB) -I./include
LINK_OPT = -Bstatic
VPATH = src
.PHONY: all clean

all: crt0.o libfd32.a

libfd32.a : $(OBJS)
	$(AR) cru libfd32.a $(OBJS)
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<

clean :
	$(RM) *.o
	$(RM) *.a
