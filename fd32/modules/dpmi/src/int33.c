/* DPMI Driver for FD32: int 0x33 services
 * by Hanzac Chen
 *
 * This is free software; see GPL.txt
 */
 
#include <ll/i386/hw-data.h>
#include <ll/i386/hw-instr.h>
#include <ll/i386/hw-func.h>
#include <ll/i386/mem.h>
#include <ll/i386/string.h>
#include <ll/i386/error.h>
#include <ll/i386/cons.h>
#include <devices.h>
#include <logger.h>
#include "rmint.h"


#define __INT33_DEBUG__


#ifdef __INT33_DEBUG__
#define LOG_PRINTF(s) fd32_log_printf("[MOUSE BIOS] "s)
#else
#define LOG_PRINTF(s) asm("")
#endif


#define FD32_MOUSE_HIDE		0x00
#define FD32_MOUSE_SHOW		0x01
#define FD32_MOUSE_SHAPE	0x02
#define FD32_MOUSE_GETXY	0x03
#define FD32_MOUSE_GETBTN	0x04	/* The low 3 bits of retrieved data is the button status */


static fd32_request_t *request;
static int hdev;
static int res;


int mousedev_get(void)
{
	if(hdev == 0)
		if((hdev = fd32_dev_search("mouse")) < 0)
			{ LOG_PRINTF("no mouse driver\n"); return 1; }
	if(request == 0)
		if((res = fd32_dev_get(hdev, &request, NULL, NULL, 0)) < 0)
			{ LOG_PRINTF("no mouse request calls\n"); return 1; }
	
	return 0;
}


int mousebios_int(union rmregs *r)
{
	if(r->h.ah == 0x00)
	{
		switch(r->h.al)
		{
			case 0x00:
#ifdef __INT33_DEBUG__
				LOG_PRINTF("Reset mouse driver\n");
#endif
				r->x.ax = 0xffff;
				r->x.bx = 0x0002;
				break;
			case 0x01:
#ifdef __INT33_DEBUG__
				LOG_PRINTF("Show Mouse cursor\n");
#endif
				if(mousedev_get() == 0)
				{
					res = request(FD32_MOUSE_SHOW, 0);
					if(res < 0) { LOG_PRINTF("no FD32_MOUSE_SHOW request call\n"); return 1; }
				}
				break;
			case 0x02:
#ifdef __INT33_DEBUG__
				LOG_PRINTF("Hide mouse cursor\n");
#endif
				if(mousedev_get() == 0)
				{
					res = request(FD32_MOUSE_HIDE, 0);
					if(res < 0) { LOG_PRINTF("no FD32_MOUSE_HIDE request call\n"); return 1; }
				}
				break;
			case 0x03:
				if(mousedev_get() == 0)
				{
					DWORD raw, pos[2];
					res = request(FD32_MOUSE_GETBTN, (void *)&raw);
					if(res < 0) { LOG_PRINTF("no FD32_MOUSE_GETBTN request call\n"); return 1; }
					res = request(FD32_MOUSE_GETXY, (void *)pos);
					if(res < 0) { LOG_PRINTF("no FD32_MOUSE_GETXY request call\n"); return 1; }
					r->x.bx = raw&0x00000007;
					r->x.cx = pos[0];
					r->x.dx = pos[1];
					if(r->x.bx != 0)
						fd32_log_printf("cx: %d, dx: %d bx: %x\n", r->x.cx, r->x.dx, r->x.bx);
				}
				break;
			case 0x0a:
				fd32_log_printf("Unimplemeted INT 33H AH=0x00 AL=0x%x\n", r->h.al);
				if(mousedev_get() == 0)
				{
					if(r->x.bx == 0)
					{
						WORD mask[2] = {r->x.cx, r->x.dx};
						res = request(FD32_MOUSE_SHAPE, (void *)mask);
					}
					else
						res = request(FD32_MOUSE_SHAPE, 0);
					if(res < 0) { LOG_PRINTF("no FD32_MOUSE_SHAPE request call\n"); return 1; }
				}
				break;
			case 0x21:
#ifdef __INT33_DEBUG__
				LOG_PRINTF("Software reset mouse driver\n");
#endif
				r->x.ax = 0xffff;
				r->x.bx = 0x0002;	
				break;
			default:
				fd32_log_printf("Unimplemeted INT 33H AL=0x%x!!!\n", r->h.al);
				break;
		}
		return 0;
	}
	else
		return 1;
}
