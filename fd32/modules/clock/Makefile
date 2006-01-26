ifndef FD32
FD32 = ../..
endif
include $(FD32)/config.mk

.PHONY: fd32
all: fd32 djgpp

install: all
	$(CP) clockdrv.o ..

clean :
	$(MAKE) -C fd32 clean
	$(MAKE) -C djgpp clean
	$(RM) *.o
	$(RM) *.exe

fd32:
	$(MAKE) -C fd32
djgpp:
	$(MAKE) -C djgpp
