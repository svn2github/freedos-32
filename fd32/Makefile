include ./config.mk

ifndef OSLIB
OSLIB = ../oslib
endif
ifndef OSLIB_DIR
OSLIB_DIR = ../oslib
endif

INCL   = $(OSLIB)
LIB_PATH    = $(OSLIB)/lib/
LIB_DIR  = $(OSLIB)\lib

OBJS =	boot/fdboot.o devices/devices.o dynalink/dynalink.o filesys/filesys.o\
	kernel/kernel.o nls/cp437.o unicode/unicode.o cons/cons.o

KERNEL_DIRS = $(OSLIB) boot devices dynalink filesys kernel nls unicode cons

all: fd32.bin drv_subdir

clean : $(OSLIB)/config.mk
	$(RM) fd32.bin
	make -C boot clean
	make -C devices clean
	make -C dynalink clean
	make -C filesys clean
	make -C kernel clean
	make -C nls clean
	make -C unicode clean
	make -C drivers/dpmi clean
	make -C cons clean

allclean: clean
	make -C $(OSLIB) clean

$(OSLIB)/config.mk: config.mk
	$(CP) config.mk $(OSLIB_DIR)

kern_subdirs: $(patsubst %, _dir_%, $(KERNEL_DIRS))

$(patsubst %, _dir_%, $(KERNEL_DIRS)): $(OSLIB)/config.mk
	$(MAKE) -C $(patsubst _dir_%, %, $@)
drv_subdir: $(OSLIB)/config.mk
	$(MAKE) -C drivers

$(LIB_PATH)/%.a: $(OSLIB)/config.mk
	make install -C $(OSLIB)

fd32.bin : kern_subdirs $(LIB_PATH)/libhc.a $(LIB_PATH)/libhx.a
	$(LD) $(LINK_OPT) $(LIB_PATH)x0.o $(OBJS) --start-group -lhc -lhx --end-group -o $@

kernel.src: fd32.bin
	$(OBJDUMP) --disassemble fd32.bin > kernel.src
