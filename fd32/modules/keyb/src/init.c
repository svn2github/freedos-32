/* Keyborad Driver for FD32
 * by Luca Abeni and Hanzac Chen
 *
 * 2004 - 2005
 * This is free software; see GPL.txt
 */

#include <dr-env.h>
#include <errno.h>
#include <devices.h>

#include "key.h"
#include "queues.h"


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
		fd32_sti();           /* Reenable interrupts... */
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

void keyb_init(struct process_info *pi)
{
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
