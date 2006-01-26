ifndef FD32
FD32 = ../..
endif

include $(FD32)/config.mk

all: fd32 djgpp

.PHONY: fd32 djgpp

install: all
	$(CP) keybdrv.o ..

clean :
	$(RM) *.o
	$(MAKE) -C fd32 clean
	$(MAKE) -C djgpp clean

djgpp:
	$(MAKE) -C djgpp

fd32:
	$(MAKE) -C fd32
