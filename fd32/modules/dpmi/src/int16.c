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


extern void *kbddev_id;
extern fd32_request_t *kbdreq;


static BYTE get_keycode(int mode)
{
  int dev, res;
  fd32_read_t R;
  BYTE c;

  if (kbdreq == NULL) {
    dev = fd32_dev_search("kbd");
    if (dev < 0) {
      return 0;
    }
    fd32_dev_get(dev, &kbdreq, &kbddev_id, NULL, 0);
  }
  R.Size = sizeof(fd32_read_t);
  R.DeviceId = kbddev_id;
  R.Buffer = &c;
  if (mode == 0) {
    R.BufferBytes = 0;      /* 0 is non-blocking read */
  } else if (mode == 1) {
    R.BufferBytes = 1;
  }

  /* I know that we should get the keycode, but for now this is ok... */
  res = kbdreq(FD32_READ, &R);
  if (res == 0) {
    return 0;
  }

  return ((WORD)c) << 8 | c;
}

int keybbios_int(union rmregs *r)
{
  BYTE keycode;

  switch (r->h.ah) {
    case 0x00:
    case 0x10:
      keycode = get_keycode(1);
      if (keycode == 0) {
        r->x.flags |= 0x40;
      }
      else if (keycode == 0xE0)
      {
        r->x.flags &= ~0x40;
        r->h.ah = keycode;
        r->h.al = get_keycode(1);
      }
      else {
        r->x.flags &= ~0x40;
        r->x.ax = keycode;
      }
      RMREGS_CLEAR_CARRY;
      return 0;

    case 0x01:
    case 0x11:
      keycode = get_keycode(0);
      if (keycode == 0) {
        r->x.flags |= 0x40;
      }
      else if (keycode == 0xE0)
      {
        r->x.flags &= ~0x40;
        r->h.ah = keycode;
        r->h.al = get_keycode(0);
      }
      else {
        r->x.flags &= ~0x40;
        r->x.ax = keycode;
      }
      RMREGS_CLEAR_CARRY;
      return 0;

    case 0x02:
    case 0x12:
      if (kbdreq == NULL) RMREGS_SET_CARRY;
      else {
      	/* Fix it! should define new keyboard function number */
        r->x.ax = kbdreq(FD32_GETATTR, NULL);
        RMREGS_CLEAR_CARRY;
      }
      return 0;

    default:
      error("Unimplemeted INT!!!\n");
      message("INT 16, AX = 0x%x\n", r->x.ax);
      fd32_abort();
  }

  return 1;
}
