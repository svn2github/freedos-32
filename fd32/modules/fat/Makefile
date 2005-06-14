ifndef KERNEL
KERNEL = ../../fd32
endif
ifndef OSLIB
OSLIB = ../../oslib
endif
ifndef DRIVERS
DRIVERS = ..
endif
ifndef PORTAB
PORTAB = ../include/fd32
endif

include $(KERNEL)/config.mk

INCL = $(OSLIB)

C_OPT   += -I$(KERNEL)/include -I$(DRIVERS) -I$(PORTAB)
LINK_OPT = -Bstatic

OBJS = alloc.o dir.o dos.o file.o open.o volume.o fd32.o
BIN  = fat2.o

.PHONY: all clean

all: $(BIN)

clean:
	$(RM) *.o

$(BIN): $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -r -s -o $(BIN)

%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
