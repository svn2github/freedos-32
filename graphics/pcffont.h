/**************************************************************************
 * FreeDOS32 PCF Fonts Library                                            *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2003, Salvatore Isaja, all rights reserved.              *
 *                                                                        *
 *                                                                        *
 * This file is part of the FreeDOS32 PCF Fonts Library (LIBRARY).        *
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
#ifndef __PCFFONT_H
#define __PCFFONT_H

#include "basictyp.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef struct FontMetrics
{
  int left_side_bearing;  /* leftmost non-blank pixel    */
  int right_side_bearing; /* rightmost non-blank pixel   */
  int character_width;    /* character spacing in pixels */
  int ascent;             /* pixels above base-line      */
  int descent;            /* pixels below base-line      */
}
FontMetrics;


typedef struct PcfFont
{
  unsigned     glyph_count;
  unsigned     min_encoding;
  unsigned     max_encoding;
  WORD        *glyph_indeces;  /* max_encoding - min_encoding + 1 entries */
  FontMetrics *metrics;        /* glyph_count entries                     */
  DWORD        bitmap_size;    /* size of bitmap_data[] in bytes          */
  DWORD       *bitmap_offsets; /* glyph_count entries                     */
  DWORD       *bitmap_data;    /* glyph_count entries                     */
  WORD         default_char;
}
PcfFont;


int  pcffont_load  (const char *filename, PcfFont *font);
void pcffont_unload(PcfFont *font);


#ifdef __cplusplus
}
#endif

#endif /* #ifndef __PCFFONT_H */

