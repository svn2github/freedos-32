/**************************************************************************
 * FreeDOS32 VESA2 Graphics Library                                       *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2003, Salvatore Isaja, all rights reserved.              *
 *                                                                        *
 * This is "vesa2.h" - Library's constants, structures and prototypes     *
 *                                                                        *
 *                                                                        *
 * This file is part of the FreeDOS32 VESA2 Graphics Library (LIBRARY).   *
 *                                                                        *
 * The LIBRARY is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the  *
 * Free Software Foundation; either version 2 of the License, or (at your *
 * option) any later version.                                             *
 *                                                                        *
 * The LIBRARY is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with the LIBRARY; see the file COPYING.txt; if not, write to     *
 * the Free Software Foundation, Inc.,                                    *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA                *
 **************************************************************************/
#ifndef __FD32_VESA2_H
#define __FD32_VESA2_H

#include "../basictyp.h"
#include "../pcffont.h"
#include "../bmpimage.h"


#ifdef __cplusplus
extern "C" {
#endif


#if 0
/* Info block for the VESA controller */
typedef struct VbeInfoBlock
{
  char  vbe_signature[4], /* Must be 'VESA', or 'VBE2' for VESA2          */
  WORD  vbe_version,      /* 0200h for VESA2                              */
  DWORD oem_string_ptr,   /* Pointer to OEM string                        */
  DWORD capabilities,     /* Capabilities of the controller               */
  DWORD video_mode_ptr,   /* Pointer to a video mode list                 */
  WORD  total_memory,     /* Number of 64 KiB memory blocks (VESA2)       */
  WORD  oem_software_rev, /* VBE implementation software revision         */
  DWORD oem_vendor_name,  /* Pointer to Vendor Name string                */
  DWORD oem_product_name, /* Pointer to Product Name string               */
  DWORD oem_product_rev,  /* Pointer to Product Revision string           */
  BYTE  reserved[222],    /* Reserved for VBE implementation scratch area */
  BYTE  oem_data[256]     /* Data area for OEM strings                    */
}
__attribute__ ((packed)) VbeInfoBlock;


/* Mnemonics for VbeInfoBlock.capabilities flags */
enum
{
  VIB_8BITDAC = 1 << 0, /* DAC width switchable to 8 bits per primary color */
  VIB_NOTVGA  = 1 << 1, /* Controller is not VGA compatible                 */
  VIB_BLANK   = 1 << 2  /* RAMDAC recommentds programming during blank      */
};
#endif


/* Info block for a VESA video mode */
typedef struct VesaModeInfo
{
  /* Mandatory information for all VBE revisions */
  WORD  mode_attributes;        /* See enumerated flags below                 */
  BYTE  window_attributes[2];   /* See enumerated flags below                 */
  WORD  window_granularity;     /* KiB boundary for frame buffer placement    */
  WORD  window_size;            /* Window size in KiB                         */
  WORD  window_segment[2];      /* Real-mode segments for the two windows     */
  DWORD window_function_ptr;    /* Real-mode pointer to windowing function    */
  WORD  bytes_per_scanline;
  /* Mandatory information for VBE 1.2 and above */
  WORD  x_res;                  /* horizontal resolution in pixels or chars   */
  WORD  y_res;                  /* vertical resolution in pixels or chars     */
  BYTE  x_char_size;            /* character cell width in pixels             */
  BYTE  y_char_size;            /* character cell height in pixels            */
  BYTE  number_of_planes;       /* number of memory planes                    */
  BYTE  bits_per_pixel;
  BYTE  number_of_banks;
  BYTE  mem_model;              /* memory model type                          */
  BYTE  bank_size;              /* bank size in KiB                           */
  BYTE  number_of_image_pages;
  BYTE  reserved;               /* reserved for page function                 */
  /* Direct Color fields (required for direct/6 and YUV/7 memory models) */
  BYTE  red_mask_size;          /* size of direct color red mask in bits      */
  BYTE  red_field_pos;          /* bit position of lsb of red mask            */
  BYTE  green_mask_size;        /* size of direct color green mask in bits    */
  BYTE  green_field_pos;        /* bit position of lsb of green mask          */
  BYTE  blue_mask_size;         /* size of direct color blue mask in bits     */
  BYTE  blue_field_pos;         /* bit position of lsb of blue mask           */
  BYTE  rsvd_mask_size;         /* size of direct color reserved mask in bits */
  BYTE  rsvd_field_pos;         /* bit position of lsb of reserved mask       */
  BYTE  direct_color_mode_info; /* direct color mode attributes               */
  /* Mandatory information for VBE 2.0 and above */
  DWORD phys_base_ptr;          /* physical address for flat frame buffer     */
  DWORD off_screen_memoffset;   /* pointer to start of off screen memory      */
  WORD  off_screen_memsize;     /* amount of off screen memory in 1k units    */
  BYTE  __Reserved[206];        /* pads VesaModeInfo to 512 bytes             */
}
__attribute__ ((packed)) VesaModeInfo;


/* Mnemonics for VesaModeInfo.mode_attributes flags */
enum
{
  VMI_MA_SUPPORTED = 1 << 0, /* Mode supported by hardware configuration   */
  VMI_MA_TTY       = 1 << 2, /* TTY output functions supported by the BIOS */
  VMI_MA_COLOR     = 1 << 3, /* Color mode if set, monochrome if not       */
  VMI_MA_GRAPHICS  = 1 << 4, /* Graphics mode if set, text mode if not     */
  VMI_MA_NOTVGA    = 1 << 5, /* VGA compatible mode                        */
  VMI_MA_NOTVGAWIN = 1 << 6, /* VGA compatible windowed memory available   */
  VMI_MA_LFB       = 1 << 7  /* Linear frame buffer mode available         */
};


/* Mnemonics for VesaModeInfo.window_attributes flags */
enum
{
  VMI_WA_RELOC = 1 << 0, /* Relocatable window(s) supported */
  VMI_WA_READ  = 1 << 1, /* Window is readable              */
  WMI_WA_WRITE = 1 << 2  /* Window is writeable             */
};


/* Valid values for VesaModeInfo.mem_model */
typedef enum VmiMemModel
{
  VMI_MM_TEXT = 0x00, /* Text mode                   */
  VMI_MM_CGA  = 0x01, /* CGA graphics                */
  VMI_MM_HERC = 0x02, /* Hercules graphics           */
  VMI_MM_PLAN = 0x03, /* Planar                      */
  VMI_MM_PACK = 0x04, /* Packed pixel                */
  VMI_MM_NCHN = 0x05, /* Non-chain 4, 256 color      */
  VMI_MM_DIR  = 0x06, /* Direct color                */
  VMI_MM_YUV  = 0x07, /* YUV color                   */
  VMI_MM_OEM  = 0x10  /* Start of OEM defined models */
}
VmiMemModel;


/* Mnemonics for VesaModeInfo.direct_color_mode_info */
enum
{
  VMI_DC_RAMP = 1 << 0, /* Color ramp programmable if set, fixed if not     */
  VMI_DC_RSVD = 1 << 1  /* Bits in rsvd field are usable by the application */
};


/* modes.c - Setting VESA2 video modes */
int  vesa2_get_mode_info  (WORD mode, VesaModeInfo *info);
int  vesa2_find_video_mode(unsigned x_res, unsigned y_res, unsigned bpp,
                           VesaModeInfo *info);
int  vesa2_set_video_mode (WORD mode, const VesaModeInfo *info, void **lfb);
void vesa2_done           (void);

/* box.c - Drawing filled boxes */
void vesa2_box(int x1, int y1, int x2, int y2,
               unsigned r, unsigned g, unsigned b,
               DWORD *lfb, const VesaModeInfo *info);

/* boxalpha.c - Drawing filled boxes with transparency */
void vesa2_box_alpha(int x1, int y1, int x2, int y2,
                     unsigned r, unsigned g, unsigned b, unsigned alpha,
                     DWORD *lfb, const VesaModeInfo *info);

/* copy.c - Copying rectangular portions between linear frame buffers */
void vesa2_copy(DWORD *dest, int xdest, int ydest, unsigned width, unsigned height,
                const DWORD *src, unsigned xsrc, unsigned ysrc, const VesaModeInfo *info);


/* textout.c - Rendering a text string using a PCF font */
void vesa2_textout(unsigned x, unsigned y, const char *s, const PcfFont *font,
                   DWORD pixel, DWORD *lfb, const VesaModeInfo *info);

/* putimage.c - Displaying bitmap images with transparency */
void vesa2_putimage(const BmpImage *bmp, int x, int y,
                    DWORD *lfb, const VesaModeInfo *info);

/* getimage.c - Capturing bitmap images from a linear frame buffer */
void vesa2_getimage(BmpImage *bmp, unsigned x, unsigned y,
                    const DWORD *lfb, const VesaModeInfo *info);

/* clipping.c - Clipping draw operations to a specified rectangle */
extern RECT vesa2_cliprect;
void vesa2_set_clipping(const RECT *r, const VesaModeInfo *info);


#ifdef __cplusplus
}
#endif

#endif /* #ifndef __FD32_VESA2_H */

