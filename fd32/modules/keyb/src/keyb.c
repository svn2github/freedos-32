#include <dr-env.h>

#include "key.h"
#include "queues.h"

#define KEYB_STATUS_PORT 0x64
#define KEYB_DATA_PORT   0x60

#define KEYB_CMD_SET_LEDS 0xED

static int ack_expected = 0;

static DWORD leds = 0;
static int flags;

#define KEYB_RAW     1
static int keyb_mode;

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
  if (code == 0xFA) {
    handle_ack();
    return 1;
  }

  if ((code == 0xE0) || (code == 0xE1)) {
    fd32_message("Double Code? 0x%x\n", code);
  }

  if (code & 0x80) {
    switch(code & 0x7F) {
      case LSHIFT:
      case RSHIFT:
	flags &= ~SHIFT_FLAG;
	break;
      case LCTRL:
	flags &= ~CTRL_FLAG;
      case LALT:
	flags &= ~ALT_FLAG;
      default:
    }
    return 1;
  }
  switch(code) {
    case LSHIFT:
    case RSHIFT:
      flags |= SHIFT_FLAG;
      break;
    case LCTRL:
      flags |= CTRL_FLAG;
      break;
    case LALT:
      flags |= ALT_FLAG;
      break;
    case CAPS:
      if (leds & LED_CAPS) {
	leds &= ~LED_CAPS;
      } else {
	leds |= LED_CAPS;
      }
      set_leds();
      break;
    case SCRLOCK:
      if (leds & LED_SCRLK) {
        leds &= ~LED_SCRLK;
      } else {
        leds |= LED_SCRLK;
      }
      set_leds();
      break;
    case NUMLOCK:
      if (leds & LED_NUMLK) {
	leds &= ~LED_NUMLK;
      } else {
	leds |= LED_NUMLK;
      }
      set_leds();
      break;
    default:
      return 0;
  }
  return 1;
}

/* Interrupts are enabled, here... */
void postprocess(void)
{
  BYTE code, decoded;
  BYTE decode(BYTE c, int f, int lock);

  code = rawqueue_get();

  while (code != 0) {
    /* Decode it... And put it in the keyboard queue */
#if 1
    /*
message("Scancode %d\n", code);
    */
    if (keyb_mode != KEYB_RAW) {
      decoded = decode(code, flags, leds);
      if (decoded == 0) {
	fd32_message("Strange key: %d (0x%x)\n", code, code);
      } else {
	/*
	message("  %c ", code);
	*/
      }
    }
    keyqueue_put(decoded);
#else
    fd32_message("Key code: %d\n", rawcode);
#endif
    code = rawqueue_get();
  }
}

