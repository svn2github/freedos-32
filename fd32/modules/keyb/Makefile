# Standard library for X/COFF applications
# Makefile for GNU MAKE & GCC 2.8.0

ifndef FD32
FD32 = ../../fd32
endif

BASE = $(FD32)/oslib

include $(BASE)/config.mk

all: fd32 djgpp

.PHONY: fd32 djgpp

clean :
	$(RM) *.o
	$(RM) *.exe
	$(MAKE) -C fd32 clean
	$(MAKE) -C djgpp clean

djgpp:
	$(MAKE) -C djgpp

fd32:
	$(MAKE) -C fd32
