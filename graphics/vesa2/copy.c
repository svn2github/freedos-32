/**************************************************************************
 * FreeDOS32 VESA2 Graphics Library                                       *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2003, Salvatore Isaja, all rights reserved.              *
 *                                                                        *
 * This is "copy.c" - Copying rectangular portions between linear frame   *
 *                    buffers.                                            *
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


void vesa2_copy(DWORD *dest, int xdest, int ydest, unsigned width, unsigned height,
                const DWORD *src, unsigned xsrc, unsigned ysrc, const VesaModeInfo *info)
{
  const DWORD *srcrow, *srccolumn;
  DWORD       *destrow, *destcolumn;
  int          x1 = xdest;
  int          y1 = ydest;
  int          x2 = x1 + width  - 1;
  int          y2 = y1 + height - 1;
  int          w, h;

  /* Clipping */
  if (x1 < vesa2_cliprect.left)
  {
    xsrc += vesa2_cliprect.left - x1;
    x1    = vesa2_cliprect.left;
  }
  if (y1 < vesa2_cliprect.top)
  {
    ysrc += vesa2_cliprect.top - y1;
    y1    = vesa2_cliprect.top;
  }
  if (x2 > vesa2_cliprect.right)  x2 = vesa2_cliprect.right;
  if (y2 > vesa2_cliprect.bottom) y2 = vesa2_cliprect.bottom;
  /* Draw */
  srcrow  = &src[ysrc * info->bytes_per_scanline >> 2];
  destrow = &dest[y1 * info->bytes_per_scanline >> 2];
  for (h = y2 - y1 + 1; h > 0; h--)
  {
    srccolumn  = srcrow  + xsrc;
    destcolumn = destrow + x1;
    for (w = x2 - x1 + 1; w > 0; w--, srccolumn++, destcolumn++)
      *destcolumn = *srccolumn;
    srcrow  += info->bytes_per_scanline >> 2;
    destrow += info->bytes_per_scanline >> 2;
  }
}
