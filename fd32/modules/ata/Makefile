.PHONY: fd32
all: fd32 #djgpp

clean :
	$(MAKE) -C fd32 clean
#	$(MAKE) -C djgpp clean
	$(RM) *.o
	$(RM) *.exe

fd32:
	cp ../shared/partscan/partscan.c ./src  # This is a bit dirty...
	$(MAKE) -C fd32
djgpp:
	$(MAKE) -C djgpp
