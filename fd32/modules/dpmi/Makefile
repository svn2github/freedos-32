ifndef OSLIB
OSLIB = ../../../oslib
endif

include $(OSLIB)/config.mk

INCL   = $(OSLIB)
LIB_PATH    = $(OSLIB)/lib/
LIB_DIR  = $(OSLIB)\lib

#DEBUG = -D__DEBUG__ -D__DPMI_DEBUG__ -D__DPMIMEM_DEBUG__ -D__RM_INT_DEBUG__

C_OPT += -I../../include -I ./include $(DEBUG)
LINK_OPT = -Bstatic

OBJS = chandler.o dpmimem.o dpmiexc.o rmint.o ldtmanag.o init.o \
	vga.o int31_00.o int31_02.o int31_03.o int31_04.o int31_05.o \
	int31_06.o int31_0e.o int10.o int16.o int21.o int28.o int29.o \
	int33.o int31_09.o

VPATH = src

all: dpmi.o

clean :
	$(RM) *.o

dpmi.o: $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -r -s -o dpmi.o

# Common rules
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
%.s : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -S $<
%.o : %.s
	$(CC) $(ASM_OPT) -c $<

