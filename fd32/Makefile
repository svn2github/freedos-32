# Standard library for X/COFF applications
# Makefile for GNU MAKE & GCC 2.8.0

BASE = oslib

include $(BASE)/config.mk

INCL   = $(BASE)
LIB_PATH    = $(BASE)/lib/
LIB_DIR  = $(BASE)\lib

#LINK_OPT = -T oslib/os.x -Bstatic -Ttext 0x320000 -nostartfiles -nostdlib -L$(LIB_PATH)

OBJS =	boot/fdboot.o devices/devices.o dynalink/dynalink.o filesys/filesys.o\
	kernel/kernel.o nls/cp437.o unicode/unicode.o

KERNEL_DIRS = boot devices dynalink filesys kernel nls oslib unicode

all: fd32.bin drv_subdir

clean :
	$(RM) fd32.bin
	make -C boot clean
	make -C devices clean
	make -C dynalink clean
	make -C filesys clean
	make -C kernel clean
	make -C nls clean
	make -C unicode clean
	make -C drivers/dpmi clean

allclean: clean
	make -C oslib clean

kern_subdirs: $(patsubst %, _dir_%, $(KERNEL_DIRS))

$(patsubst %, _dir_%, $(KERNEL_DIRS)):
	$(MAKE) -C $(patsubst _dir_%, %, $@)
drv_subdir:
	$(MAKE) -C drivers

$(LIB_PATH)/%.a:
	make install -C oslib

fd32.bin : kern_subdirs
	$(LD) $(LINK_OPT) $(LIB_PATH)x0.o $(OBJS) --start-group -lhc -lhx --end-group -o $@

kernel.src: fd32.bin
	$(OBJDUMP) --disassemble fd32.bin > kernel.src
