ifndef OSLIB
OSLIB = ../../oslib
endif

include $(OSLIB)/config.mk

INCL   = $(OSLIB)
LIB_PATH    = $(OSLIB)/lib/
LIB_DIR  = $(OSLIB)\lib

C_OPT +=  -I../include $(DEBUG)
LINK_OPT = -Bstatic

OBJS = cp437.o

all: nls.o

clean:
	$(RM) *.o

nls.o: $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -r -s -o nls.o

%.o : %.c
	$(REDIR) $(CC) $(C_OPT) -s -c $<

