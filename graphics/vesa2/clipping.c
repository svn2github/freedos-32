/**************************************************************************
 * FreeDOS32 VESA2 Graphics Library                                       *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2003, Salvatore Isaja, all rights reserved.              *
 *                                                                        *
 * This is "clipping.c" - Clipping draw operations to a specified         *
 *                        rectangle.                                      *
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


RECT vesa2_cliprect;


void vesa2_set_clipping(const RECT *r, const VesaModeInfo *info)
{
  if (!r)
  {
    vesa2_cliprect.left   = 0;
    vesa2_cliprect.top    = 0;
    vesa2_cliprect.right  = info->x_res - 1;
    vesa2_cliprect.bottom = info->y_res - 1;
    return;
  }
  vesa2_cliprect.left   = (r->left < 0) ? 0 : r->left;
  vesa2_cliprect.top    = (r->top  < 0) ? 0 : r->top;
  vesa2_cliprect.right  = (r->right  >= info->x_res) ? info->x_res - 1 : r->right;
  vesa2_cliprect.bottom = (r->bottom >= info->y_res) ? info->y_res - 1 : r->bottom;
}
