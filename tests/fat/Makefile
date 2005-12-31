ifndef FD32
FD32 = ../../fd32
endif
ifndef DRIVERS
DRIVERS = ../../fd32/modules
endif
ifndef OSLIB
OSLIB = ../../oslib
endif

BASE = $(FD32)/oslib

include $(OSLIB)/config.mk

OBJS = read.o write.o readdir.o creat.o unlink.o rename.o mkdir.o rmdir.o dumpdir.o mediachg.o

all: $(OBJS)

clean:
	$(RM) *.o

# Common rules
%.o: %.c
	$(CC) $(C_OPT) -I$(OSLIB) -I$(FD32)/include -I$(DRIVERS)/include/fd32 -c $< -o $@

