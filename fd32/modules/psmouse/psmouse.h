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


#ifndef __PSMOUSE_H__
#define __PSMOUSE_H__


#define CMD_WRITE_MOUSE_OUTBUF  0xd3
#define CMD_WRITE_MOUSE         0xd4
#define CMD_ENABLE_MOUSE        0xa8  /* Not needed just use MOUSE_INTERRUPTS_ON? */
#define CMD_READ_MODE           0x20
#define CMD_WRITE_MODE          0x60

#define PSMOUSE_CMD_SETSTREAM   0x00ea
#define PSMOUSE_CMD_GETID       0x02f2
#define PSMOUSE_CMD_SETRATE     0x10f3
#define PSMOUSE_CMD_ENABLE      0x00f4
#define PSMOUSE_CMD_DISABLE     0x00f5
#define PSMOUSE_CMD_RESET_DIS   0x00f6

/* PS/2 Controller mode */
#define MOUSE_INTERRUPTS_ON     (0x40|0x04|0x01|0x02)
#define MOUSE_INTERRUPTS_OFF    (0x40|0x04|0x01|0x20)


#endif
