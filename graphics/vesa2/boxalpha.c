/**************************************************************************
 * FreeDOS32 VESA2 Graphics Library                                       *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2003, Salvatore Isaja, all rights reserved.              *
 *                                                                        *
 * This is "boxalpha.c" - Drawing filled boxes with transparency.         *
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
#include "pixalpha.c"


void vesa2_box_alpha(int x1, int y1, int x2, int y2,
                     unsigned r, unsigned g, unsigned b, unsigned alpha,
                     DWORD *lfb, const VesaModeInfo *info)
{
  int             width, height;
  register DWORD *column;
  DWORD          *row;
  DWORD           pixel;

  if (alpha == 0) return;
  if (alpha == 255) vesa2_box(x1, y1, x2, y2, r, g, b, lfb, info);

  /* Clipping */
  if (x1 < vesa2_cliprect.left)   x1 = vesa2_cliprect.left;
  if (y1 < vesa2_cliprect.top)    y1 = vesa2_cliprect.top;
  if (x2 > vesa2_cliprect.right)  x2 = vesa2_cliprect.right;
  if (y2 > vesa2_cliprect.bottom) y2 = vesa2_cliprect.bottom;
  /* Draw */
  pixel = (r << info->red_field_pos)
        | (g << info->green_field_pos)
        | (b << info->blue_field_pos);
  row = &lfb[y1 * info->bytes_per_scanline >> 2];
  #ifdef __MMX__
  asm("emms");
  #endif
  for (height = y2 - y1 + 1; height > 0; height--)
  {
    column = row + x1;
    for (width = x2 - x1 + 1; width > 0; width--, column++)
      vesa2_pixel_alpha(column, pixel, alpha);
    row += info->bytes_per_scanline >> 2;
  }
  #ifdef __MMX__
  asm("emms");
  #endif
}
