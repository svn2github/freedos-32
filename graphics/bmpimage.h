/**************************************************************************
 * FreeDOS32 Bitmap Images Library                                        *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2003, Salvatore Isaja, all rights reserved.              *
 *                                                                        *
 *                                                                        *
 * This file is part of the FreeDOS32 Bitmap Images Library (LIBRARY).    *
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
#ifndef __BMPIMAGE_H
#define __BMPIMAGE_H

#include "basictyp.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct BmpImage
{
  unsigned width;   /* in pixels             */
  unsigned height;  /* in pixels             */
  DWORD    data[1]; /* width*height elements */
}
BmpImage;


BmpImage *bmpimage_load(const char *filename);


#ifdef __cplusplus
}
#endif

#endif /* #ifndef __BMPIMAGE_H */

