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
#define CLIPPING


#ifdef CLIPPING
void vesa2_textout(int x, int y, const char *s, const PcfFont *font,
                   DWORD pixel, DWORD *lfb, const VesaModeInfo *info)
{
  DWORD       *row;
  const DWORD *data;
  DWORD        bit;
  int          xx, yy;
  int          xmax, ymax;
  unsigned     c;
  unsigned     glyph;
  FontMetrics *met;

  for (; *s; s++)
  {
    c     = (unsigned) *s;
    glyph = font->default_char;
    /* Get the glyph for the character from the glyph index */
    if ((c >= font->min_encoding)
     && (c <= font->max_encoding)
     && (font->glyph_indeces[c] != 0xFFFF))
      glyph = font->glyph_indeces[c - font->min_encoding];

    /* Get font metrics and data and skip rows above clipping rectangle */
    met  = &font->metrics[glyph];
    data = &font->bitmap_data[font->bitmap_offsets[glyph] >> 2];
    ymax = y + met->descent - 1;
    if (ymax > vesa2_cliprect.bottom) ymax = vesa2_cliprect.bottom;
    for (yy = y - met->ascent; yy < vesa2_cliprect.top; yy++) data++;

    /* Draw rows into the clipping rectangle */
    row = &lfb[yy * (info->bytes_per_scanline >> 2)];
    for (; yy <= ymax; yy++)
    {
      bit  = 0x00000001;
      /* Skip all columns on the left of the clipping rectangle */
      xmax = x + met->right_side_bearing - 1;
      if (xmax > vesa2_cliprect.right) xmax = vesa2_cliprect.right;
      for (xx = x + met->left_side_bearing; xx < vesa2_cliprect.left; xx++)
        bit <<= 1;
      /* Draw columns into the clipping rectangle */
      for (; xx <= xmax; xx++)
      {
        if (*data & bit) *(row + xx) = pixel;
        bit <<= 1;
      }
      row += info->bytes_per_scanline >> 2;
      data++;
    }
    x += met->character_width;
  }
}
#else
void vesa2_textout(int x, int y, const char *s, const PcfFont *font,
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
#endif
