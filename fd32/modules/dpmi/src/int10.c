/* DPMI Driver for FD32: int 0x10 services
 * by Luca Abeni
 *
 * This is free software; see GPL.txt
 */
 
#include <ll/i386/hw-data.h>
#include <ll/i386/hw-instr.h>
#include <ll/i386/hw-func.h>
#include <ll/i386/x-bios.h>
#include <ll/i386/mem.h>
#include <ll/i386/string.h>
#include <ll/i386/error.h>
#include <ll/i386/cons.h>
#include <logger.h>
#include <kernel.h>
#include "rmint.h"

#define CURSOR_START    0x0A
#define CURSOR_END      0x0B
#define CGA_INDEX_REG   0x3D4
#define CGA_DATA_REG    0x3D5

int videobios_int(union rmregs *r)
{
  int res = 0, x, y;
  switch (r->h.ah) {
    case 0x01:
	  cursor(r->h.ch&0x0f, r->h.cl&0x0f);
      break;;

    case 0x02:
      /* Set Cursor Position */
      x = r->h.dl;
      y = r->h.dh;
      place(x, y);
      RMREGS_CLEAR_CARRY;
      break;

    case 0x03:
      /* Get Cursor Position and Size...*/
      /* BH is the page number... For now, don't care about it! */
      /*
        r->ch = 0x06;
        r->cl = 0x07;
      */
      r->x.cx = 0x0607;
      /* CH - CL are the start and end scanline for the cursor (!!?)
       * The previous values 0x06 and 0x07 are hopefully the standard
       * ones...
       */
      getcursorxy(&x, &y);
      /*
        r->dh = y;
        r->dl = x;
      */
      r->h.dh = y;
      r->h.dl = x;
      RMREGS_CLEAR_CARRY;
      break;

    case 0x08:
      /* Read Character and Attribute */
      /* Let's just return a reasonable attribute... */
      r->h.ah = get_attr();
      r->h.al = ' ';
      RMREGS_CLEAR_CARRY;
      break;
      
    /* Video - Teletype output */
    case 0x0E:
      /* AL = Character to write */
      /* BH = Page number        */
      /* BL = Foreground color   */
      /* TODO: page, colors and special BEL character (07h) are ignored */
      if (r->h.al != 0x07) cputc(r->h.al);
      break;

    /* VIDEO - GET CURRENT VIDEO MODE */
    case 0x0F:
      r->h.al = 0x03;
      r->h.ah = 80;
      r->h.bh = 0;
      break;

    /* BIOS Window Extension v1.1 - SET WINDOW COORDINATES */
    case 0x10:
      fd32_log_printf("INT 10h AH=10h VIDEO not implemented\n");
      break;
    
    /* BIOS Window Extension v1.1 - GET BLANKING ATTRIBUTE */
    case 0x12:
      fd32_log_printf("Unimplemeted INT 0x10 AX=%x\n", r->x.ax);
      break;

    case 0x1A:
      if (r->h.al != 0x00)
        r->h.al = 0;
      /* Get display combination mode (???)
       * let's say that it is used for checking if the video card is a VGA...
       */
      r->h.al = 0x1A;
      /* 7 Should be VGA!!! */
      /* Alternate Display Code (BH) and Active Display Code (BL) */
      r->x.bx = 0x0707;
      break;

    case 0x4F:
      /* VESA SuperVGA BIOS - GET CURRENT VIDEO MODE */
      if (r->h.al == 0x03)
      {
        r->h.al = 0x4f;
        r->h.ah = 0x00; /* successful */
        r->x.bx = 0x0108;
      } else {
        r->h.ah = 0x01; /* failed */
      }
      break;

    case 0xFE:
      /* Get Shadow Buffer (???) */
      /* For the moment, we don't have it... Return 0! */
      r->x.es = 0;
      r->d.edi = 0;
      break;

    /* DJ GO32.EXE 80386+ DOS extender - VIDEO EXTENSIONS */
    case 0xFF:
      /* Not clear what to do here... */
      RMREGS_SET_CARRY; /* So, fail... */
      break;

    default:
    if (r->h.ah == 0) {
      DWORD f;
      X_REGS16 r16 = { {r->x.ax, r->x.bx, r->x.cx, r->x.dx, r->x.si, r->x.di, r->x.flags} };
      X_SREGS16 sr16 = {r->x.es, r->x.cs, r->x.ss, r->x.ds};
      f = ll_fsave();
      vm86_callBIOS(0x10, &r16, &r16, &sr16);
      ll_frestore(f);
    } else {
      error("Unimplemeted INT!!!\n");
      message("INT 10, AX = 0x%x\n", r->x.ax);
      fd32_abort();
    }
  }

  return res;
}
