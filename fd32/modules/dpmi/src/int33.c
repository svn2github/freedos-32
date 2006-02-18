/* DPMI Driver for FD32: int 0x33 services
 * by Hanzac Chen
 *
 * This is free software; see GPL.txt
 */
 
#include <ll/i386/hw-data.h>
#include <ll/i386/mem.h>
#include <ll/i386/error.h>
#include <devices.h>
#include <errno.h>
#include <logger.h>
#include "rmint.h"
#include <mouse.h>


#ifdef __INT33_DEBUG__
#define LOG_PRINTF(s, ...) fd32_log_printf("[MOUSE BIOS] "s, ## __VA_ARGS__)
#else
#define LOG_PRINTF(s, ...)
#endif


/* TODO: Make a common mouse header, define and put the request structures there */
typedef struct fd32_mouse_getbutton
{
	WORD button;
	WORD count;
	WORD x;
	WORD y;
} fd32_mouse_getbutton_t;

/* text mode video memory */
typedef WORD (*text_screen_t)[][80];
static text_screen_t screen = (text_screen_t)0xB8000;
/* mouse status */
static volatile int text_x = 0, text_y = 0, x, y;
static volatile WORD buttons;
/* text mode cursor and mask */
static WORD save = 0, scrmask = 0x77ff, curmask = 0x7700;

static void pos(int posx, int posy)
{
	x = posx;
	y = posy;

	/* Get the text mode new X and new Y */
	text_x = x/8;
	text_y = y/8;
	/* Save the previous character */
	save = (*screen)[text_y][text_x];
	/* Display the cursor */
	(*screen)[text_y][text_x] &= scrmask;
	(*screen)[text_y][text_x] ^= curmask;
}

static void cb(const fd32_mousedata_t *data)
{
	buttons = data->buttons;
	x += data->axes[MOUSE_X];
	y -= data->axes[MOUSE_Y];

	if (x < 0 || x >= 640 || y < 0 || y >= 200) {
		/* Recover the original coordinates */
		x -= data->axes[MOUSE_X];
		y += data->axes[MOUSE_Y];
	} else {
		if(save != 0)
			(*screen)[text_y][text_x] = save;
		pos(x, y);
	}
}

int _mousebios_init(void)
{
	fd32_request_t *request;
	int res;

	if ((res = fd32_dev_search("mouse")) < 0)
	{
		LOG_PRINTF("no mouse driver\n");
		return -1;
	}

	if (fd32_dev_get(res, &request, NULL, NULL, 0) < 0)
	{
		LOG_PRINTF("no mouse request calls\n");
		return -1;
	}
	res = request(FD32_MOUSE_SETCB, cb);

	return res;
}


int mousebios_int(union rmregs *r)
{
	int res = 0;

	switch(r->x.ax)
	{
		/* MS MOUSE - RESET DRIVER AND READ STATUS */
		case 0x0000:
#ifdef __INT33_DEBUG__
			LOG_PRINTF("Reset mouse driver\n");
#endif
			r->x.ax = 0xffff;
			r->x.bx = 0x0002;
			break;
		/* MS MOUSE v1.0+ - SHOW MOUSE CURSOR */
		case 0x0001:
#ifdef __INT33_DEBUG__
			LOG_PRINTF("Show Mouse cursor\n");
#endif
			save = (*screen)[text_y][text_x];
			(*screen)[text_y][text_x] &= scrmask;
			(*screen)[text_y][text_x] ^= curmask;
			break;
		/* MS MOUSE v1.0+ - HIDE MOUSE CURSOR */
		case 0x0002:
#ifdef __INT33_DEBUG__
			LOG_PRINTF("Hide mouse cursor\n");
#endif
			(*screen)[text_y][text_x] = save;
			break;
		/* MS MOUSE v1.0+ - RETURN POSITION AND BUTTON STATUS */
		case 0x0003:
			r->x.bx = buttons&0x07;
			r->x.cx = x;
			r->x.dx = y;
			if(r->x.bx != 0)
			fd32_log_printf("[MOUSE BIOS] Button clicked, cx: %x\tdx: %x\tbx: %x\n", r->x.cx, r->x.dx, r->x.bx);
#ifdef __INT33_DEBUG__
			if(r->x.bx != 0)
				fd32_log_printf("[MOUSE BIOS] Button clicked, cx: %x\tdx: %x\tbx: %x\n", r->x.cx, r->x.dx, r->x.bx);
#endif
			break;
		/* MS MOUSE v1.0+ - POSITION MOUSE CURSOR */
		case 0x0004:
			pos(r->x.cx, r->x.dx);
			break;
		/* MS MOUSE v1.0+ - RETURN BUTTON PRESS DATA */
		case 0x0005:
			r->x.ax = buttons; /* button states */
			r->x.bx = 0; 
			r->x.cx = text_x;
			r->x.dx = text_x;
			break;
		/* MS MOUSE v1.0+ - RETURN BUTTON RELEASE DATA */
		case 0x0006:
			r->x.ax = buttons; /* button states */
			r->x.bx = 0; 
			r->x.cx = text_x;
			r->x.dx = text_x;
			break;
		/* MS MOUSE v1.0+ - DEFINE HORIZONTAL CURSOR RANGE */
		case 0x0007:
			/* r->x.cx = 0; minimum column */
			/* r->x.dx = 639; maximum column */
			res = 0; /* TODO: malfunction */
			break;
		/* MS MOUSE v1.0+ - DEFINE VERTICAL CURSOR RANGE */
		case 0x0008:
			/* r->x.cx = 0; minimum row */
			/* r->x.dx = 479; maximum row */
			res = 0; /* TODO: malfunction */
			break;
		/* MS MOUSE v3.0+ - DEFINE TEXT CURSOR */
		case 0x000A:
			if (r->x.bx == 0) {
				scrmask = r->x.cx;
				curmask = r->x.dx;
			} else {
				scrmask = 0xff;
				curmask = 0xff;
			}
			break;
		/* MS MOUSE v1.0+ - DEFINE INTERRUPT SUBROUTINE PARAMETERS */
		case 0x000C:
#ifdef __INT33_DEBUG__
			LOG_PRINTF("DEFINE INTERRUPT SUBROUTINE PARAMETERS: %x\n", r->x.cx);
#endif
			res = 0; /* TODO: malfunction */
			break;
		/* Genius Mouse 9.06 - GET NUMBER OF BUTTONS */
		case 0x0011:
			r->x.ax = 0xFFFF;
			r->x.bx = 2; /* Number of buttons */
			break;
		/* MS MOUSE v6.0+ - RETURN DRIVER STORAGE REQUIREMENTS */
		case 0x0015:
#ifdef __INT33_DEBUG__
			LOG_PRINTF("RETURN DRIVER STORAGE REQUIREMENTS NOT USED, SIZE SET TO ZERO\n");
#endif
			r->x.bx = 0;
			break;
		/* MS MOUSE v6.0+ - SOFTWARE RESET */
		case 0x0021:
#ifdef __INT33_DEBUG__
			LOG_PRINTF("Software reset mouse driver\n");
#endif
			r->x.ax = 0xffff;
			r->x.bx = 0x0002;
			break;
		default:
			message("[MOUSE BIOS] Unimplemeted INT 33H AL=0x%x!!!\n", r->h.al);
			res = 0;
			break;
	}
	
	return res;
}
