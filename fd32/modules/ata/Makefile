.PHONY: fd32
all: fd32 djgpp

clean :
	$(MAKE) -C fd32 clean
	$(MAKE) -C djgpp clean
	$(RM) *.o
	$(RM) *.exe

fd32:
	$(MAKE) -C fd32
djgpp:
	$(MAKE) -C djgpp
