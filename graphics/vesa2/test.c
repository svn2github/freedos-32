/**************************************************************************
 * FreeDOS32 VESA2 Graphics Library                                       *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2003, Salvatore Isaja, all rights reserved.              *
 *                                                                        *
 * This is "test.c" - A simple test program for the VESA2 library.        *
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
#include <sys/nearptr.h>
#include <conio.h>
#include <stdio.h>
#include "vesa2.h"

#ifdef PROFILE
 #define DEST back
#else
 #define DEST lfb
#endif


unsigned getoffset(unsigned x, unsigned y, const VesaModeInfo *info)
{
  return y * info->bytes_per_scanline / (info->bits_per_pixel / 8) + x;
}


void pixels(DWORD *lfb, const VesaModeInfo *info)
{
  unsigned  x, y;
  for (x = 0; x < info->x_res; x++)
  {
    register unsigned offset;
    for (y = 0; y < info->y_res / 3; y++)
    {
      offset = getoffset(x, y, info);
      lfb[offset] = (x * 256 / info->x_res) << info->red_field_pos;
    }
    for (; y < info->y_res * 2 / 3; y++)
    {
      offset = getoffset(x, y, info);
      lfb[offset] = (x * 256 / info->x_res) << info->green_field_pos;
    }
    for (; y < info->y_res; y++)
    {
      offset = getoffset(x, y, info);
      lfb[offset] = (x * 256 / info->x_res) << info->blue_field_pos;
    }
  }
}


DWORD back[1024*768]; /* A back buffer in RAM */


/* Change them according to your needs. */
#define FONTNAME "font.pcf"  /* A little-endian PCF font       */
#define BMPNAME  "image.bmp" /* A 32-bit per pixel Windows BMP */


int main()
{
  void        *lfb;
  int          mode;
  VesaModeInfo i;
  PcfFont      font;
  BmpImage    *image;

//  #ifdef PROFILE
  unsigned     k;
//  #endif
  if (pcffont_load(FONTNAME, &font) < 0)
  {
    printf("Error reading font\n");
    return -1;
  }
  if ((image = bmpimage_load(BMPNAME)) == NULL)
  {
    printf("Error reading image\n");
    return -1;
  }

  __djgpp_nearptr_enable();
  #if 0
  /* Enumerate */
  for (k = 0; k < 0x1000; k++)
    if (vesa2_get_mode_info(k, &i) == 0)
      printf("%3X supported: %ux%u, %u planes, %u colors\n",
             k, i.x_res, i.y_res, i.number_of_planes, i.bits_per_pixel);
  return 0;
  #endif
  mode = vesa2_find_video_mode(1024, 768, 32, &i);
  if (mode < 0)
  {
    printf("Cannot find the requested video mode\n");
    return -1;
  }
  if (vesa2_set_video_mode(mode, &i, &lfb) < 0)
  {
    printf("Cannot set the requested video mode\n");
    return -1;
  }
  #ifdef PROFILE
  for (k = 0; k < 100; k++)
  #endif
  {
//  pixels(back, &i);
//  pixels(lfb, &i);
  vesa2_box(0, 0, 1024, 768, 230, 230, 230, DEST, &i);
  #ifdef PROFILE
  {
  unsigned j;
  for (j = 0; j < 1000; j++)
  {
  #endif
  vesa2_textout(100, 100, "The quick brown fox jumps over the lazy dog.", &font, 0x00FFFFFF, DEST, &i);
  vesa2_putimage(image, 100, 130, DEST, &i);
  #ifdef PROFILE
  }
  }
  #endif
//  vesa2_box_alpha(0, 0, 1024, 768, 60, 50, 130, 100, lfb, &i);
//  vesa2_copy(lfb, 80, 80, 180, 130, back, 80, 80, &i);
//  vesa2_box(0, 0, 1024, 768, k, k, k, (DWORD *) lfb, &i);
//  vesa2_box_alpha(100, 100, 700, 400, 60, 50, 130, 140, (DWORD *) lfb, &i);
  }
  #ifndef PROFILE
  getch();
  #endif
  vesa2_done();
  return 0;
}
