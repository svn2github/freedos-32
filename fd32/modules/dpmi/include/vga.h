/* DPMI Driver for FD32: VGA calls
 *
 * Copyright (C) 2001,2002 the LGPL VGABios developers Team
 */

#ifndef __VGA_H__
#define __VGA_H__

/* from http://www.nongnu.org/vgabios (vgatables.h) */
#define VGAREG_MDA_CRTC_ADDRESS   0x3B4
#define VGAREG_ACTL_ADDRESS       0x3C0
#define VGAREG_ACTL_WRITE_DATA    0x3C0
#define VGAREG_ACTL_READ_DATA     0x3C1
#define VGAREG_WRITE_MISC_OUTPUT  0x3C2
#define VGAREG_SEQU_ADDRESS       0x3C4
#define VGAREG_SEQU_DATA          0x3C5
#define VGAREG_PEL_MASK           0x3C6
#define VGAREG_DAC_READ_ADDRESS   0x3C7
#define VGAREG_DAC_WRITE_ADDRESS  0x3C8
#define VGAREG_DAC_DATA           0x3C9
#define VGAREG_GRDC_ADDRESS       0x3Ce
#define VGAREG_GRDC_DATA          0x3Cf
#define VGAREG_VGA_CRTC_ADDRESS   0x3D4
#define VGAREG_ACTL_RESET         0x3DA

#define ACTL_MAX_REG              0x14

/*
 *
 * BIOS Memory 
 *
 */
#define BIOSMEM_SEG 0x40

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
#define BIOSMEM_VBE_FLAG      0xB9
#define BIOSMEM_VBE_MODE      0xBA

#define TEXT       0x00
#define GRAPH      0x01
#define CTEXT      0x00
#define MTEXT      0x01
#define CGA        0x02
#define PLANAR1    0x03
#define PLANAR2    0x04
#define PLANAR4    0x05
#define LINEAR8    0x06
#define MODE_MAX   0x14

typedef struct
{
 BYTE  svgamode;
 WORD  vesamode;
 BYTE  class;    /* TEXT, GRAPH */
 BYTE  memmodel; /* CTEXT,MTEXT,CGA,PL1,PL2,PL4,P8,P15,P16,P24,P32 */
 BYTE  nbpages; 
 BYTE  pixbits;
 WORD  swidth, sheight;
 WORD  twidth, theight;
 WORD  cwidth, cheight;
 WORD  sstart;
 WORD  slength;
 BYTE  miscreg;
 BYTE  pelmask;
 BYTE  crtcmodel;
 BYTE  actlmodel;
 BYTE  grdcmodel;
 BYTE  sequmodel;
 BYTE  dacmodel; /* 0 1 2 3 */
} __attribute__ ((packed)) VGAMODE;


void vga_get_all_palette_reg(BYTE *pal);
void vga_set_all_palette_reg(BYTE *pal);
void vga_toggle_intensity(BYTE state);
BYTE vga_get_single_palette_reg(BYTE reg);
void vga_set_single_palette_reg(BYTE reg, BYTE value);
void vga_set_all_dac_reg(WORD start, WORD count, BYTE *table);
void vga_set_single_dac_reg(WORD reg, BYTE r, BYTE g, BYTE b);
void vga_read_single_dac_reg(WORD reg, BYTE *r, BYTE *g, BYTE *b);
BYTE vga_get_video_mode(BYTE *colsnum, BYTE *curpage);
BYTE vga_set_video_mode(BYTE modenum);

#endif
