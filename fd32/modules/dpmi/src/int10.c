/* DPMI Driver for FD32: int 0x10 services
 * by Luca Abeni and Hanzac Chen
 *
 * Ref: http://www.nongnu.org/vgabios
 *
 * This is free software; see GPL.txt
 */
 
#include <ll/i386/hw-data.h>
#include <ll/i386/error.h>
#include <ll/i386/cons.h>
#include <logger.h>
#include <kernel.h>
#include "rmint.h"
#include "vga.h"

extern void int33_set_screen(int w, int h);
extern void int33_set_text_cell(int w, int h);

int videobios_int(union rmregs *r)
{
  int res = 0;
  switch (r->h.ah) {
    /* VIDEO - SET VIDEO MODE */
    case 0x00:
      r->h.al = vga_set_video_mode(r->h.al);
      break;
    /* VIDEO - SET TEXT-MODE CURSOR SHAPE */
    case 0x01:
      vga_set_cursor_shape(r->h.ch, r->h.cl);
      break;;

    case 0x02:
      /* Set Cursor Position */
      vga_set_cursor_pos(r->h.bh, r->x.dx);
      break;

    case 0x03:
      /* Get Cursor Position and Size...*/
      vga_get_cursor_pos(r->h.bh, &r->x.cx, &r->x.dx);
      break;

    case 0x05:
      vga_set_active_page(r->h.al);
      break;

    case 0x06:
      vga_scroll(r->h.al, r->h.bh, r->h.ch, r->h.cl, r->h.dh, r->h.dl, 0xFF, SCROLL_UP);
      break;

    case 0x08:
      /* Read Character and Attribute */
      /* Let's just return a reasonable attribute... */
      r->h.ah = get_attr();
      r->h.al = ' ';
      break;

    case 0x09:
      vga_write_char_attr(r->h.al, r->h.bh, r->h.bl, r->x.cx);
      break;
      
    /* Video - Teletype output */
    case 0x0E:
      /* AL = Character to write */
      /* BH = Page number        */
      /* BL = Foreground color   */
      /* TODO: page, colors and special BEL character (07h) are ignored */
      vga_write_teletype(r->h.al, 0xFF, r->h.bl, NO_ATTR);
      break;

    /* VIDEO - GET CURRENT VIDEO MODE */
    case 0x0F:
      r->h.al = vga_get_video_mode(&(r->h.ah), &(r->h.bh));
      break;

    /* VIDEO - RIGISTERS SETTING AND READING */
    case 0x10:
      switch (r->h.al)
      {
        case 0x00: /* VIDEO - SET SINGLE PALETTE REGISTER (PCjr,Tandy,EGA,MCGA,VGA) */
          vga_set_single_palette_reg(r->h.bl, r->h.bh);
          break;
        case 0x01: /* VIDEO - SET BORDER (OVERSCAN) COLOR (PCjr,Tandy,EGA,VGA) */
          vga_set_overscan_border_color(r->h.bh);
          break;
        case 0x02: /* VIDEO - SET ALL PALETTE REGISTERS (PCjr,Tandy,EGA,VGA) */
          vga_set_all_palette_reg((BYTE *)(r->x.es<<4)+r->x.dx);
          break;
        case 0x03: /* VIDEO - TOGGLE INTENSITY/BLINKING BIT (Jr, PS, TANDY 1000, EGA, VGA) */
          vga_toggle_intensity(r->h.bl);
          break;
        case 0x07: /* VIDEO - GET INDIVIDUAL PALETTE REGISTER (VGA,UltraVision v2+) */
          r->h.bh = vga_get_single_palette_reg(r->h.bl);
          break;
        case 0x09: /* VIDEO - READ ALL PALETTE REGISTERS AND OVERSCAN REGISTER (VGA) */
          vga_get_all_palette_reg((BYTE *)(r->x.es<<4)+r->x.dx);
          break;
        case 0x10: /* VIDEO - SET INDIVIDUAL DAC REGISTER (VGA/MCGA) */
          vga_set_single_dac_reg(r->x.bx, r->h.dh /* RED   */, r->h.ch /* GREEN */, r->h.cl /* BLUE  */);
          break;
        case 0x12: /* VIDEO - SET BLOCK OF DAC REGISTERS (VGA/MCGA) */
          vga_set_all_dac_reg(r->x.bx, r->x.cx, (BYTE *)(r->x.es<<4)+r->x.dx);
          break;
        case 0x15: /* VIDEO - READ INDIVIDUAL DAC REGISTER (VGA/MCGA) */
          vga_read_single_dac_reg(r->h.bl, &r->h.dh /* RED   */, &r->h.ch /* GREEN */, &r->h.cl /* BLUE  */);
          break;
        default:
          fd32_log_printf("Unimplemented INT 0x10 AX=%x\n", r->x.ax);
          break;
      }
      break;

    /* VIDEO - TEXT-MODE CHARGEN */
    case 0x11:
      switch (r->h.al)
      {
        case 0x02:
        case 0x12: /* VIDEO - TEXT-MODE CHARGEN - LOAD ROM 8x8 DBL-DOT PATTERNS (PS,EGA,VGA) */
          vga_load_text_8_8_pat(r->h.al, r->h.bl);
          int33_set_text_cell(8, 8);
          break;
        case 0x03:
          vga_set_text_block_specifier(r->h.bl);
          break;
        case 0x04:
        case 0x14: /* VIDEO - TEXT-MODE CHARGEN - LOAD ROM 8x16 CHARACTER SET (VGA) */
          vga_load_text_8_16_pat(r->h.al, r->h.bl);
          int33_set_text_cell(8, 16);
          break;
        default:
          fd32_log_printf("Unimplemented INT 0x10 AX=%x\n", r->x.ax);
          break;
      }
      break;

    /* VIDEO - ALTERNATE FUNCTIONS */
    case 0x12:
      fd32_log_printf("VIDEO - Alternate Functions (AX=%x BX=%x)\n", r->x.ax, r->x.bx);
      switch (r->h.bl)
      {
        case 0x10: /* GET EGA INFO */
          break;
        case 0x20: /* ALTERNATE PRTSC */
          break;
        case 0x30: /* SELECT VERTICAL RESOLUTION */
          break;
        case 0x31: /* PALETTE LOADING */
          break;
        case 0x32: /* VIDEO ADDRESSING */
          break;
        case 0x33: /* GRAY-SCALE SUMMING */
          break;
        case 0x34: /* CURSOR EMULATION */
          break;
        case 0x35: /* DISPLAY-SWITCH INTERFACE */
          break;
        case 0x36: /* VIDEO REFRESH CONTROL */
          break;
        default:
          fd32_log_printf("VIDEO - Alternate Functions (AX=%x BX=%x)\n", r->x.ax, r->x.bx);
          break;
      }
      break;

    /* VIDEO - SET/GET DISPLAY COMBINATION CODE */
    case 0x1A:
      /* Get display combination mode (???)
       * let's say that it is used for checking if the video card is a VGA...
       */
      if ( r->h.al == 0) {
        /* Alternate Display Code (BH) and Active Display Code (BL) */
        r->h.bh = 0x07; /* 7 Should be VGA!!! */
        r->h.bl = vga_read_display_code();
      } else if ( r->h.al == 1) {
        vga_set_display_code(r->h.bl);
      }
      r->h.al = 0x1A; /* function supported */
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
      error("Unimplemeted INT!!!\n");
      message("INT 0x10, AX = 0x%x\n", r->x.ax);
      fd32_abort();
  }

  return res;
}
