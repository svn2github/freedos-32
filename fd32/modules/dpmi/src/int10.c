/* DPMI Driver for FD32: int 0x10 services
 * by Luca Abeni
 * extended by Hanzac Chen
 *
 * Ref: http://www.nongnu.org/vgabios
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


/* from http://www.nongnu.org/vgabios (vgabios-0.4c.tgz/vgatables.h) */
#define VGAREG_ACTL_ADDRESS       0x3C0
#define VGAREG_ACTL_READ_DATA     0x3C1
#define VGAREG_ACTL_WRITE_DATA    0x3C0
#define VGAREG_ACTL_RESET         0x3DA

#define VGAREG_DAC_READ_ADDRESS   0x3C7
#define VGAREG_DAC_WRITE_ADDRESS  0x3C8
#define VGAREG_DAC_DATA           0x3C9
#define ACTL_MAX_REG              0x14

#define BIOSMEM_INITIAL_MODE  0x10
#define BIOSMEM_CURRENT_MODE  0x49
#define BIOSMEM_NB_COLS       0x4A
#define BIOSMEM_PAGE_SIZE     0x4C
#define BIOSMEM_CURRENT_START 0x4E
#define BIOSMEM_CURSOR_POS    0x50
#define BIOSMEM_CURSOR_TYPE   0x60
#define BIOSMEM_CURRENT_PAGE  0x62
#define BIOSMEM_CRTC_ADDRESS  0x63
#define BIOSMEM_CURRENT_MSR   0x65
#define BIOSMEM_CURRENT_PAL   0x66
#define BIOSMEM_NB_ROWS       0x84
#define BIOSMEM_CHAR_HEIGHT   0x85
#define BIOSMEM_VIDEO_CTL     0x87
#define BIOSMEM_SWITCHES      0x88
#define BIOSMEM_MODESET_CTL   0x89
#define BIOSMEM_DCC_INDEX     0x8A
#define BIOSMEM_VS_POINTER    0xA8
#define BIOSMEM_VBE_MODE      0xBA

static BYTE *biosmem_p = (BYTE *)0x400;

int videobios_int(union rmregs *r)
{
  int res = 0, x, y;
  switch (r->h.ah) {
    case 0x01:
      cursor(r->h.ch&0x3f, r->h.cl&0x1f);
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
      r->h.al =(biosmem_p[BIOSMEM_VIDEO_CTL] & 0x80) | biosmem_p[BIOSMEM_CURRENT_MODE];
      r->h.ah = biosmem_p[BIOSMEM_NB_COLS];
      r->h.bh = biosmem_p[BIOSMEM_CURRENT_PAGE];
      break;

    /* VIDEO - RIGISTERS SETTING AND READING */
    case 0x10:
      switch (r->h.al)
      {
        case 0x00: /* VIDEO - SET SINGLE PALETTE REGISTER (PCjr,Tandy,EGA,MCGA,VGA) */
          if(r->h.bl <= ACTL_MAX_REG)
          {
            inp(VGAREG_ACTL_RESET);
            outp(VGAREG_ACTL_ADDRESS, r->h.bl);
            outp(VGAREG_ACTL_WRITE_DATA, r->h.bh);
            outp(VGAREG_ACTL_ADDRESS, 0x20);
          }
          break;
        case 0x01: /* VIDEO - SET BORDER (OVERSCAN) COLOR (PCjr,Tandy,EGA,VGA) */
          inp(VGAREG_ACTL_RESET);
          outp(VGAREG_ACTL_ADDRESS, 0x11);
          outp(VGAREG_ACTL_WRITE_DATA, r->h.bh);
          outp(VGAREG_ACTL_ADDRESS, 0x20);
          break;
        case 0x02: /* VIDEO - SET ALL PALETTE REGISTERS (PCjr,Tandy,EGA,VGA) */
          if (1) {
            BYTE *seg = (BYTE *)(r->x.es<<4);
            DWORD offset = r->x.dx;

            inp(VGAREG_ACTL_RESET);
            /* First the colors */
            #define VGAREG_WRITE_COLOR(c) outp(VGAREG_ACTL_ADDRESS, c); outp(VGAREG_ACTL_WRITE_DATA, seg[offset++])
            VGAREG_WRITE_COLOR(0x00); VGAREG_WRITE_COLOR(0x01);
            VGAREG_WRITE_COLOR(0x02); VGAREG_WRITE_COLOR(0x03);
            VGAREG_WRITE_COLOR(0x04); VGAREG_WRITE_COLOR(0x05);
            VGAREG_WRITE_COLOR(0x06); VGAREG_WRITE_COLOR(0x07);
            VGAREG_WRITE_COLOR(0x08); VGAREG_WRITE_COLOR(0x09);
            VGAREG_WRITE_COLOR(0x0A); VGAREG_WRITE_COLOR(0x0B);
            VGAREG_WRITE_COLOR(0x0C); VGAREG_WRITE_COLOR(0x0D);
            VGAREG_WRITE_COLOR(0x0E); VGAREG_WRITE_COLOR(0x0F);
            #undef VGAREG_WRITE_COLOR

            /* Then the border */
            outp(VGAREG_ACTL_ADDRESS, 0x11);
            outp(VGAREG_ACTL_WRITE_DATA, seg[offset]);
            outp(VGAREG_ACTL_ADDRESS, 0x20);
          }
          break;
        case 0x03: /* VIDEO - TOGGLE INTENSITY/BLINKING BIT (Jr, PS, TANDY 1000, EGA, VGA) */
          if (1) {
            DWORD value;
            inp(VGAREG_ACTL_RESET);
            outp(VGAREG_ACTL_ADDRESS, 0x10);
            value = inp(VGAREG_ACTL_READ_DATA);
            value &= 0xF7;
            value |= (r->h.bl&0x01)<<3;
            outp(VGAREG_ACTL_WRITE_DATA, value);
            outp(VGAREG_ACTL_ADDRESS, 0x20);
          }
          break;
        case 0x07: /* VIDEO - GET INDIVIDUAL PALETTE REGISTER (VGA,UltraVision v2+) */
          if(r->h.bl <= ACTL_MAX_REG)
          {
            inp(VGAREG_ACTL_RESET);
            outp(VGAREG_ACTL_ADDRESS, r->h.bl);
            r->h.bh = inp(VGAREG_ACTL_READ_DATA);
            inp(VGAREG_ACTL_RESET);
            outp(VGAREG_ACTL_ADDRESS, 0x20);
          }
          break;
        case 0x09: /* VIDEO - READ ALL PALETTE REGISTERS AND OVERSCAN REGISTER (VGA) */
          if (1) {
            BYTE *seg = (BYTE *)(r->x.es<<4);
            DWORD offset = r->x.dx;

            /* First the colors */
            #define VGAREG_READ_COLOR(c) inp(VGAREG_ACTL_RESET); outp(VGAREG_ACTL_ADDRESS, c); seg[offset++] = inp(VGAREG_ACTL_READ_DATA)
            VGAREG_READ_COLOR(0x00); VGAREG_READ_COLOR(0x01);
            VGAREG_READ_COLOR(0x02); VGAREG_READ_COLOR(0x03);
            VGAREG_READ_COLOR(0x04); VGAREG_READ_COLOR(0x05);
            VGAREG_READ_COLOR(0x06); VGAREG_READ_COLOR(0x07);
            VGAREG_READ_COLOR(0x08); VGAREG_READ_COLOR(0x09);
            VGAREG_READ_COLOR(0x0A); VGAREG_READ_COLOR(0x0B);
            VGAREG_READ_COLOR(0x0C); VGAREG_READ_COLOR(0x0D);
            VGAREG_READ_COLOR(0x0E); VGAREG_READ_COLOR(0x0F);
            #undef VGAREG_READ_COLOR

            /* Then the border */
            inp(VGAREG_ACTL_RESET);
            outp(VGAREG_ACTL_ADDRESS, 0x11);
            seg[offset] = inp(VGAREG_ACTL_READ_DATA);
            outp(VGAREG_ACTL_ADDRESS, 0x20);
          }
          break;
        case 0x15: /* VIDEO - READ INDIVIDUAL DAC REGISTER (VGA/MCGA) */
          outp(VGAREG_DAC_READ_ADDRESS, r->h.bl);
          r->h.dh = inp(VGAREG_DAC_DATA); /* RED   */
          r->h.ch = inp(VGAREG_DAC_DATA); /* GREEN */
          r->h.cl = inp(VGAREG_DAC_DATA); /* BLUE  */
          break;
        default:
          fd32_log_printf("Unimplemented INT 0x10 AX=%x\n", r->x.ax);
          break;
      }
      break;

    /* VIDEO - ALTERNATE FUNCTIONS */
    case 0x12:
      fd32_log_printf("Unimplemented INT 0x10 AX=%x\n", r->x.ax);
      break;

    /* VIDEO - SET/GET DISPLAY COMBINATION CODE */
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
