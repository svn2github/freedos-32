ifndef OSLIB
OSLIB = ../../oslib
endif
ifndef DRIVERS
DRIVERS = ../../drivers
endif

include $(OSLIB)/config.mk

INCL   = $(OSLIB)
LIB_PATH    = $(OSLIB)/lib/
LIB_DIR  = $(OSLIB)\lib

C_OPT +=  -I$(INCL) -I../include $(DEBUG) -I$(DRIVERS)/include/fd32
ASM_OPT += -I$(INCL)
LINK_OPT = -Bstatic

OBJS = cp437.o

all: $(OBJS)

clean:
	$(RM) *.o

%.o : %.c
	$(REDIR) $(CC) $(C_OPT) -s -c $<

