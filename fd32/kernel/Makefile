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

OBJS = call.o pmm.o run.o exch.o abort.o process.o exec.o \
	syscalls.o logger.o slabmem.o params.o file.o utils.o

all: kernel.a

clean :
	$(RM) *.o
	$(RM) *.a

kernel.a: $(OBJS)
	$(AR) rs kernel.a $(OBJS)

# Common rules
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
%.s : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -S $<
%.o : %.s
	$(CC) $(ASM_OPT) -c $<

