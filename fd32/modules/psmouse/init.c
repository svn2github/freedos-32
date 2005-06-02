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
#include <errno.h>
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
} psctrl_status_t;


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
} psmouse_info_t;


#define FD32_MOUSE_HIDE            0x00
#define FD32_MOUSE_SHOW            0x01
#define FD32_MOUSE_SHAPE           0x02
#define FD32_MOUSE_GETXY           0x03
#define FD32_MOUSE_GETBTN          0x04
#define FD32_MOUSE_GETBTN_PRESSES  0x05
#define FD32_MOUSE_GETBTN_RELEASES 0x06


/* text mode video memory */
static WORD *v = (WORD *)0xB8000;
/* mouse status */
static int text_x = -1, text_y = -1, x, y, visible;
static fd32_request_t request;
static psmouse_info_t info;
/* text mode cursor and mask */
static WORD save, scrmask = 0x77ff, curmask = 0x7700;
static volatile WORD tmp;

typedef struct psmouse_button_info
{
  WORD presses;
  WORD releases;
  WORD status[2];
} psmouse_button_info_t;
static psmouse_button_info_t btn[3];


typedef struct fd32_mouse_getbutton
{
  WORD button;
  WORD count;
  WORD x;
  WORD y;
} fd32_mouse_getbutton_t;


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
      if(data != NULL) {
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
    case FD32_MOUSE_GETBTN_PRESSES:
      if(data != NULL) {
        fd32_mouse_getbutton_t *p = (fd32_mouse_getbutton_t *)data;
        p[0].count = btn[p[0].button].presses;
        /* Reset the counter */
        btn[p[0].button].presses = 0;
        p[0].x = text_x;
        p[0].y = text_y;
        /* Retrieve the current buttons' status */
        p[0].button = info.raw&0x0007;
      }
      break;
    case FD32_MOUSE_GETBTN_RELEASES:
      if(data != NULL) {
        fd32_mouse_getbutton_t *p = (fd32_mouse_getbutton_t *)data;
        p[0].count = btn[p[0].button].releases;
        /* Reset the counter */
        btn[p[0].button].releases = 0;
        p[0].x = text_x;
        p[0].y = text_y;
        /* Retrieve the current buttons' status */
        p[0].button = info.raw&0x0007;
      }
      break;
    default:
      return -ENOTSUP;
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

  /*
   * status 0: previous button status
   * status 1: new button status
   */
  btn[0].status[1] = info.i.lbutton;
  btn[1].status[1] = info.i.rbutton;
  btn[2].status[1] = info.i.mbutton;
  /* Track the button presses count, status changing from 0 -> 1 */
  btn[0].presses += (!btn[0].status[0])&(btn[0].status[1]);
  btn[1].presses += (!btn[1].status[0])&(btn[1].status[1]);
  btn[2].presses += (!btn[2].status[0])&(btn[2].status[1]);
  /* Track the button releases count, status changing from 1 -> 0 */
  btn[0].releases += (btn[0].status[0])&(!btn[0].status[1]);
  btn[1].releases += (btn[1].status[0])&(!btn[1].status[1]);
  btn[2].releases += (btn[2].status[0])&(!btn[2].status[1]);
  /* Store the buttons statuses */
  btn[0].status[0] = info.i.lbutton;
  btn[1].status[0] = info.i.rbutton;
  btn[2].status[0] = info.i.mbutton;

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
  psctrl_status_t status;

  for(status.data = fd32_inb(0x64); status.data&0x03; status.data = fd32_inb(0x64))
  {
    if(status.s.outbuf_full) {
      tmp = fd32_inb(0x60);
      fd32_log_printf("[PSMOUSE] Retrieve mouse outbuf data ... %x\n", tmp);
    }
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
    /* Set stream, enable, start the interrupt */
    {CMD_WRITE_MOUSE, PSMOUSE_CMD_SETSTREAM},
    {CMD_WRITE_MOUSE, PSMOUSE_CMD_ENABLE},
    {0, 0}
  };
  BYTE code = fd32_inb(0x60);

  switch (code)
  {
    /* ACK */
    case 0xFA:
      if (psmouse_init_seq[stage].cmd != 0)
      {
        tmp_wait();
        fd32_outb(0x64, psmouse_init_seq[stage].cmd);
        tmp_wait();
        fd32_outb(0x60, psmouse_init_seq[stage].data);
      } else if (psmouse_init_seq[stage].data == 0) {
        /* Set the real interrupt handler in the end */
        fd32_irq_bind(12, psmouse_handler);
        stage = 0xFFFFFFFE;
      }
#ifdef __PSMOUSE_DEBUG__
      fd32_log_printf("[PSMOUSE] ACK: %lx code: %x\n", stage, code);
#endif
      stage++;
      break;
    case 0xFE:
      /* Fail the initialization */
      stage = 0xFFFFFFFF;
      fd32_message("[PSMOUSE] Initialization failed!\n");
      break;
    default:
      fd32_message("[PSMOUSE] Unknown code %x\n", code);
      break;
  }

  fd32_sti();
}

void psmouse_init(void)
{
  /* TODO: Check if the PS/2 port exists */

  fd32_message("Start PS/2 mouse initialization ...\n");
  /* TODO: Save the old handler for recovery */
  fd32_irq_bind(12, tmp_handler);

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
  fd32_log_printf("[PSMOUSE] PS/2 CTRL mode is: %x\n", tmp);
#endif

  /* Detect PS/2 mouse */
  tmp_wait();
  fd32_outb(0x64, CMD_WRITE_MOUSE);
  tmp_wait();
  fd32_outb(0x60, (BYTE)PSMOUSE_CMD_GETID);
  tmp_wait();
  fd32_outb(0x64, CMD_WRITE_MOUSE);
  tmp_wait();

  /* Check PS/2 mouse attached or not */
  if (tmp != 0xFE) {
    /* Reset defaults and disable mouse data reporting */
    fd32_outb(0x60, (BYTE)PSMOUSE_CMD_RESET_DIS);

    /* Triger the start phase through a temporary handler then wait for ready */
    WFC (stage != 0xFFFFFFFF);
    
    if (fd32_dev_register(request, 0, "mouse") < 0)
      fd32_message("[PSMOUSE] Could not register the mouse device\n");
  } else {
    fd32_message("[PSMOUSE] Could not find a PS/2 mouse!\n");
  }
}
