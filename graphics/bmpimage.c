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
#include <stdio.h>
#include <stdlib.h>
#include "bmpimage.h"


#define BMPSIGNATURE 0x4D42 /* 'BM' little endian */


typedef struct BmpHeader
{
  WORD  signature;        // signature - 'BM'
  DWORD file_size;        // file size in bytes
  DWORD reserved;         // 0
  DWORD bitmap_offset;    // offset to bitmap
  DWORD struct_size;      // size of this struct (40)
  DWORD width;            // bmap width in pixels
  DWORD height;           // bmap height in pixels
  WORD  planes;           // num planes - always 1
  WORD  bits_per_pixel;   // bits per pixel
  DWORD compression;      // compression flag
  DWORD image_size;       // image size in bytes
  DWORD horiz_res;        // horz resolution
  DWORD vert_res;         // vert resolution
  DWORD colors;           // 0 -> color table size
  DWORD important_colors; // important color count
}
__attribute__ ((packed)) BmpHeader;


#define ABORTLOAD(f, img, msg) { fclose(f); free(img); printf(msg); return NULL; }


BmpImage *bmpimage_load(const char *filename)
{
  BmpHeader  hdr;
  BmpImage  *img = NULL;
  FILE      *f;
  DWORD     *data;
  unsigned   y;

  if ((f = fopen(filename, "rb")) == NULL)
  {
    printf("[BPMIMAGE] open error\n");
    return NULL;
  }
  if (fread(&hdr, sizeof(hdr), 1, f) != 1)
    ABORTLOAD(f, img, "[BPMIMAGE] header read error\n");
  if (hdr.bits_per_pixel != 32)
    ABORTLOAD(f, img, "[BMPIMAGE] bits_per_pixel != 32\n");
  img = (BmpImage *) malloc(2 * sizeof(unsigned) + (hdr.width * hdr.height * hdr.bits_per_pixel / 8));
  if (img == NULL)
    ABORTLOAD(f, img, "[BMPIMAGE] out of memory\n");
  img->width  = hdr.width;
  img->height = hdr.height;
  fseek(f, hdr.bitmap_offset, SEEK_SET);
  data = &img->data[(hdr.height - 1) * hdr.width];
  for (y = hdr.height; y; y--)
  {
    if (fread(data, 4, hdr.width, f) != hdr.width)
      ABORTLOAD(f, img, "[BMPIMAGE] read error\n");
    data -= hdr.width;
  }
  return img;
}
