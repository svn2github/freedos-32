/* Keyborad Driver for FD32
 * original by Luca Abeni
 * extended by Hanzac Chen
 *
 * 2004 - 2005
 * This is free software; see GPL.txt
 */

#include <dr-env.h>
#include <errors.h>
#include <devices.h>

#include "key.h"
#include "queues.h"


/* The name of the keyboard device. Using the Linux name. */
#define KEYB_DEV_NAME "kbd"


/* TODO: This could go in a separate header file??? */
int preprocess(BYTE code);
void postprocess(void);
void set_leds(void);
BYTE keyb_get_data(void);
void keyb_queue_clear(void);
DWORD keyb_get_shift_flags(void);
void keyb_set_shift_flags(WORD f);


void keyb_handler(int n)
{
  static BYTE code;

  /* We are the int handler... If we are called, we assume that there is
     something to be read... (so, we do not read the status port...)
  */
  code = keyb_get_data(); /* Input datum */

  /* Well, we do not check the error code, for now... */
  if(!preprocess(code)) /* Do that leds stuff... */
    rawqueue_put(code);
  /* Reenable interrupts... */
  fd32_sti();

  fd32_master_eoi();
}

static int read(void *id, DWORD n, BYTE *buf)
{
  int res = FD32_EREAD;
  DWORD count;
  BYTE b;

  /* Convention: n = 0 --> nonblock; n != 0 --> block */
  if (n == 0) {
    postprocess();
    if ((b = keyqueue_gethead()) == 0) {
      res = 0;
    } else {
      *buf = b;
      res = 1;
    }
  } else {
    count = n;
    while (count > 0) {
      WFC((postprocess(), b = keyqueue_get()) == 0);
      *buf++ = b;
      count--;
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
        res = FD32_EFORMAT;
      } else {
        res = read(r->DeviceId, r->BufferBytes, r->Buffer);
      }
      break;
    default:
      res = FD32_EINVAL;
      break;
  }
  
  return res;
}

void keyb_init(void)
{
  /* Clear the queue */
  keyb_queue_clear();
  /* Clear the shift flags */
  keyb_set_shift_flags(0);
  /* Handle the keyboard */
  fd32_message("Setting Keyboard handler\n");
  fd32_irq_bind(1, keyb_handler);
  /* How to reset the leds, seems not work in some virtual machine */
  /* set_leds(); */
  fd32_message("Installing new call keyb_read...\n");
  if (add_call("keyb_read", (DWORD)read, ADD) == -1) {
    fd32_error("Failed to install a new kernel call!!!\n");
  }
  fd32_message("Registering...\n");
  if (fd32_dev_register(keyb_request, NULL, KEYB_DEV_NAME) < 0) {
    fd32_message("Failed to register!!!\n");
  }
}
