ifndef OSLIB
OSLIB = ../../oslib
endif
ifndef DRIVERS
DRIVERS = ../../drivers
endif
ifndef PORTAB
PORTAB = ../../drivers/include/fd32
endif

include $(OSLIB)/config.mk

INCL   = $(OSLIB)
LIB_PATH    = $(OSLIB)/lib/
LIB_DIR  = $(OSLIB)\lib

C_OPT +=  -I../include -I$(DRIVERS) -I$(PORTAB) $(DEBUG)
LINK_OPT = -Bstatic

OBJS = cp437.o

all: cp437.o

clean:
	$(RM) *.o

# Module with single object, Common compile and link
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) -c $< $(LINK_OPT) -o tmp.o
	$(LD) $(LINK_OPT) tmp.o -r -s -o $@
	$(RM) tmp.o
