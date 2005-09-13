ifndef OSLIB
OSLIB = ../../oslib
endif

include $(OSLIB)/config.mk

INCL   = $(OSLIB)
LIB_PATH    = $(OSLIB)/lib/
LIB_DIR  = $(OSLIB)\lib

C_OPT +=  -I../include $(DEBUG)
LINK_OPT = -Bstatic

OBJS = devices.o

all: devices.a

clean:
	$(RM) *.o
	$(RM) *.a

devices.a: $(OBJS)
	$(AR) rs devices.a $(OBJS)

# Common rules
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
%.s : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -S $<
%.o : %.s
	$(CC) $(ASM_OPT) -c $<

