ifndef OSLIB
OSLIB = ../../oslib
endif

include $(OSLIB)/config.mk

INCL   = $(OSLIB)
LIB_PATH    = $(OSLIB)/lib/
LIB_DIR  = $(OSLIB)\lib

#DEBUG = -D__MOD_DEBUG__ -D__BOOT_DEBUG__

C_OPT +=  -I../include $(DEBUG)
LINK_OPT = -Bstatic

OBJS = boot.o init.o modules.o mem.o mmap.o modfs.o

all: fdboot.a

clean :
	$(RM) *.o
	$(RM) *.a

fdboot.a: $(OBJS)
	$(AR) rs fdboot.a $(OBJS)
fd32.bin : $(OBJS) $(LIB_PATH)/libhc.a $(LIB_PATH)/libhx.a
	$(LD) $(LINK_OPT) $(LIB_PATH)x0.o $(OBJS) --start-group -lhc -lhx --end-group -o $@

# Common rules
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
%.s : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -S $<
%.o : %.s
	$(CC) $(ASM_OPT) -c $<

