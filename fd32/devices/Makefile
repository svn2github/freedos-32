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

C_OPT +=  -I../include $(DEBUG) -I$(DRIVERS)/include/fd32
LINK_OPT = -Bstatic

all: devices.o

clean:
	$(RM) *.o

devices.o: devices.c
	$(REDIR) $(CC) $(C_OPT) -s -c devices.c

