# Standard library for X/COFF applications
# Makefile for GNU MAKE & GCC 2.8.0

BASE = ../oslib
include $(BASE)/config.mk

INCL   = $(BASE)
LIB_PATH    = $(BASE)/lib/
LIB_DIR  = $(BASE)\lib

#DEBUG = -D__MOD_DEBUG__ -D__BOOT_DEBUG__

C_OPT +=  -I$(INCL) -I../include $(DEBUG)
ASM_OPT += -I$(INCL)
LINK_OPT = -Bstatic

OBJS = boot.o init.o modules.o mem.o mmap.o modfs.o

all: fdboot.o

clean :
	$(RM) *.o

fdboot.o: $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -r -o fdboot.o
fd32.bin : $(OBJS) $(LIB_PATH)/libhc.a $(LIB_PATH)/libhx.a
	$(LD) $(LINK_OPT) $(LIB_PATH)x0.o $(OBJS) --start-group -lhc -lhx --end-group -o $@

# Common rules
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
%.s : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -S $<
%.o : %.s
	$(CC) $(ASM_OPT) -c $<

