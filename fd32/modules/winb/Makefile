ifndef FDOS 
FDOS = ../..
endif
ifndef OSLIB
OSLIB = ../../../oslib
endif
ifndef LIBFD32
LIBFD32 = ../../../apps/libfd32
endif

include $(FDOS)/config.mk

INCL   = $(OSLIB)
LIB_PATH = $(OSLIB)/lib/
LIB_DIR  = $(OSLIB)\lib

C_OPT = -Wall -fexpensive-optimizations -O3 -finline-functions -mno-stack-arg-probe -ffreestanding -D__WIN32__ -I$(FDOS)/include -I$(OSLIB)
ASM_OPT +=  -I$(INCL)
LINK_OPT = -Bstatic -L$(LIBFD32)

OBJS = memory.o kernel32.o msvcrt.o init.o

all: winb.o

clean :
	$(RM) *.o

winb.o: $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -lfd32 -r -s -o winb.o

# Common rules
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
%.s : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -S $<
%.o : %.s
	$(CC) $(ASM_OPT) -c $<

