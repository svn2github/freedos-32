# Standard library for X/COFF applications
# Makefile for GNU MAKE & GCC 2.8.0

# Standard library for X/COFF applications
# Makefile for GNU MAKE & GCC 2.8.0

ifndef BASE
	BASE = ..
	BASEDOS = ..
endif

include $(BASE)/config.mk

#C_OPT += -DPROFILE
#ASM_OPT += -DPROFILE

COMMON_OBJS = reboot.o

STRING_OBJS = strncat.o \
	strrchr.o \
	strstr.o \
	string.o \
	special.o \
	strnum.o \
	strbase.o

IO_OBJS = ecvt.o \
	fcvt.o \
	gcvt.o \
	sprintf.o \
	ksprintf.o \
	sscanf.o \
	ucvt.o

STD_OBJS = random.o \
	stdlib.o \
	strtod.o \
	strtoi.o \
	strtol.o \
	strtou.o \
	strtoul.o

GNU_S_OBJS = modf.o

GNU_C_OBJS =

OBJS = $(GNU_S_OBJS) $(GNU_C_OBJS) $(COMMON_OBJS) $(STRING_OBJS) $(IO_OBJS) $(STD_OBJS)

vpath %.c string stdlib ioformat
#VPATH := $(subst $(space),:,$(SRCDIRS) $(MOSTLY_SRCDIRS))

.PHONY : clean allclean info

info :
	@echo "OSLib Makefile"
	@echo "Chose: all, install, clean"
	

all : libhc.a

libs : libhc.a

install : libhc.a $(LIB_DIR)
	$(CP) libhc.a $(LIB_DIR)

$(LIB_DIR) :
	$(MKDIR) $(LIB_DIR)
clean :
	$(RM) *.o
	$(RM) *.err
	$(RM) libhc.a

allclean :
	echo # XTN Library dependencies > deps
	$(RM) $(LIB_PATH)libhc.a

deps: $(COMMON_OBJS:.o=.c) $(patsubst %.o,string/%.c,$(STRING_OBJS)) $(patsubst %.o,ioformat/%.c,$(IO_OBJS)) $(patsubst %.o,stdlib/%.c,$(STD_OBJS))
	$(CC) -E $(C_OPT) -M $(COMMON_OBJS:.o=.c) \
	$(patsubst %.o,string/%.c,$(STRING_OBJS)) \
	$(patsubst %.o,ioformat/%.c,$(IO_OBJS)) \
	$(patsubst %.o,stdlib/%.c,$(STD_OBJS))  > deps

#
# The library!!
#
libhc.a : $(OBJS)
	$(AR) rs libhc.a $(OBJS)

ifeq (deps,$(wildcard deps))
include deps
endif
