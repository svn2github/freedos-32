/* FD32's PS/2 Mouse Driver
 *
 * Copyright (C) 2004-2006 by Hanzac Chen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


#include <dr-env.h>
#include <errno.h>
#include <devices.h>
#include <mouse.h>
#include <timer.h>
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


static fd32_request_t request;
static psmouse_info_t info;
static fd32_mousecallback_t *callback;   /* The user-defined callback */


static int request(DWORD function, void *params)
{
  switch(function)
  {
    case FD32_MOUSE_SETCB:
      callback = (fd32_mousecallback_t *) params;
      return 0;
    default:
      return -ENOTSUP;
  }
}


static void psmouse_handler(int n)
{
#ifdef __PSMOUSE_DEBUG__
  fd32_log_printf("[PSMOUSE] PS/2 Mouse handler ...\n");
#endif

  info.d.data[info.d.index] = fd32_inb(0x60);
  info.d.index++;

  if(info.d.index > 2) /* The precision */
  {
#ifdef __PSMOUSE_DEBUG__
    fd32_log_printf("[PSMOUSE] L%x R%x M%x #%x XS%x YS%x XO%x YO%x x: %x y: %x\n", info.i.lbutton, info.i.rbutton, info.i.mbutton, info.i.reserved, info.i.xsignbit, info.i.ysignbit, info.i.xoverflow, info.i.yoverflow, info.i.xmovement, info.i.ymovement);
#endif
    info.d.index = 0;

    /* Enqueue the raw data */
    if (callback) {
      fd32_mousedata_t data;
      data.flags = 2;
      data.buttons = info.raw&0x0F;
      data.axes[MOUSE_X] = info.i.xmovement;
      data.axes[MOUSE_Y] = info.i.ymovement;
      
      callback(&data);
    }
  }
  
  fd32_sti();
}


/*
 * The i8042_wait_read() and i8042_wait_write functions wait for the i8042 to
 * be ready for reading values from it / writing values to it.
 * Called always with i8042_lock held.
 */
#define I8042_CTL_TIMEOUT	10000

static int i8042_wait_read(void)
{
	int i = 0;
	psctrl_status_t status;

	status.data = fd32_inb(0x64);
	while (!status.s.outbuf_full && (i < I8042_CTL_TIMEOUT)) {
		timer_delay(50);
		status.data = fd32_inb(0x64);
		i++;
	}
	return -(i == I8042_CTL_TIMEOUT);
}

static int i8042_wait_write(void)
{
	int i = 0;
	psctrl_status_t status;

	status.data = fd32_inb(0x64);
	while (status.s.inbuf_full && (i < I8042_CTL_TIMEOUT)) {
		timer_delay(50);
		status.data = fd32_inb(0x64);
		i++;
	}
	return -(i == I8042_CTL_TIMEOUT);
}


/* The temporary handler only for initialization */
static volatile DWORD stage = 0;
static void tmp_handler(int n)
{
  static struct PSMOUSEINITSEQ
  {
    WORD cmd;
    WORD data;
  } psmouse_init_seq[] = {
    /* {CMD_WRITE_MOUSE, PSMOUSE_CMD_GETID}, */
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
        i8042_wait_write();
        fd32_outb(0x64, psmouse_init_seq[stage].cmd);
        i8042_wait_write();
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
      fd32_log_printf("[PSMOUSE] Initialization failed!\n");
      break;
    default:
      fd32_message("[PSMOUSE] Unknown code %x\n", code);
      fd32_log_printf("[PSMOUSE] Unknown code %x\n", code);
      break;
  }

  fd32_sti();
}

void psmouse_init(void)
{
  /* TODO: Check if the PS/2 port exists */

  fd32_message("Start PS/2 mouse initialization ...\n");

  /* Enable mouse interface */
  i8042_wait_write();
  fd32_outb(0x64, CMD_ENABLE_MOUSE);
  /* Turn on mouse interrupt */
  i8042_wait_write();
  fd32_outb(0x64, CMD_WRITE_MODE);
  i8042_wait_write();
  fd32_outb(0x60, MOUSE_INTERRUPTS_ON);
#ifdef __PSMOUSE_DEBUG__
  i8042_wait_write();
  fd32_outb(0x64, CMD_READ_MODE);
  i8042_wait_read();
  volatile int tmp = fd32_inb(0x60);
  fd32_log_printf("[PSMOUSE] PS/2 CTRL mode is: %x\n", tmp);
#endif

  /* TODO: Save the old handler for recovery */
  fd32_irq_bind(PS2MOUSE_IRQ, tmp_handler);

  /* TODO: Detect PS/2 mouse and check PS/2 mouse attached or not */
  /* fd32_message("[PSMOUSE] Could not find a PS/2 mouse!\n"); */

  i8042_wait_write();
  fd32_outb(0x64, CMD_WRITE_MOUSE);

  /* Reset defaults and disable mouse data reporting */
  i8042_wait_write();
  fd32_outb(0x60, (BYTE)PSMOUSE_CMD_RESET_DIS);

  /* Triger the start phase through a temporary handler then wait for ready */
  WFC (stage != 0xFFFFFFFF);

  if (fd32_dev_register(request, 0, "mouse") < 0)
    fd32_message("[PSMOUSE] Could not register the mouse device\n");

  fd32_message("PS/2 mouse initialized!\n");
}
