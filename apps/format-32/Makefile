# format-32 for freedos-32 or DPMI environment
# Makefile
#
# The original Makefile is for format and Borland Turbo C++ 3.0.
#
# Modified by Hanzac Chen (hanzac at sourceforge.net)
# 2005-02


# Compile options
RM     = del
CPP    = g++.exe
CC     = gcc.exe
OBJS1  = src/createfs.o src/floppy.o src/hdisk.o src/main.o src/savefs.o src/bcread.o
OBJS2  = src/userint.o src/driveio.o src/init.o src/recordbc.o src/uformat.o
INCS   = 
# DEBUG  = -D__FORMAT_DEBUG__
CFLAGS = -Wall -fexpensive-optimizations -O3 $(INCS) $(DEBUG)

# Link options
LDFLAGS=
LDLIBS=


.PHONY: all all-before all-after clean clean-custom

all: all-before format.exe all-after

clean: clean-custom
	$(RM) src\*.o

format.exe: $(OBJS1) $(OBJS2)
	$(CC) $(LDFLAGS) $(OBJS1) $(OBJS2) $(LDLIBS) -s -o format.exe

# Common rules
%.o : %.c
	$(REDIR) $(CC) $(CFLAGS) $(C_OUTPUT) -c $< -o $@