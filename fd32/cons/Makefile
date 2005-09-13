ifndef OSLIB
OSLIB = ../../oslib
endif

include $(OSLIB)/config.mk

INCL   = $(OSLIB)
LIB_PATH    = $(OSLIB)/lib/
LIB_DIR  = $(OSLIB)\lib

#DEBUG = -D__DEBUG__ -D__EXC_DEBUG__ -D__EXEC_DEBUG__ -D__PROCESS_DEBUG__

C_OPT += -I../include $(DEBUG)
LINK_OPT = -Bstatic

OBJS = cons1.o cons2.o message.o

.PHONY : clean allclean info install

all : cons.a

clean :
	$(RM) *.o
	$(RM) *.a

cons.a: $(OBJS)
	$(AR) rs cons.a $(OBJS)

# Common rules
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
%.s : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -S $<
%.o : %.s
	$(CC) $(ASM_OPT) -c $<
