ifndef FD32
FD32 = ../../fd32
endif
ifndef DRIVERS
DRIVERS = ../../drivers
endif

BASE = $(FD32)/oslib

include $(BASE)/config.mk

OBJS = read.o write.o readdir.o creat.o unlink.o rename.o mkdir.o rmdir.o dumpdir.o

all: $(OBJS)

clean:
	$(RM) *.o

# Common rules
%.o: %.c
	$(CC) $(C_OPT) -I$(FD32)/include -I$(DRIVERS)/include/fd32 -c $< -o $@

