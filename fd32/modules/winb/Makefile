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

INCL   = -I$(FDOS)/include -I$(OSLIB) -Imingw-runtime-3.7/include -Iw32api-3.2/include
LIB_PATH = $(OSLIB)/lib/
LIB_DIR  = $(OSLIB)\lib

WIN32 = -D__WIN32__ -DWIN32_LEAN_AND_MEAN -D__WINB_DEBUG__
C_OPT = -Wall -fexpensive-optimizations -O3 -finline-functions -nostdinc -mno-stack-arg-probe -ffreestanding $(WIN32) $(INCL)
ASM_OPT += $(INCL)
LINK_OPT = -Bstatic -L$(LIBFD32) -L$(NEWLIBC)

OBJS = kernel32.o msvcrt.o init.o

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

