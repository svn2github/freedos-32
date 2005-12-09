include config.mk

all:
	$(MAKE) -C xlib all
	$(MAKE) -C libm all
	$(MAKE) -C libc all
	$(MAKE) -C libcons all
	$(MAKE) -C kl all

install:
	$(MAKE) -C xlib install
	$(MAKE) -C libm install
	$(MAKE) -C libc install
	$(MAKE) -C libcons install
	$(MAKE) -C kl install

clean:
	$(MAKE) -C xlib clean
	$(MAKE) -C libm clean
	$(MAKE) -C libc clean
	$(MAKE) -C libcons clean
	$(MAKE) -C kl clean
	$(MAKE) -C examples clean

allclean: clean
	$(RMDIR) lib
