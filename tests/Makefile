ifndef FD32
FD32 = ../fd32
endif
ifndef DRIVERS
DRIVERS = ../drivers
endif

BASE = $(FD32)/oslib

include $(BASE)/config.mk

DJGPP_TESTS = hello.out empty.out exctest.out get.out argtest.out filetest.out exectest.out
NATIVE_TESTS = fstest.32 read.32

all: $(DJGPP_TESTS) $(NATIVE_TESTS)
	make -C fat
clean:
	$(RM) *.exe
	$(RM) *.o
	$(RM) *.out
	$(RM) *.32
	make -C fat clean

hello.out: hello.o main.o
	$(LD)  -Bstatic --oformat coff-go32 -s -nostartfiles -nostdlib hello.o main.o -o hello.out

main.o: main.c
	$(CC) -c $(C_OPT_1) main.c
command.exe: command.c
	$(CC) -Xlinker -Ttext -Xlinker 0x150000 command.c -o command.exe

# Common rules
%.32: %.c
	$(CC) $(C_OPT) -I$(FD32)/include -I$(DRIVERS)/include/fd32 -I$(BASE)/ll -c $< -o $@
%.out: %.c
	$(CC) -Xlinker -Ttext -Xlinker 0x400000 $< -o $@
