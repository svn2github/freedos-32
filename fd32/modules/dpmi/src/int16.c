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


#define BIOSKEY_NON_BLOCKING 0
#define BIOSKEY_BLOCKING     1


extern void *kbddev_id;
extern fd32_request_t *kbdreq;


static WORD get_keycode(int mode)
{
  int res;
  fd32_read_t R;
  WORD c;

  if (kbdreq == NULL) {
    res = fd32_dev_search("kbd");
    if (res < 0) {
      return 0;
    }
    fd32_dev_get(res, &kbdreq, &kbddev_id, NULL, 0);
  }
  R.Size = sizeof(fd32_read_t);
  R.DeviceId = kbddev_id;
  R.Buffer = &c;
  if (mode == BIOSKEY_NON_BLOCKING) {
    R.BufferBytes = 0;      /* 0 is non-blocking read BIOS keycode */
  } else if (mode == BIOSKEY_BLOCKING) {
    R.BufferBytes = -1;     /* -1 is blocking read BIOS keycode */
  }

  /* Note: Directly get the keycode ... */
  res = kbdreq(FD32_READ, &R);
  if (res == 0) {
    return 0;
  }

  return c;
}

int keybbios_int(union rmregs *r)
{
  WORD keycode;

  switch (r->h.ah) {
    case 0x00:
    case 0x10:
      keycode = get_keycode(BIOSKEY_BLOCKING);
      if (keycode == 0) {
        r->x.flags |= 0x40;
      } else {
        r->x.flags &= ~0x40;
        r->x.ax = keycode;
      }

      RMREGS_CLEAR_CARRY;
      return 0;

    case 0x01:
    case 0x11:
      keycode = get_keycode(BIOSKEY_NON_BLOCKING);
      if (keycode == 0) {
        r->x.flags |= 0x40;
      } else {
        r->x.flags &= ~0x40;
        r->x.ax = keycode;
      }

      RMREGS_CLEAR_CARRY;
      return 0;

    case 0x02:
    case 0x12:
      if (kbdreq == NULL) RMREGS_SET_CARRY;
      else {
      	/* TODO: Should define new keyboard function number */
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
