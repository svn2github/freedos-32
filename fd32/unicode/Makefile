ifndef OSLIB
OSLIB = ../../oslib
endif

include $(OSLIB)/config.mk

INCL   = $(OSLIB)
LIB_PATH    = $(OSLIB)/lib/
LIB_DIR  = $(OSLIB)\lib

C_OPT +=  -I../include $(DEBUG)
LINK_OPT = -Bstatic

OBJS =  case.o convers.o utf8.o utf16.o

all: unicode.o

clean:
	$(RM) *.o

unicode.o: $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -r -s -o unicode.o

# Common rules
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
%.s : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -S $<
%.o : %.s
	$(CC) $(ASM_OPT) -c $<

