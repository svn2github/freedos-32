ifndef FD32
FD32 = ../../fd32
endif

include $(FD32)/config.mk

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
