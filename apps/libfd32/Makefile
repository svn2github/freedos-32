ifndef FD32
FD32 = ../newlib-port/sys/fd32
endif
ifndef NEWLIBC
NEWLIBC = ../newlib
endif

include config.mk

OBJS = execve.o exit.o io.o isatty.o sbrk.o stubs.o unlink.o time.o sysconf.o
# Add the mapping of the syscalls to c names
C_OPT += -D_close=close \
		-D_execve=execve \
		-D_fcntl=fcntl \
		-D_fork=fork \
		-D_fstat=fstat \
		-D_getpid=getpid \
		-D_gettimeofday=gettimeofday \
		-D_kill=kill \
		-D_link=link \
		-D_lseek=lseek \
		-D_open=open \
		-D_read=read \
		-D_sbrk=sbrk \
		-D_stat=stat \
		-D_times=times \
		-D_unlink=unlink \
		-D_wait=wait \
		-D_write=write \
		-I$(NEWLIBC)/include -I$(FD32)/include -I$(FD32)/sys

LINK_OPT = -Bstatic
VPATH = $(FD32)

.PHONY: all clean

all: crt0.o libfd32.a

libfd32.a: $(OBJS)
	$(AR) cru libfd32.a $(OBJS)

clean:
	$(RM) *.o
	$(RM) *.a
