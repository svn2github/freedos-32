/* DPMI Driver for FD32: int 0x16 services
 * by Luca Abeni
 *
 * This is free software; see GPL.txt
 */
 
#include <ll/i386/hw-data.h>
#include <ll/i386/hw-instr.h>
#include <ll/i386/hw-func.h>
#include <ll/i386/mem.h>
#include <ll/i386/string.h>
#include <ll/i386/error.h>
#include <ll/i386/cons.h>
#include <logger.h>
#include <devices.h>
#include <kernel.h>
#include "rmint.h"


BYTE get_keycode_nonblock(void)
{
  int dev;
  fd32_read_t R;
  static fd32_request_t *request = NULL;
  static void *dev_id;
  BYTE c;
  
  if (request == NULL) {
    dev = fd32_dev_search("kdb");
    if (dev < 0) {
      return 0;
    }

    fd32_dev_get(dev, &request, &dev_id, NULL, 0);
  }
  R.Size = sizeof(fd32_read_t);
  R.DeviceId = dev_id;
  R.Buffer = &c;
  R.BufferBytes = 0;		/* 0 is non-blocking read */
  
  /* I know that we should get the keycode, but for now this is ok... */
  return request(FD32_READ, &R);
}

int keybbios_int(union rmregs *r)
{
  BYTE keycode;

  switch (r->h.ah) {
    case 0x11:
      keycode = get_keycode_nonblock();
      if (keycode == 0) {
	r->x.flags |= 0x40;
      } else {
	r->x.flags &= ~0x40;
	r->x.ax = keycode;
      }
      RMREGS_CLEAR_CARRY;
      return 0;

    default:
      error("Unimplemeted INT!!!\n");
      message("INT 16, AX = 0x%x\n", r->x.ax);
      fd32_abort();
  }
  
  return 1;
}
