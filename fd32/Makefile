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

OBJS =	boot/fdboot.a devices/devices.a dynalink/dynalink.a \
	kernel/kernel.a filesys/filesys.a cons/cons.a modules/block/block.o

KERNEL_DIRS = $(OSLIB) boot cons devices dynalink filesys kernel modules/block

.PHONY: all modules kern_subdirs clean allclean

all: fd32.bin modules

install: all
	$(MAKE) -C modules install

clean: $(OSLIB)/config.mk
	$(RM) fd32.bin
	$(MAKE) -C boot clean
	$(MAKE) -C cons clean
	$(MAKE) -C devices clean
	$(MAKE) -C dynalink clean
	$(MAKE) -C filesys clean
	$(MAKE) -C kernel clean
	$(MAKE) -C modules clean
	$(MAKE) -C modules/block clean

allclean: clean
	$(MAKE) -C $(OSLIB) allclean

$(OSLIB)/config.mk: config.mk
	$(CP) config.mk $(OSLIB_DIR)

kern_subdirs: $(patsubst %, _dir_%, $(KERNEL_DIRS))

$(patsubst %, _dir_%, $(KERNEL_DIRS)): $(OSLIB)/config.mk
	$(MAKE) -C $(patsubst _dir_%, %, $@)

modules: $(OSLIB)/config.mk
	$(MAKE) -C modules

$(LIB_PATH)/%.a: $(OSLIB)/config.mk
	$(MAKE) install -C $(OSLIB)

fd32.bin : kern_subdirs $(LIB_PATH)/libhc.a $(LIB_PATH)/libhx.a
	$(LD) $(LINK_OPT) $(LIB_PATH)x0.o --start-group $(OBJS) -lhc -lhx -lkl --end-group -o $@

kernel.src: fd32.bin
	$(OBJDUMP) --disassemble fd32.bin > kernel.src
