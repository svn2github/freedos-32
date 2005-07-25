# Standard library for X/COFF applications
# Makefile for GNU MAKE & GCC 2.8.0

ifndef BASE
	BASE = ..
	BASEDOS = ..
endif

include $(BASE)/config.mk

all: mbdemo.xtn timetest.xtn eventdem.xtn vmdemo.xtn \
	ctxswdem.xtn scheddem.xtn cpudemo.xtn biosdemo.xtn \
	asdemo.xtn kerndem.xtn pitdemo.xtn vssd.xtn pitsched.xtn readdemo.xtn

%.ftp: %.xtn
	ncftpput -u ll -p example thorin . $<
%.flp: %.xtn
	mcopy $< a:

clean :
	$(RM) *.o
	$(RM) *.err
	$(RM) *.xtn

#
# Demo
#

j1.xtn: j1.o ti.o $(LIB_PATH)/libhc.a $(LIB_PATH)/libhm.a $(LIB_PATH)/libhx.a $(LIB_PATH)/libkl.a
	$(LD) $(LINK_OPT) $(LIB_PATH)x0.o j1.o ti.o --start-group -lhc -lhm -lhx -lkl -lcons --end-group -o $@
kerndem.xtn: syscalls.o $(LIB_PATH)/libhc.a $(LIB_PATH)/libhm.a $(LIB_PATH)/libhx.a $(LIB_PATH)/libkl.a
	$(LD) $(LINK_OPT) $(LIB_PATH)x0.o syscalls.o --start-group -lhc -lhm -lhx -lkl -lcons --end-group -o $@


%.xtn : %.o $(LIB_PATH)/libhc.a $(LIB_PATH)/libhm.a $(LIB_PATH)/libhx.a $(LIB_PATH)/libkl.a
	$(LD) $(LINK_OPT) $(LIB_PATH)x0.o $< --start-group -lhc -lhm -lhx -lkl -lcons --end-group -o $@
#       $(LD) $(LINK_OPT) $(LIB_PATH)x0.o $< --start-group -lhc -lhx -lkl --end-group -o $@
