/* DPMI Driver for FD32: int 0x10 services
 * by Luca Abeni
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
#include <logger.h>
#include <kernel.h>
#include "rmint.h"

int videobios_int(union rmregs *r)
{
  int x, y;
  switch (r->h.ah) {
    case 0x200:
      /* Set Cursor Position */
      x = r->h.dl;
      y = r->h.dh;
      place(x, y);
      RMREGS_CLEAR_CARRY;
      return 0;

    case 0x300:
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
      return 0;

    case 0x800:
      /* Read Character and Attribute */
      /* Let's just return a reasonable attribute... */
      r->h.ah = get_attr();
      r->h.al = ' ';
      RMREGS_CLEAR_CARRY;
      return 0;

    case 0x1A00:
      /* Get display combination mode (???)
       * let's say that it is used for checking if the video card is a VGA...
       */
      /*
	r->al = 0x1A;
      */
      r->h.al = 0x1A;
#if 0
      r->bl = 0; /* Active Display Code... For the moment, we return 7*/
      r->bh = 0; /* Alternate Display Code */
      /* 7 Should be VGA!!! */
#endif
      r->x.bx = 0x0707;
      return 0;

    case 0xFE00:
      /* Get Shadow Buffer (???)
       */
      /* For the moment, we don't have it... Return 0! */
      r->x.es = 0;
      r->d.edi = 0;
      return 0;

    case 0xFF00:
      /* Not clear what to do here... */
      RMREGS_SET_CARRY; /* So, fail... */
      return 0;

    default:
      error("Unimplemeted INT!!!\n");
      message("INT 10, AX = 0x%x\n", r->x.ax);
      fd32_abort();
  }
  
  return 1;
}
