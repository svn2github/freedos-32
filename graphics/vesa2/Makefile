OBJS   = box.o boxalpha.o clipping.o copy.o getimage.o modes.o putimage.o textout.o
TESTC  = test.c ../pcffont.c ../bmpimage.c
TESTH  = ../pcffont.h ../bmpimage.h
C_OPT  = -Wall -O3 -s
C_OPT += -D__MMX__
CC     = gcc
AR     = ar
RM     = del
ECHO   = @echo

.PHONY: all clean

all: libvesa2.a test.exe profile.exe

libvesa2.a: $(OBJS)
	$(ECHO) *** Copiling VESA2 library... ***
	$(AR) rs libvesa2.a $(OBJS)
	$(RM) *.o

test.exe: $(TESTC) $(TESTH)
	$(ECHO) *** Compiling test program... ***
	$(CC) -Wall -O3 -s $(TESTC) -o test.exe -L. -lvesa2

profile.exe: $(TESTC) $(TESTH)
	$(ECHO) *** Compiling test program for profiling... ***
	$(CC) -Wall -O3 -g -pg -DPROFILE $(TESTC) -o profile.exe -L. -lvesa2

clean:
	$(RM) *.o
	$(RM) libvesa2.a
	$(RM) *.exe
	$(RM) gmon.out

%.o: %.c
	$(CC) $(C_OPT) -c $<
