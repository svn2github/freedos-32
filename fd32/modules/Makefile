MODULES = ata biosdisk cdrom clock console dpmi fat fat2 floppy keyb nls psmouse unicode wrapper

.PHONY: all clean

all: $(patsubst %, _dir_%, $(MODULES))

clean: $(patsubst %, _dir_%_clean, $(MODULES))

$(patsubst %, _dir_%, $(MODULES)):
	$(MAKE) -C $(patsubst _dir_%, %, $@)

$(patsubst %, _dir_%_clean, $(MODULES)):
	$(MAKE) -C $(patsubst _dir_%_clean, %, $@) clean
