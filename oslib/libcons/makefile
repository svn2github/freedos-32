# Standard library for X/COFF applications
# Makefile for GNU MAKE & GCC 2.8.0

ifndef BASE
	BASE = ..
	BASEDOS = ..
endif

include $(BASE)/config.mk

#C_OPT += -DPROFILE
#ASM_OPT += -DPROFILE

OBJS = cons1.o \
	cons2.o \
	cprintf.o \
	message.o

.PHONY : clean allclean info install

info :
	@echo "OSLib Makefile"
	@echo "Chose: all, install, clean"
	
all : libcons.a

install : libcons.a $(LIB_DIR)
	$(CP) libcons.a $(LIB_DIR)

$(LIB_DIR) :
	$(MKDIR) $(LIB_DIR)

clean :
	$(RM) *.o
	$(RM) *.err
	$(RM) libcons.a

allclean :
	echo # XTN Library dependencies > deps
	$(RM) $(LIB_PATH)libcons.a

deps: $(OBJS:.o=.c)
	$(CC) $(C_OPT) -M $(OBJS:.o=.c) > deps

#
# The library!!
#
libcons.a : $(OBJS)
	$(AR) rs libcons.a $(OBJS)

ifeq (deps,$(wildcard deps))
include deps
endif
