/*
 * PS/2 Mouse Driver
 *
 * Make it as simple as it can be ...
 *
 * Hanzac Chen
 * 2004 - 2005
 *
 * Ref: Bochs' keyboard code
 *      Linux's psaux driver
 *      ReactOS's mouse driver
 *      http://www.computer-engineering.org
 *
 * This is free software; see GPL.txts
 */


#include <dr-env.h>
#include <errors.h>
#include <devices.h>

#include "psmouse.h"


typedef union psctrl_status
{
  DWORD data;
  struct
  {
    BYTE outbuf_full       :1;
    BYTE inbuf_full        :1;
    BYTE self_test         :1;
    BYTE command           :1;
    BYTE unlocked          :1;
    BYTE mouse_outbuf_full :1;
    BYTE general_timeout   :1;
    BYTE parity_error      :1;
  } s;
} psctrl_status;


typedef union psmouse_info
{
  struct
  {
    BYTE data[3];
    BYTE index;
  } d;
  struct
  {
    BYTE lbutton           :1;
    BYTE rbutton           :1;
    BYTE mbutton           :1;
    BYTE reserved          :1;
    BYTE xsignbit          :1;
    BYTE ysignbit          :1;
    BYTE xoverflow         :1;
    BYTE yoverflow         :1;
    
    char xmovement;
    char ymovement;
  } i;
  DWORD raw;
} psmouse_info;


#define FD32_MOUSE_HIDE    0x00
#define FD32_MOUSE_SHOW    0x01
#define FD32_MOUSE_SHAPE   0x02
#define FD32_MOUSE_GETXY   0x03
#define FD32_MOUSE_GETBTN  0x04


/* text mode video memory */
static WORD *v = (WORD *)0xB8000;
/* text mode cursor and mask */
static WORD save, reserved, scrmask = 0x77ff, curmask = 0x7700;
/* mouse status */
static int text_x = -1, text_y = -1, x, y, visible;

static fd32_request_t request;
static psmouse_info info;


static int request(DWORD function, void *params)
{
  DWORD *data = (DWORD *)params;
  
  switch(function)
  {
    case FD32_MOUSE_HIDE:
      visible = 0;
      v[80*text_y+text_x] = save;
      break;
    case FD32_MOUSE_SHOW:
      visible = 1;
      save = v[80*text_y+text_x];
      v[80*text_y+text_x] &= scrmask;
      v[80*text_y+text_x] ^= curmask;
      break;
    case FD32_MOUSE_SHAPE:
      if(data != 0) {
        scrmask = data[0]&0x000000ff;
        curmask = data[0]&0x00ff0000;
      } else {
        scrmask = 0xff;
        curmask = 0xff;
      }
      break;
    case FD32_MOUSE_GETXY:
      data[0] = text_x*8;
      data[1] = text_y*8;
      break;
    case FD32_MOUSE_GETBTN:
      data[0] = info.raw;
      break;
    default:
      return FD32_EINVAL;
  }
  
  return 0;
}


static void psmouse_handler(int n)
{
#ifdef __PSMOUSE_DEBUG__
  fd32_log_printf("[PSMOUSE] PS/2 Mouse handler X%d Y%d...\n", x, y);
#endif

  info.d.data[info.d.index] = fd32_inb(0x60);
  info.d.index++;
  
  if(info.d.index > 2) /* The precision */
  {
#ifdef __PSMOUSE_DEBUG__
    fd32_log_printf("[PSMOUSE] L%x R%x M%x #%x XS%x YS%x XO%x YO%x x: %x y: %x\n", info.i.lbutton, info.i.rbutton, info.i.mbutton, info.i.reserved, info.i.xsignbit, info.i.ysignbit, info.i.xoverflow, info.i.yoverflow, info.i.xmovement, info.i.ymovement);
#endif
    info.d.index = 0;
    x += info.i.xmovement;
    y -= info.i.ymovement;
    if (x < 0 || x >= 640 || y < 0 || y >= 200) {
      /* Recover the original coordinates */
      x -= info.i.xmovement;
      y += info.i.ymovement;
    } else if (visible) {
      if(text_x != -1 && text_y != -1)
        v[80*text_y+text_x] = save;
      /* Get the text mode new X and new Y */
      text_x = x/8;
      text_y = y/8;
      /* Save the previous character */
      save = v[80*text_y+text_x];
      /* Display the cursor */
      v[80*text_y+text_x] &= scrmask;
      v[80*text_y+text_x] ^= curmask;
    }
    /* Enqueue the raw data */
  }
  
  fd32_sti();
}


/* The temporary wait only for initialization */
static void tmp_wait(void)
{
  psctrl_status status;

  for(status.data = fd32_inb(0x64); status.data&0x03; status.data = fd32_inb(0x64))
  {
    if(status.s.outbuf_full)
      reserved = fd32_inb(0x60);
  }
}


/* The temporary handler only for initialization */
static DWORD stage = 0;
static void tmp_handler(int n)
{
  static struct PSMOUSEINITSEQ
  {
    WORD cmd;
    WORD data;
  } psmouse_init_seq[] = {
    {CMD_WRITE_MOUSE, PSMOUSE_CMD_SETSTREAM}, /* Set stream, enable, start the interrupt */
    {CMD_WRITE_MOUSE, PSMOUSE_CMD_ENABLE},
    {0, 0}
  };
  BYTE code = fd32_inb(0x60);

  switch (code)
  {
    case 0xFA: /* ACK */
      if (psmouse_init_seq[stage].data != 0)
      {
        tmp_wait();
        fd32_outb(0x64, psmouse_init_seq[stage].cmd);
        tmp_wait();
        fd32_outb(0x60, psmouse_init_seq[stage].data);
      } else {
        /* Set the real interrupt handler in the end */
        fd32_irq_bind(12, psmouse_handler);
        stage = 0xFFFFFFFE;
      }
#ifdef __PSMOUSE_DEBUG__
      fd32_log_printf("[PSMOUSE] ACK: %lx code: %x\n", stage, code);
#endif
      stage++;
      break;
    default:
      fd32_message("[PSMOUSE] Unknown code %x\n", code);
      break;
  }

  fd32_sti();
}

void psmouse_init(void)
{
  reserved = 0;
  fd32_message("Setting PS/2 mouse handler ...\n");
  fd32_irq_bind(12, tmp_handler);

  /* TODO: Detect PS/2 mouse */

  /* Enable mouse interface */
  tmp_wait();
  fd32_outb(0x64, CMD_ENABLE_MOUSE);

  /* Turn on mouse interrupt */
  tmp_wait();
  fd32_outb(0x64, CMD_WRITE_MODE);
  tmp_wait();
  fd32_outb(0x60, MOUSE_INTERRUPTS_ON);

#ifdef __PSMOUSE_DEBUG__
  tmp_wait();
  fd32_outb(0x64, CMD_READ_MODE);
  tmp_wait();
  fd32_log_printf("[PSMOUSE] PS/2 CTRL mode is: %x\n", reserved);
#endif

  /* Reset defaults and disable mouse data reporting */
  tmp_wait();
  fd32_outb(0x64, CMD_WRITE_MOUSE);
  tmp_wait();
  fd32_outb(0x60, PSMOUSE_CMD_RESET_DIS);

  /* Triger the start phase through a temporary handler then wait for ready */
  WFC (stage != 0xFFFFFFFF);

  if (fd32_dev_register(request, 0, "mouse") < 0)
    fd32_message("Could not register the mouse device\n");
}
