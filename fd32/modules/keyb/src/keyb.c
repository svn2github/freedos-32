/* Keyborad Driver for FD32
 * original by Luca Abeni
 * extended by Hanzac Chen
 *
 * 2004 - 2005
 * This is free software; see GPL.txt
 */

#include <dr-env.h>

#include "key.h"
#include "queues.h"

#define KEYB_STATUS_PORT  0x64
#define KEYB_DATA_PORT    0x60

#define KEYB_CMD_SET_LEDS 0xED

static int ack_expected = 0;

static DWORD ecode = 0;
static DWORD leds = 0;
static DWORD flags = 0;

WORD decode(BYTE c, int flags, int lock);

BYTE keyb_get_status(void)
{
  return fd32_inb(KEYB_STATUS_PORT);
}

BYTE keyb_get_data(void)
{
  return fd32_inb(KEYB_DATA_PORT);
}

DWORD keyb_get_shift_status(void)
{
  return leds|flags;
}

/*
 * Function to write in the keyboard register; it waits for the port
 * to be free, then write the command
 */
int kbd_write(BYTE cmd)
{
  DWORD i = 0;
  BYTE fl;

  while (((fl = fd32_inb(0x64)) & 0x01) && (i < 0xffffff)) i++;
  if (i < 0xffffff) {
    fd32_outb(KEYB_DATA_PORT, cmd);
    return 1;
  }
  else return 0;
}

void set_leds(void)
{
  if (ack_expected != 0) {
    /* Uhmm.... Why is this != 0? */
    fd32_error("Set Leds with ack_expected != 0!!!\n");
    /* The best thing to do here is to reset the keyboard... */
    return;
  }
  ack_expected = 1;
  kbd_write(KEYB_CMD_SET_LEDS);
}

void handle_ack(void)
{
  if (ack_expected == 0) {
    /* What happened? An ack arrived but we are not waiting for it... */
    fd32_error("Unexpected Ack!!!\n");
    return;
  }
  if (ack_expected == 1) {
    /* We sent a 0xED command... After the ack, the led mask must be sent! */
    kbd_write(leds);
    ack_expected = 2;
    return;
  }
  if (ack_expected == 2) {
#if 0   /* Uhmmm... We don't need this! */
    kbd_write(KBD_CMD_ENABLE);
#endif
    ack_expected = 0;
    return;
  }
  /* This is impossible!!! */
  fd32_message("ack_expected = %d\n", ack_expected);
  fd32_error("Keyboard driver: inconsistent internal status!\n");
}

int preprocess(BYTE code)
{
  if (ecode == 0xE000) {
    ecode |= code;
    switch(ecode) {
      /* FUNCTION KEY pressed ... */
      case MK_RCTRL:
        flags |= RCTRL_FLAG|CTRL_FLAG;
        break;
      case MK_RALT:
        flags |= RALT_FLAG|ALT_FLAG;
        break;
      /* FUNCTION KEY released ... */
      case BREAK|MK_RCTRL:
        flags &= ~(RCTRL_FLAG|CTRL_FLAG);
        break;
      case BREAK|MK_RALT:
        flags &= ~(RALT_FLAG|ALT_FLAG);
        break;

      /* Extended KEY */
      case BREAK|MK_INSERT:
        flags &= ~INSERT_FLAG;
        break;
      case MK_INSERT:
        flags |= INSERT_FLAG;
      default:
        if (BREAK&ecode) break;
        ecode = 0;
        return 0;
    }
    ecode = 0;
  } else {
    switch(code) {
      case 0xFA:
        handle_ack();
        break;
      case 0xE0:
        ecode = 0xE000;
        break;
      /* FUNCTION KEY pressed ... */
      case MK_LCTRL:
        flags |= LCTRL_FLAG|CTRL_FLAG;
        break;
      case MK_LSHIFT:
        flags |= LSHIFT_FLAG;
        break;
      case MK_RSHIFT:
        flags |= RSHIFT_FLAG;
        break;
      case MK_LALT:
        flags |= LALT_FLAG|ALT_FLAG;
        break;
      case MK_CAPS:
        flags |= CAPS_FLAG;
        if (leds & LED_CAPS) {
          leds &= ~LED_CAPS;
        } else {
          leds |= LED_CAPS;
        }
        set_leds();
        break;
      case MK_NUMLK:
        flags |= NUMLK_FLAG;
        if (leds & LED_NUMLK) {
          leds &= ~LED_NUMLK;
        } else {
          leds |= LED_NUMLK;
        }
        set_leds();
        break;
      case MK_SCRLK:
        flags |= SCRLK_FLAG;
        if (leds & LED_SCRLK) {
          leds &= ~LED_SCRLK;
        } else {
          leds |= LED_SCRLK;
        }
        set_leds();
        break;
      case MK_SYSRQ:
        flags |= SYSRQ_FLAG;
        fd32_message("SYSRQ pressed ...\n");
        break;

      /* FUNCTION KEY released ... */
      case BREAK|MK_LCTRL:
        flags &= ~(LCTRL_FLAG|CTRL_FLAG);
        break;
      case BREAK|MK_LSHIFT:
        flags &= ~LSHIFT_FLAG;
        break;
      case BREAK|MK_RSHIFT:
        flags &= ~RSHIFT_FLAG;
        break;
      case BREAK|MK_LALT:
        flags &= ~(LALT_FLAG|ALT_FLAG);
        break;
      case BREAK|MK_CAPS:
        flags &= ~CAPS_FLAG;
        break;
      case BREAK|MK_NUMLK:
        flags &= ~NUMLK_FLAG;
        break;
      case BREAK|MK_SCRLK:
        flags &= ~SCRLK_FLAG;
        break;
      case BREAK|MK_SYSRQ:
        flags &= ~SYSRQ_FLAG;
        fd32_message("SYSRQ released ...\n");
        break;

      default:
        if (BREAK&code) break;
        return 0;
    }
  }
  return 1;
}

/* Interrupts are enabled, here... */
void postprocess(void)
{
  WORD decoded = 0;
  BYTE code = rawqueue_get();

  while (code != 0) {
    /* Decode it... And put it in the keyboard queue */
    if (flags&CTRL_FLAG && flags&ALT_FLAG && code == 0x53)
      fd32_reboot();
    else if (flags&CTRL_FLAG && code == 0x32)
      mem_dump();
    else
      decoded = decode(code, flags, leds);
    if (decoded == 0) {
      fd32_log_printf("Strange key: %d (0x%x)\n", code, code);
    } else {
      if ((decoded & 0x00ff) == 0)
        keyqueue_put(0xE0), keyqueue_put(decoded>>0x08);
      else
        keyqueue_put(decoded & 0x00ff);
    }
    code = rawqueue_get();
  }
}
