#MODULES = ata biosdisk cdrom clock console dpmi fat fat2 floppy keyb nls psmouse unicode wrapper
MODULES = ata cdrom clock console dpmi fat floppy iso9660 keyb nls pit psmouse unicode wrapper

.PHONY: all clean

all: $(patsubst %, _dir_%, $(MODULES))

install: $(patsubst %, _dir_%_install, $(MODULES))

clean: $(patsubst %, _dir_%_clean, $(MODULES))
	$(RM) *.o
	$(RM) *.com

$(patsubst %, _dir_%, $(MODULES)):
	$(MAKE) -C $(patsubst _dir_%, %, $@)

$(patsubst %, _dir_%_install, $(MODULES)):
	$(MAKE) -C $(patsubst _dir_%_install, %, $@) install

$(patsubst %, _dir_%_clean, $(MODULES)):
	$(MAKE) -C $(patsubst _dir_%_clean, %, $@) clean
