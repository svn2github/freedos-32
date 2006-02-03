/*****************************************************************************
 * The FreeDOS 32 Serial Mouse Driver                                        *
 * Copyright (C) 2004, Salvatore Isaja                                       *
 *   project web site: http://freedos-32.sourceforge.net                     *
 *   e-mail: salvois at users.sourceforge.net                                *
 *                                                                           *                                                         
 * This program is free software; you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License as published by      *
 * the Free Software Foundation; either version 2 of the License, or         *
 * (at your option) any later version.                                       *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License for more details.                              *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with this program; see the file COPYING.txt;                        *
 * if not, write to the Free Software Foundation, Inc.,                      *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *                 *
 *****************************************************************************/

#include <dr-env.h>
typedef signed long LONG;
typedef signed char CHAR;
#ifndef __cplusplus
typedef enum { false = 0, true = 1 } bool;
#endif
#include <devices.h>
#include <errno.h>
#include <mouse.h>
#include "pit/pit.h"

typedef void *  Fd32Event;


#if 0
#define NUM_COM_PORTS 4
static struct
{
    const DWORD    bios; /* Physical addresses of BIOS data for port address */
    const unsigned irq;  /* IRQ number of the port                           */
    unsigned       port; /* Port number (as got from BIOS data area)         */
}
com_ports[NUM_COM_PORTS] =
{
    { 0x400, 0, 4 }, /* COM1 */
    { 0x402, 0, 3 }, /* COM2 */
    { 0x404, 0, 4 }, /* COM3 */
    { 0x406, 0, 3 }  /* COM4 */
};



typedef enum
{
    PROT_NO, /* Uknown/no mouse       */
    PROT_M,  /* 2-buttons MS protocol */
    PROT_M3, /* 3-buttons MS protocol */
    PROT_MZ, /* Mouse wheel protocol  */
    PROT_H   /* Mouse System protocol */
}
Protocol;
#endif

typedef void Handler(int n);

static bool           installed = false; /* Set if handler installed     */
static fd32_mousecallback_t *callback;   /* The user-defined callback    */
static unsigned       port = 0;          /* Base address of the COM port */


/* The serial port interrupt handler, MS (2-button) protocol.             */
/*                                                                        */
/* When a mouse event occurs, three bytes are received from the COM port, */
/* and an interrupt is generated for each of them:                        */
/*    1 L R Y7 Y6 X7 X6    0 X5 X4 X3 X2 X1 X0    0 Y5 Y4 Y3 Y2 Y1 Y0     */
/* Bit 7 is fake because the mouse does a 7-bit data transfer.            */
/* The first byte has bit 6 set for sequence identification.              */
/* L and R are left and right buttons status (set if pressed).            */
/* Y and X, packed in 8-bit signed integers, are the increment of the     */
/* mouse position (in mouse units) meausured by the mouse for that event. */
/* When all three bytes are received, a fd32_mousedata_t structure is prepared   */
/* and passed to a user-defined callback function.                        */
static void m_handler(int n)
{
    static BYTE     bytes[3];
    static unsigned count = 4;
    fd32_mousedata_t       data;

    /* TODO: Should we check for interrupt reason (register at base_port + 2)? */
    /* Check for Data Ready on the specified COM port */
    if (fd32_inb(port + 5) & 1)
    {
        BYTE b = fd32_inb(port);
        if (b & (1 << 6)) count = 0;
        if (count < 3) bytes[count++] = b;
        if (count == 3)
        {
            data.flags         = 2; /* 2 axes */
            data.axes[MOUSE_X] = (LONG) (CHAR) (((bytes[0] & 0x03) << 6) | (bytes[1] & 0x3F));
            data.axes[MOUSE_Y] = (LONG) (CHAR) (((bytes[0] & 0x0C) << 4) | (bytes[2] & 0x3F));
            data.buttons       = 0;
            if (bytes[0] & (1 << 5)) data.buttons |= MOUSE_LBUT;
            if (bytes[0] & (1 << 4)) data.buttons |= MOUSE_RBUT;
            if (callback) callback(&data);
            count++;
        }
    }
    fd32_master_eoi();
}


/* The serial port interrupt handler, MS protocol with wheel support.    */
/*                                                                       */
/* Same as standard (2-buttons) MS protocol, but a fourth byte is sent   */
/* when the middle button is active or is changing status:               */
/*                            0 M 0 0 0 0 0                              */
/* M is the middle button status (set if pressed).                       */
/* When all four bytes are received, a fd32_mousedata_t structure is prepared   */
/* and passed to a user-defined callback function.                       */
static void m3_handler(int n)
{
    static BYTE      bytes[4];
    static unsigned  count = 5;
    static fd32_mousedata_t data;

    /* TODO: Should we check for interrupt reason (register at base_port + 2)? */
    /* Check for Data Ready on the specified COM port */
    if (fd32_inb(port + 5) & 1)
    {
        BYTE b = fd32_inb(port);
        if (b & (1 << 6)) count = 0;
        if (count < 4) bytes[count++] = b;
        if (count == 3)
        {
            data.flags         = 2; /* 2 axes */
            data.axes[MOUSE_X] = (LONG) (CHAR) (((bytes[0] & 0x03) << 6) | (bytes[1] & 0x3F));
            data.axes[MOUSE_Y] = (LONG) (CHAR) (((bytes[0] & 0x0C) << 4) | (bytes[2] & 0x3F));
            data.buttons       = 0;
            if (bytes[0] & (1 << 5)) data.buttons |= MOUSE_LBUT;
            if (bytes[0] & (1 << 4)) data.buttons |= MOUSE_RBUT;
            if (callback) callback(&data);
            count++;
        }
        if (count == 4)
        {
            data.buttons &= ~MOUSE_MBUT;
            if (bytes[3] & (1 << 5)) data.buttons |= MOUSE_MBUT;
            if (callback) callback(&data);
            count++;
        }
    }
    fd32_master_eoi();
}


/* The serial port interrupt handler, MS protocol with wheel support.    */
/*                                                                       */
/* When a mouse event occurs, four bytes are received from the COM port, */
/* and an interrupt is generated for each of them. The first three are   */
/* as in standard (2-buttons) MS protocol. The fourth byte is:           */
/*                          0 0 M Z3 Z2 Z1 Z0                            */
/* M is the middle button status (set if pressed).                       */
/* Z3..Z0 is a 4-bit two's complement increment of the mouse wheel.      */
/* When all four bytes are received, a fd32_mousedata_t structure is prepared   */
/* and passed to a user-defined callback function.                       */
static void mz_handler(int n)
{
    static BYTE     bytes[4];
    static unsigned count = 5;
    fd32_mousedata_t       data;

    /* TODO: Should we check for interrupt reason (register at base_port + 2)? */
    /* Check for Data Ready on the specified COM port */
    if (fd32_inb(port + 5) & 1)
    {
        BYTE b = fd32_inb(port);
        if (b & (1 << 6)) count = 0;
        if (count < 4) bytes[count++] = b;
        if (count == 4)
        {
            data.flags         = 3; /* 3 axes */
            data.axes[MOUSE_X] = (LONG) (CHAR) (((bytes[0] & 0x03) << 6) | (bytes[1] & 0x3F));
            data.axes[MOUSE_Y] = (LONG) (CHAR) (((bytes[0] & 0x0C) << 4) | (bytes[2] & 0x3F));
            data.axes[MOUSE_Z] = (((LONG) bytes[3] & 0x0F) << 28) >> 28;
            data.buttons       = 0;
            if (bytes[0] & (1 << 5)) data.buttons |= MOUSE_LBUT;
            if (bytes[0] & (1 << 4)) data.buttons |= MOUSE_RBUT;
            if (bytes[3] & (1 << 4)) data.buttons |= MOUSE_MBUT;
            if (callback) callback(&data);
            count++;
        }
    }
    fd32_master_eoi();
}


/* The serial port interrupt handler, Mouse System protocol.                */
/*                                                                          */
/* When a mouse event occurs, five bytes are received from the COM port,    */
/* and an interrupt is generated for each of them:                          */
/*     1  0  0  0  0  L  M  R                                               */
/*    X7 X6 X5 X4 X3 X2 X1 X0     Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0                   */
/*    x7 x6 x5 x4 x3 x2 x1 x0     y7 y6 y5 y4 y3 y2 y1 y0                   */
/* L, M and R are the three buttons state (zero if pressed).                */
/* X7..X0 and Y7..Y0 are the 8-bit two's complement increments of the mouse */
/* position since the last packet, while x7..x0 and y7..y0 are increments   */
/* since X7..X0 and Y7..Y0 of the same packet.                              */
/* When all three bytes are received, a fd32_mousedata_t structure is prepared   */
/* and passed to a user-defined callback function.                        */
static void h_handler(int n)
{
    static BYTE     bytes[5];
    static unsigned count = 6;
    fd32_mousedata_t       data;

    /* TODO: Should we check for interrupt reason (register at base_port + 2)? */
    /* Check for Data Ready on the specified COM port */
    if (fd32_inb(port + 5) & 1)
    {
        BYTE b = fd32_inb(port);
        if ((b & 0xF8) == 0x80) count = 0;
        if (count < 5) bytes[count++] = b;
        if (count == 5)
        {
            data.flags         = 2; /* 2 axes */
            data.axes[MOUSE_X] = ((LONG) (CHAR) bytes[1]) + ((LONG) (CHAR) bytes[3]);
            data.axes[MOUSE_Y] = ((LONG) (CHAR) bytes[2]) + ((LONG) (CHAR) bytes[4]);
            data.buttons       = 0;
            if (!(bytes[0] & (1 << 2))) data.buttons |= MOUSE_LBUT;
            if (!(bytes[0] & (1 << 1))) data.buttons |= MOUSE_MBUT;
            if (!(bytes[0] & (1 << 0))) data.buttons |= MOUSE_RBUT;
            if (callback) callback(&data);
            count++;
        }
    }
    fd32_master_eoi();
}


/* Callback function for timer driven events */
static void timer_cb(void *params)
{
    *((bool *) params) = true; /* Signal timeout */
}


/* Installs the mouse interrupt handler and sets the COM port for mouse  */
/* data transfer: 1200 bps, no parity, 7 data bits, 1 stop bit, no FIFO. */
static int install(unsigned base_port, unsigned irq, fd32_mousecallback_t *cb)
{
    /* To manage timeout for mouse identification string */
    volatile bool timeout = false;
    Fd32Event     e;
    BYTE          id[300] = { 0, 0 };
    unsigned      n = 0;
    Handler      *handler = NULL;

    if (installed) return -1;

    /* Power down the mouse (by lowering RTS, and also DTR, OUT1 and OUT2) */
    fd32_outb(base_port + 4, 0x00);
    /* Disable COM interrupts for initialization */
    fd32_outb(base_port + 1, 0);
    /* Set baud date to 1200 bps (that is 115200 / 96) */
    fd32_outb(base_port + 3, 0x80); /* Set Divisor Latch bit (DLAB) */
    fd32_outb(base_port + 0, 96);   /* Divisor Latch low byte       */
    fd32_outb(base_port + 1, 0);    /* Divisor Latch high byte      */
    /* Communication parameters */
    fd32_outb(base_port + 3, 0x02); /* Clear DLAB, 7-bit, no parity, 1 stop bit */
    fd32_outb(base_port + 2, 0x00); /* Disable FIFO                             */
    fd32_inb (base_port + 0);       /* Flush input buffer                       */
    /* Power up the mouse (by raising RTS, and also DTR, OUT1 and OUT2) */
    pit_delay(200);
    fd32_outb(base_port + 4, 0x0F);
    
    /* Read mouse identification string */
    e = pit_event_register(2200, timer_cb, (void *) &timeout);
    /* TODO: Check for FD32_EVENT_NULL */
    for (;;)
    {
        BYTE c;
        WFC(!(fd32_inb(base_port + 5) & 1) && !timeout);
        if (timeout) break;
        c = fd32_inb(base_port + 0);
        id[n] = c;
        //if (++n == 2) break;
        fd32_message("%c", n++ < 2 ? c : c + 0x20);
    }
    fd32_message("\n");
    pit_event_cancel(e);
    switch (id[0])
    {
        case 'M': fd32_message("MS protocol mouse found on port %x.\n", base_port);
                  handler = m_handler;
                  switch (id[1])
                  {
                      case '3': fd32_message("Mouse has 3 buttons.\n");
                                handler = m3_handler;
                                break;
                      case 'Z': fd32_message("Mouse has wheel.\n");
                                handler = mz_handler;
                                break;
                  }
                  break;
        case 'H': fd32_message("Mouse System protocol mouse found on port %x.\n", base_port);
                  fd32_outb(base_port + 3, 0x03); /* Clear DLAB, 8-bit, no parity, 1 stop bit */
                  handler = h_handler;
                  break;
    }

    /* Set the new interrupt handler and enable COM interrupt on Data Ready*/
    if (!handler) return -1;
    fd32_irq_bind(irq, handler);
    callback  = cb;
    installed = true;
    port      = base_port;
    fd32_outb(base_port + 1, 0x01);
    return 0;
}


static int uninstall(void)
{
    if (!installed) return -1;
    #ifdef DJGPP_DPMI
    /* Restore old IRQ handler */
    outportb(0x21, masksave);
    _go32_dpmi_set_protected_mode_interrupt_vector(COM1_IRQ, &oldint);
    _go32_dpmi_free_iret_wrapper(&newint);
    #endif
    callback  = 0;
    installed = false;
    return 0;
}


static fd32_request_t request;
static int request(DWORD function, void *params)
{
    switch (function)
    {
        case FD32_MOUSE_SETCB:
        {
            callback = (fd32_mousecallback_t *) params;
            return 0;
        }
    }
    return -ENOTSUP;
}



/******************************************************************************/
/*                                ENTRY POINT                                 */
/******************************************************************************/


#define DEV_NAME "mouse"
void sermouse_init(void)
{
    unsigned base_port = 0x3F8;
    unsigned irq       = 4;

    if (install(base_port, irq, 0) < 0)
    {
        fd32_message("Could not install the Serial Mouse Driver\n");
        return;
    }
    if (fd32_dev_register(request, 0, DEV_NAME) < 0)
    {
        fd32_message("Could not register the mouse device\n");
        return;
    }
}
