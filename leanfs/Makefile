# GNU Makefile for the LEAN File System Driver version 0.1

CC       = gcc
OBJS     = alloc.o blockio.o dir.o file.o mount.o open.o test.o
BIN      = leanfs
MKLEANFS = mkleanfs
#CFLAGS   = -Wall -O3 -s
CFLAGS   = -Wall -g3

.PHONY: all clean

all: $(BIN) $(MKLEANFS)

clean:
	rm -f $(OBJS) $(BIN) $(MKLEANFS)

$(BIN): $(OBJS)
	$(CC) $(OBJS) -o $(BIN)

$(MKLEANFS): mkfs.c leanfs.h
	$(CC) $(CFLAGS) -o $(MKLEANFS) mkfs.c

%.o : %.c
	$(CC) -c $< $(CFLAGS)
