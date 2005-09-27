/* FD32's Keyborad Driver
 *
 * Copyright (C) 2004,2005 by Luca Abeni and Hanzac Chen
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
	code = keyb_get_data(); /* Input datum */

	/* Well, we do not check the error code, for now... */
	if(!preprocess(code)) { /* Do that leds stuff... */
		rawqueue_put(code);
		fd32_sti();         /* Reenable interrupts... */
		postprocess();
	}

	fd32_master_eoi();
}

/* Note: N = 0 Nonblock mode, N = -1 Block mode both read BIOS key
 *       N != 0 && N != -1 supporting console read
 */
static int read(void *id, DWORD n, BYTE *buf)
{
	int res = -EIO;
	DWORD count;

	/* Convention: n = 0 --> nonblock; n != 0 --> block */
	if (n == 0) {
		WORD w;
		if ((w = keyqueue_gethead()) == 0) {
			res = 0;
		} else {
			((WORD *)buf)[0] = w;
			res = 1;
		}
	} else if (n == -1) {
		WORD w;
		WFC(keyqueue_empty());
		/* Get the total BIOS key */
		w = keyqueue_get();
		w |= keyqueue_get()<<8;

		((WORD *)buf)[0] = w;
		res = 1;
	} else { /* Normal console get char or get string */
		BYTE b;
		count = n;
		while (count > 0) {
			WFC(keyqueue_empty());
			b = keyqueue_get();
			*buf++ = b, count--;

			/* Handle the extended key and get rid of scan code */
			if (count > 0 && b == 0) {
				*buf++ = keyqueue_get(), count--;
			} else if (b != 0) {
				/* only the extended code preserved on the queue */
				b = keyqueue_get();
			}
		}

		res = n;
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
				res = read(r->DeviceId, r->BufferBytes, r->Buffer);
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

void keyb_init(struct process_info *pi)
{
	char **argv;
	int argc = fd32_get_argv(name_get(pi), args_get(pi), &argv);

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
	/* Clear the shift flags and reset the leds, otherwise it won't work fine in some circumstances */
	keyb_set_shift_flags(0);

	/* Ctrl+Alt+Del reboot PC */
	keyb_hook(0x53, 1, 1, (DWORD)fd32_reboot);
	/* Ctrl+M print memory dump */
	keyb_hook(0x32, 1, 0, (DWORD)mem_dump);
	fd32_message("Installing new call keyb_read...\n");
	if (add_call("keyb_read", (DWORD)read, ADD) == -1) {
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
