#include <dr-env.h>
#include "devices.h"
#include "errors.h"

#include "queues.h"

/* The name of the keyboard device. Using the Linux name. */
#define KEYB_DEV_NAME "kbd"

/* Uhmmm... This could go in a separate header file??? */
int preprocess(BYTE code);
void postprocess(void);
BYTE keyb_get_data(void);
int fd32_register(int h, void *f, int type);


void keyb_handler(int n)
{
  BYTE code;

  /* We are the int handler... If we are called, we assume that there is
     something to be read... (so, we do not read the status port...)
  */
  code = keyb_get_data(); /* Input datum */
  /* Well, we do not check the error code, for now... */
  if (!preprocess(code)) { /* Do that leds stuff... */
    rawqueue_put(code);

    fd32_sti();           /* Reenable interrupts... */
    postprocess();
  }
  fd32_master_eoi();
}

static int read(void *id, DWORD n, BYTE *buf)
{
  char b;
  int count;

  /* Convention: n = 0 --> nonblock; n != 0 --> block */
  if (n == 0) {
    if ((b = keyqueue_get()) == 0) {
      return 0;
    } else {
      *buf = b;
      return 1;
    }
  } else {
    count = n;
    while (count > 0) {
      WFC((b = keyqueue_get()) == 0);
      *buf++ = b;
      count--;
    }

    return n;
  }

  return -1;
}

static fd32_request_t keyb_request;
static int keyb_request(DWORD function, void *params)
{
  fd32_read_t *r = (fd32_read_t *) params;

  if (function != FD32_READ) {
    return FD32_EINVAL;
  }
  if (r->Size < sizeof(fd32_read_t)) return FD32_EFORMAT;
  return read(r->DeviceId, r->BufferBytes, r->Buffer);
}

void keyb_init(void)
{
  fd32_message("Setting Keyboard handler\n");
  fd32_irq_bind(1, keyb_handler);
  fd32_message("Installing new call keyb_read...\n");
  if (add_call("_keyb_read", (DWORD)read, ADD) == -1) {
    fd32_error("Failed to install a new kernel call!!!\n");
  }
  fd32_message("Registering...\n");
  if (fd32_dev_register(keyb_request, NULL, KEYB_DEV_NAME) < 0) {
    fd32_message("Failed to register!!!\n");
  }
}
