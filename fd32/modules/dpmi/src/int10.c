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

#define CURSOR_START    0x0A
#define CURSOR_END      0x0B
#define CGA_INDEX_REG   0x3D4
#define CGA_DATA_REG    0x3D5

int videobios_int(union rmregs *r)
{
  int x, y;
  switch (r->h.ah) {
    case 0x01:
	  cursor(r->h.ch&0x0f, r->h.cl&0x0f);
      return 0;

    case 0x02:
      /* Set Cursor Position */
      x = r->h.dl;
      y = r->h.dh;
      place(x, y);
      RMREGS_CLEAR_CARRY;
      return 0;

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
      return 0;

    case 0x08:
      /* Read Character and Attribute */
      /* Let's just return a reasonable attribute... */
      r->h.ah = get_attr();
      r->h.al = ' ';
      RMREGS_CLEAR_CARRY;
      return 0;
      
    /* Video - Teletype output */
    case 0x0E:
      /* AL = Character to write */
      /* BH = Page number        */
      /* BL = Foreground color   */
      /* TODO: page, colors and special BEL character (07h) are ignored */
      if (r->h.al != 0x07) cputc(r->h.al);
      return 0;

    case 0x0F:
      r->h.al = 0x03;
      r->h.ah = 80;
      r->h.bh = 0;
      return 0;

    case 0x10:
      fd32_log_printf("INT 10h AH=10h VIDEO not implemented\n");
      return 0;
    
    case 0x12:
/*
      AH = 12h
      AL = number of rows or columns to scroll
      BH = buffer flag
      00h data in user buffer
      ES:SI -> buffer containing character/attribute pairs
      01h no buffer, fill emptied rows/columns with blanks
      BL = direction in which to scroll
      00h up
      01h down
      02h left
      03h right
      CH,CL = row,column of upper left corner of scroll area
      DH,DL = row,column of lower right corner
      _scroll(char attr,int x1,int y1,int x2,int y2)
*/
      fd32_log_printf("scroll cols %u dir %d top %u left %u bottom %u right %u \n", r->h.al, r->h.bl, r->h.ch, r->h.cl, r->h.dh, r->h.dl);
      return 0;

    case 0x1A:
      if (r->h.al != 0x00) break;
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

    case 0xFE:
      /* Get Shadow Buffer (???)
       */
      /* For the moment, we don't have it... Return 0! */
      r->x.es = 0;
      r->d.edi = 0;
      return 0;

    case 0xFF:
      /* Not clear what to do here... */
      RMREGS_SET_CARRY; /* So, fail... */
      return 0;
  }
  error("Unimplemeted INT!!!\n");
  message("INT 10, AX = 0x%x\n", r->x.ax);
  fd32_abort();
  return 1;
}
