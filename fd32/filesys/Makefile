ifndef OSLIB
OSLIB = ../../oslib
endif
ifndef MODULES
MODULES = ../modules
endif
ifndef PORTAB
PORTAB = ../modules/include/fd32
endif

include $(OSLIB)/config.mk

INCL   = $(OSLIB)
LIB_PATH    = $(OSLIB)/lib/
LIB_DIR  = $(OSLIB)\lib

C_OPT +=  -I../include -I$(MODULES) -I$(PORTAB) $(DEBUG)
LINK_OPT = -Bstatic

OBJS =  drives.o fs.o jft.o truename.o

all: filesys.a

clean:
	$(RM) *.o
	$(RM) *.a

filesys.a: $(OBJS)
	$(AR) rs filesys.a $(OBJS)

# Common rules
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
%.s : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -S $<
%.o : %.s
	$(CC) $(ASM_OPT) -c $<

