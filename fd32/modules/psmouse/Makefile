ifndef FDOS
FDOS = ../..
endif
ifndef OSLIB
OSLIB = ../../../oslib
endif

include $(FDOS)/config.mk

INCL   = $(OSLIB)
LIB_PATH    = $(OSLIB)/lib/
LIB_DIR  = $(OSLIB)\lib

C_OPT += -O3 -I../include -I$(FDOS)/include -I../include/fd32
ASM_OPT +=  -I$(INCL)
LINK_OPT = -Bstatic
# DEBUG_OPT = -D__PSMOUSE_DEBUG__

OBJS = init.o

all: psmouse.o

install: all
	$(CP) psmouse.o ..

clean :
	$(RM) *.o

psmouse.o: $(OBJS)
	$(LD) $(LINK_OPT) $(OBJS) -r -s -o psmouse.o

# Common rules
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(DEBUG_OPT) $(C_OUTPUT) -c $<
%.s : %.c
	$(REDIR) $(CC) $(C_OPT) $(DEBUG_OPT) $(C_OUTPUT) -S $<
%.o : %.s
	$(CC) $(ASM_OPT) -c $<

