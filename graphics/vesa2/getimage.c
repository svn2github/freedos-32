/**************************************************************************
 * FreeDOS32 VESA2 Graphics Library                                       *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2003, Salvatore Isaja, all rights reserved.              *
 *                                                                        *
 * This is "getimage.c" - Capturing bitmap images from a linear frame     *
 *                        buffer.                                         *
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


void vesa2_getimage(BmpImage *bmp, int x, int y,
                    const DWORD *lfb, const VesaModeInfo *info)
{
  const DWORD *srcrow,  *srccolumn;
  DWORD       *destrow, *destcolumn;
  int          x1 = x;
  int          y1 = y;
  int          x2 = x1 + bmp->width  - 1;
  int          y2 = y1 + bmp->height - 1;
  int          xdest = 0, ydest = 0;
  int          height, width;

  /* Clipping */
  if (x1 < vesa2_cliprect.left)
  {
    xdest = vesa2_cliprect.left - x1;
    x1    = vesa2_cliprect.left;
  }
  if (y1 < vesa2_cliprect.top)
  {
    ydest = vesa2_cliprect.top - y1;
    y1    = vesa2_cliprect.top;
  }
  if (x2 > vesa2_cliprect.right)  x2 = vesa2_cliprect.right;
  if (y2 > vesa2_cliprect.bottom) y2 = vesa2_cliprect.bottom;
  /* Take the address of the destination row only if it's valid */
  if (ydest >= bmp->height) return;
  destrow = &bmp->data[ydest * bmp->width];
  srcrow  = &lfb[y1 * info->bytes_per_scanline >> 2];
  /* Get */
  for (height = y2 - y1 + 1; height > 0; height--)
  {
    srccolumn  = srcrow  + x1;
    destcolumn = destrow + xdest;
    for (width = x2 - x1 + 1; width > 0; width--, srccolumn++, destcolumn++)
      *destcolumn = *srccolumn | 0xFF000000; /* Full opaque */
    destrow += bmp->width;
    srcrow  += info->bytes_per_scanline >> 2;
  }
}
