ifndef FD32
FD32 = ../fd32
endif

BASE = $(FD32)/oslib

include $(BASE)/config.mk

DJGPP_TESTS = hello.out empty.out exctest.out get.out argtest.out filetest.out exectest.out
NATIVE_TESTS = fattest.32 read.32
all: $(DJGPP_TESTS) $(NATIVE_TESTS)
clean:
	$(RM) *.exe
	$(RM) *.o
	$(RM) *.out
	$(RM) *.32

hello.out: hello.o main.o
	$(LD)  -Bstatic --oformat coff-go32 -s -nostartfiles -nostdlib hello.o main.o -o hello.out

main.o: main.c
	$(CC) -c $(C_OPT_1) main.c

# Common rules
%.32: %.c
	$(CC) $(C_OPT) -I$(FD32)/include -I$(BASE)/ll -c $< -o $@
%.out: %.c
	$(CC) -Xlinker -Ttext -Xlinker 0x150000 $< -o $@
