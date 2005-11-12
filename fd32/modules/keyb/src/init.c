/* FD32's Keyborad Driver
 *
 * Copyright (C) 2003,2004 by Luca Abeni
 * Copyright (C) 2004,2005 by Hanzac Chen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <dr-env.h>
#include <errno.h>
#include <devices.h>

#include "key.h"
#include "queues.h"
#include "layout.h"


/* The name of the keyboard device. Using the Linux name. */
#define KEYB_DEV_NAME "kbd"


void keyb_handler(int n)
{
	static BYTE code;

	/* We are the int handler... If we are called, we assume that there is
		 something to be read... (so, we do not read the status port...)
	*/
	code = keyb_get_data();		/* Input datum */

	/* Well, we do not check the error code, for now... */
	if(!preprocess(code)) {	/* Do that leds stuff... */
		rawqueue_put(code);
		fd32_sti();				/* Re-enable interrupts... */
		postprocess();
	}

	fd32_master_eoi();
}

static int keyb_dev_read(void *buffer, size_t count)
{
	int ret = count;
	static int flag = 0;
	static BYTE b;

	/* Process the non-extended key, get rid of the scan code */
	while (count > 0) {
		if (flag == 0) {
			/* Wait for the next keystroke */
			WFC(keyqueue_empty());
			b = *(BYTE *)buffer++ = keyqueue_get(), count--;
		} else if (b == 0) { /* If keyqueue not empty, rawqueue can't be empty */
			*(BYTE *)buffer++ = rawqueue_get(), count--;
		} else {
			rawqueue_get();
		}
		flag = !flag;
	}

	return ret;
}

/* Block mode to read BIOS key */
static uint16_t keyb_dev_get_keycode(void)
{
	WFC(keyqueue_empty());
	return keyqueue_get()|(rawqueue_get()<<8);
}

/* Nonblock mode to read BIOS key */
static uint16_t keyb_dev_check_keycode(void)
{
	return keyqueue_check()|(rawqueue_check()<<8);
}

static int keyb_read(void *id, size_t n, BYTE *buf)
{
	int res = -EIO;

	/* Convention: n = 0 --> nonblock; n != 0 --> block */
	if (n == 0) {
		WORD w;
		if ((w = keyb_dev_check_keycode()) == 0) {
			res = 0;
		} else {
			((WORD *)buf)[0] = w;
			res = 1;
		}
	} else if (n == -1) {
		WFC(keyqueue_empty());
		((WORD *)buf)[0] = keyb_dev_get_keycode();
		res = 1;
	} else { /* Normal console get char or get string */
		res = keyb_dev_read(buf, n);
	}

	return res;
}

static fd32_request_t keyb_request;
static int keyb_request(DWORD function, void *params)
{
	int res;
	fd32_read_t *r = (fd32_read_t *) params;

	switch (function) {
		case FD32_GET_DEV_INFO:
			res = KEYB_DEV_INFO;
			break;
		case FD32_GETATTR:
			res = keyb_get_shift_flags();
			break;
		case FD32_READ:
			if (r->Size < sizeof(fd32_read_t)) {
				res = -EINVAL;
			} else {
				res = keyb_read(r->DeviceId, r->BufferBytes, r->Buffer);
			}
			break;
		default:
			res = -ENOTSUP;
			break;
	}
	
	return res;
}

static struct option keyb_options[] =
{
  /* These options set a flag. */

  /* These options don't set a flag.
     We distinguish them by their indices. */
  {"file-name", required_argument, 0, 'F'},
  {"layout-name", required_argument, 0, 'L'},
  {0, 0, 0, 0}
};

void keyb_init(process_info_t *pi)
{
	char **argv;
	int argc = fd32_get_argv(pi->filename, pi->args, &argv);

	if (argc > 1) {
		char *layoutname = "US";
		char *filename = "A:\\fd32\\keyboard.sys";
		int c, option_index = 0;
		/* Parse the command line */
		for ( ; (c = getopt_long (argc, argv, "+F:L:", keyb_options, &option_index)) != -1; ) {
			switch (c) {
				case 'F':
					filename = optarg;
					break;
				case 'L':
					layoutname = optarg;
					break;
				default:
					break;
			}
		}
		fd32_message("Choose a layout %s:%s ...\n", filename, layoutname);
		keyb_layout_choose(filename, layoutname);
	}
	fd32_unget_argv(argc, argv);
	/* Handle the keyboard */
	fd32_message("Setting Keyboard handler\n");
	fd32_irq_bind(KEYB_IRQ, keyb_handler);
	/* Clear the queue */
	keyb_queue_clear();
	/* TODO: Reset the keyboard, otherwise it won't work fine in some circumstances */

	/* Ctrl+Alt+Del reboot PC */
	keyb_hook(0x53, 1, 1, (DWORD)fd32_reboot);
	/* Ctrl+M print memory dump */
	keyb_hook(0x32, 1, 0, (DWORD)mem_dump);
	fd32_message("Installing new call keyb_read...\n");
	if (add_call("keyb_read", (DWORD)keyb_read, ADD) == -1) {
		fd32_error("Failed to install a new kernel call!!!\n");
	}
	fd32_message("Installing new call keyb_hook...\n");
	if (add_call("keyb_hook", (DWORD)keyb_hook, ADD) == -1) {
		fd32_error("Failed to install a new kernel call!!!\n");
	}
	fd32_message("Registering...\n");
	if (fd32_dev_register(keyb_request, NULL, KEYB_DEV_NAME) < 0) {
		fd32_message("Failed to register!!!\n");
	}
}
