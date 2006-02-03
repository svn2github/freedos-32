#ifndef __MOUSE_H__
#define __MOUSE_H__

/* Flags for struct MouseData */
enum
{
    MOUSE_AXES = 0xFF,   /* MouseData.flags:   bit mask for number of axes   */
    MOUSE_LBUT = 1 << 0, /* MouseData.buttons: left button flag              */
    MOUSE_RBUT = 1 << 1, /* MouseData.buttons: right button flag             */
    MOUSE_MBUT = 1 << 2, /* MouseData.buttons: middle button flag            */
    MOUSE_X    = 0,      /* MouseData.axes:    index for X (horizontal) axis */
    MOUSE_Y    = 1,      /* MouseData.axes:    index for Y (vertical) axis   */
    MOUSE_Z    = 2,      /* MouseData.axes:    index for Z (wheel) axis      */
};


/* This structure is passed by the serial port interrupt handler */
/* to the user-defined callback function, and contains all data  */
/* sent by the mouse on last mouse event.                        */
typedef struct fd32_mousedata
{
    DWORD flags;   /* bits 0-7: number of axes, others 0      */
    DWORD buttons; /* buttons flags (1=pressed). LSB=button 0 */
    long  axes[3]; /* Array (variable length) axis increments */
} fd32_mousedata_t;


/* This is the prototype for the user-defined callback function */
/* called by the serial port interrupt handler.                 */
typedef void fd32_mousecallback_t(const fd32_mousedata_t *data);

/* Mouse functions */
#define FD32_MOUSE_SETCB 0x300 /* in: params = MouseCallback *callback */

#endif

