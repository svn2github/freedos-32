ifndef FDOS 
FDOS = ../..
endif
ifndef OSLIB
OSLIB = ../../../oslib
endif
ifndef LIBFD32
LIBFD32 = ../../../apps/libfd32
endif
ifndef NEWLIBC
NEWLIBC = ../../../apps/newlib
endif

include $(FDOS)/config.mk

INCL   = -I$(FDOS)/include -I$(NEWLIBC)/include -I$(LIBFD32)/include -I.
LIB_PATH = $(OSLIB)/lib/
LIB_DIR  = $(OSLIB)\lib

WIN32 = -D__WIN32__ -DWIN32_LEAN_AND_MEAN -D__WINB_DEBUG__
C_OPT = -Wall -fexpensive-optimizations -O3 -finline-functions -nostdinc -mno-stack-arg-probe -ffreestanding $(WIN32) $(INCL)
ASM_OPT += $(INCL)
LINK_OPT = -Bstatic -L$(NEWLIBC)/lib

OBJS = kernel32.o user32.o advapi32.o oleaut32.o msvcrt.o init.o

all: winb.o

clean :
	$(RM) *.o

winb.o: $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -lc -lfd32 -r -s -o winb.o

# Common rules
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
%.s : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -S $<
%.o : %.s
	$(CC) $(ASM_OPT) -c $<

