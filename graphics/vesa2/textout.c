/**************************************************************************
 * FreeDOS32 VESA2 Graphics Library                                       *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2003, Salvatore Isaja, all rights reserved.              *
 *                                                                        *
 * This is "textout.c" - Rendering a text string using a PCF font.        *
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
#include "vesa2.h"


/* TODO: Add clipping to vesa2_textout */
void vesa2_textout(unsigned x, unsigned y, const char *s, const PcfFont *font,
                   DWORD pixel, DWORD *lfb, const VesaModeInfo *info)
{
  unsigned     width, height;
  DWORD       *column, *row;
  const DWORD *data;
  DWORD        bit;
  
  for (; *s; s++)
  {
    unsigned     c     = (unsigned) *s;
    unsigned     glyph = font->default_char;
    FontMetrics *met;

    if ((c >= font->min_encoding)
     && (c <= font->max_encoding)
     && (font->glyph_indeces[c] != 0xFFFF))
      glyph = font->glyph_indeces[c - font->min_encoding];

    met    = &font->metrics[glyph];
    data   = &font->bitmap_data[font->bitmap_offsets[glyph] >> 2];
    row    = &lfb[(y - met->ascent) * info->bytes_per_scanline >> 2];
    height = met->ascent + met->descent;
    for (; height; height--)
    {
      bit    = 0x00000001;
      column = row + x + met->left_side_bearing;
      width  = met->right_side_bearing - met->left_side_bearing;
      for (; width; width--, column++)
      {
        if (*data & bit) *column = pixel;
        bit <<= 1;
      }
      row += info->bytes_per_scanline >> 2;
      data++;
    }
    x += met->character_width;
  }
}
