ifndef OSLIB
OSLIB = ../../oslib
endif

include $(OSLIB)/config.mk

INCL   = $(OSLIB)
LIB_PATH    = $(OSLIB)/lib/
LIB_DIR  = $(OSLIB)\lib

#DEBUG = -D__ELF_DEBUG__

C_OPT += -I../include $(DEBUG)
LINK_OPT = -Bstatic

OBJS = coff.o elf.o pecoff.o common.o

all: dynalink.o

clean :
	$(RM) *.o

dynalink.o: $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -r -o dynalink.o

# Common rules
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
%.s : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -S $<
%.o : %.s
	$(CC) $(ASM_OPT) -c $<

