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


#define KEYB_CMD_SET_LEDS 0xED


static int ack_expected = 0;
static DWORD ecode = 0;
static volatile WORD *flags = (WORD *)BDA_KEYB_FLAG;

WORD decode_ex(WORD c);
WORD decode(BYTE c, int flags, int lock);
void keyb_fire_hook(WORD key, int isCTRL, int isALT);

BYTE keyb_get_status(void)
{
  return fd32_inb(KEYB_STATUS_PORT);
}

BYTE keyb_get_data(void)
{
  return fd32_inb(KEYB_DATA_PORT);
}

/*
 * Function to write in the keyboard register; it waits for the port
 * to be free, then write the command
 */
int kbd_write(BYTE cmd)
{
  DWORD i = 0;
  BYTE t, fl;

  while (((fl = keyb_get_status()) & 0x03) && (i < 0xffffff)) { i++; if (fl&0x01) t = fd32_inb(0x60); }
  if (i < 0xffffff) {
    fd32_outb(KEYB_DATA_PORT, cmd);
    return 1;
  } else {
    return 0;
  }
}

static void set_leds(void)
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


WORD keyb_get_shift_flags(void)
{
  return flags[0];
}

void keyb_set_shift_flags(WORD f)
{
  flags[0] = f;
  set_leds();
}

static void handle_ack(void)
{
  if (ack_expected == 0) {
    /* What happened? An ack arrived but we are not waiting for it... */
    fd32_error("WARNING: Unexpected Ack!!!\n");
  } else if (ack_expected == 1) {
    /* We sent a 0xED command... After the ack, the led mask must be sent! */
    kbd_write((flags[0]&LEGS_MASK)>>4);
    ack_expected = 2;
  } else if (ack_expected == 2) {
    /* Uhmmm... We don't need this!
    kbd_write(KBD_CMD_ENABLE); */
    ack_expected = 0;
  } else {
    /* This is impossible!!! */
    fd32_message("[KEYB] CRITICAL: ack_expected = %d\n", ack_expected);
    fd32_error("Keyboard driver: inconsistent internal status!\n");
  }
}

/*
 * Function mostly handle the LEDs
 */
int preprocess(BYTE code)
{
  switch(code) {
  	case 0xE0:
      return 0;
    case 0xFA:
      handle_ack();
      break;
    /* FUNCTION KEY pressed ... */
    case MK_LCTRL:
      flags[0] |= LCTRL_FLAG|CTRL_FLAG;
      break;
    case MK_LSHIFT:
      flags[0] |= LSHIFT_FLAG;
      break;
    case MK_RSHIFT:
      flags[0] |= RSHIFT_FLAG;
      break;
    case MK_LALT:
      flags[0] |= LALT_FLAG|ALT_FLAG;
      break;
    case MK_CAPS:
      flags[0] |= CAPS_FLAG;
      flags[0] ^= LED_CAPS;
      set_leds();
      break;
    case MK_NUMLK:
      flags[0] |= NUMLK_FLAG;
      flags[0] ^= LED_NUMLK;
      set_leds();
      break;
    case MK_SCRLK:
      flags[0] |= SCRLK_FLAG;
      flags[0] ^= LED_SCRLK;
      set_leds();
      break;
    case MK_SYSRQ:
      flags[0] |= SYSRQ_FLAG;
      fd32_message("SYSRQ pressed ...\n");
      break;

    /* FUNCTION KEY released ... */
    case BREAK|MK_LCTRL:
      flags[0] &= ~(LCTRL_FLAG|CTRL_FLAG);
      break;
    case BREAK|MK_LSHIFT:
      flags[0] &= ~LSHIFT_FLAG;
      break;
    case BREAK|MK_RSHIFT:
      flags[0] &= ~RSHIFT_FLAG;
      break;
    case BREAK|MK_LALT:
      flags[0] &= ~(LALT_FLAG|ALT_FLAG);
      break;
    case BREAK|MK_CAPS:
      flags[0] &= ~CAPS_FLAG;
      break;
    case BREAK|MK_NUMLK:
      flags[0] &= ~NUMLK_FLAG;
      break;
    case BREAK|MK_SCRLK:
      flags[0] &= ~SCRLK_FLAG;
      break;
    case BREAK|MK_SYSRQ:
      flags[0] &= ~SYSRQ_FLAG;
      fd32_message("SYSRQ released ...\n");
      break;

    default:
      return 0;
  }

  return 1;
}

/* Interrupts are enabled, here... */
void postprocess(void)
{
  WORD decoded = 0;
  BYTE code = rawqueue_get();

  while (code != 0) {
    if (ecode == 0xE000) {
      /* Handle the extended key */
      int done = 1;
      ecode |= code;
      switch(ecode) {
        /* FUNCTION KEY pressed ... */
        case MK_RCTRL:
          flags[0] |= RCTRL_FLAG|CTRL_FLAG;
          break;
        case MK_RALT:
          flags[0] |= RALT_FLAG|ALT_FLAG;
          break;
        /* FUNCTION KEY released ... */
        case BREAK|MK_RCTRL:
          flags[0] &= ~(RCTRL_FLAG|CTRL_FLAG);
          break;
        case BREAK|MK_RALT:
          flags[0] &= ~(RALT_FLAG|ALT_FLAG);
          break;
    
        /* Extended KEY, manage the instert status */
        case MK_INSERT:
          flags[0] ^= INSERT_FLAG;
          done = 0;
          break;
        default:
          if (!(BREAK&ecode))
            done = 0;
          break;
      }
      /* Determine if need the extended key */
      if (!done) {
        /* fire the hook if exists */
        keyb_fire_hook(code, flags[0]&CTRL_FLAG, flags[0]&ALT_FLAG);
      	decoded = decode_ex(ecode);
        keyqueue_put(decoded);
      }
      ecode = 0;
    } else if (code == 0xE0) {
      /* Prepare the extended key */
      ecode = 0xE000;
    } else if (!(BREAK&code)) {
      /* fire the hook if exists */
      keyb_fire_hook(code, flags[0]&CTRL_FLAG, flags[0]&ALT_FLAG);
      /* Handle the basic key */
      decoded = decode(code, flags[0], flags[0]&LEGS_MASK);
      /* Decode it... And put the scancode and charcode into the keyqueue */
      if (decoded == 0) {
        fd32_log_printf("Strange key: %d (0x%x)\n", code, code);
      } else {
        keyqueue_put(decoded);
      }
    }
    /* Retrieve raw code again */
    code = rawqueue_get();
  }
}
