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
#include "pcffont.h"
#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>


#define PCFVERSION 0x70636601 /* PCF signature: 'pcf\1" little endian */


typedef enum PcfType
{
  PCF_PROPERTIES       = 1 << 0, /* not supported */
  PCF_ACCELERATORS     = 1 << 1, /* not supported */
  PCF_METRICS          = 1 << 2,
  PCF_BITMAPS          = 1 << 3,
  PCF_INK_METRICS      = 1 << 4, /* not supported */
  PCF_BDF_ENCODINGS    = 1 << 5,
  PCF_SWIDTHS          = 1 << 6, /* not supported */
  PCF_GLYPH_NAMES      = 1 << 7, /* not supported */
  PCF_BDF_ACCELERATORS = 1 << 8  /* not supported */
}
PcfType;


typedef enum PcfFormatId
{
  PCF_DEFAULT_FORMAT     = 0,
  PCF_INKBOUNDS          = 2,
  PCF_ACCEL_W_INKBOUNDS  = 1,
  PCF_COMPRESSED_METRICS = 1
}
PcfFormatId;


typedef struct PcfFormat
{
  DWORD       glyph : 2;  /* bitmap padding (see struct PcfBitmap) */
  DWORD       bit   : 1;  /* set if big-endian numbers follow      */
  DWORD       byte  : 1;  /* set if bits are stored as MSB first   */
  DWORD       scan  : 2;  /* not supported                         */
  DWORD       dummy : 2;  /* reserved, set to 0                    */
  PcfFormatId id    : 24; /* actual format type                    */
}
PcfFormat;


/* A table entry in the PCF Table Of Contents (see struct PcfToc). */
typedef struct PcfTocEntry
{
  PcfType   type;   /* Table type (see enum PcfType)     */
  PcfFormat format; /* Table format (see enum PcfFormat) */
  DWORD     size;   /* Table size in bytes               */
  DWORD     offset; /* Table offset from start of file   */
}
PcfTocEntry;


/* PCF Table Of Contents, placed at the beginning of the file. */
typedef struct PcfToc
{
  DWORD version; /* Must be PCFVERSION                */
  DWORD ntables; /* Number of following table entries */
  /* Follows PcfTocEntry tables[ntables]; */
}
PcfToc;


/* An entry of the PCF Metrics table (compressed format). */
/* See struct PcfCompMetrics.                             */
typedef struct PcfCompMetricsEntry
{
  BYTE left_side_bearing;  /* leftmost non-blank pixel    */
  BYTE right_side_bearing; /* rightmost non-blank pixel   */
  BYTE character_width;    /* character spacing in pixels */
  BYTE ascent;             /* pixels above base-line      */
  BYTE descent;            /* pixels below base-line      */
}
__attribute__ ((packed)) PcfCompMetricsEntry;


/* PCF Metrics table (compressed format).                  */
/* type = PCF_METRICS, format.id = PCF_COMPRESSED_METRICS. */
typedef struct PcfCompMetrics
{
  PcfFormat format;
  WORD      metrics_count;
  /* Follows PcfCompMetricsEntry metrics[metrics_count] */
}
__attribute__ ((packed)) PcfCompMetrics;


/* PCF Bitmaps table.                                  */
/* type = PCF_BITMAPS, format.id = PCF_DEFAULT_FORMAT. */
typedef struct PcfBitmap
{
  PcfFormat format;
  DWORD     glyph_count;
  /* Follows DWORD offsets[glyph_count]                                     */
  /*   Each entry gives the byte offset of each glyph in the bitmap_data[]. */
  /* Follows DWORD bitmap_sizes[4]                                          */
  /*   Size of the bitmap_data field if array is BYTE/WORD/DWORD/? padded.  */
  /*   format.glyph gives the padding type used.                            */
  /* Follows BYTE  bitmap_data[bitmap_sizes[format.glyph]]                  */
}
PcfBitmap;


/* PCF BDF Encoding table.                                   */
/* type = PCF_BDF_ENCODINGS, format.id = PCF_DEFAULT_FORMAT. */
typedef struct PcfEncoding
{
  PcfFormat format;
  WORD      min_char_or_byte2; /* minimum encoded code-point              */
  WORD      max_char_or_byte2; /* maximum encoded code-point              */
  WORD      min_byte1;         /* 0 for single-byte encodings             */
  WORD      max_byte1;         /* 0 for single-byte encodings             */
  WORD      default_char;      /* character to use for unknown code-point */
  /* Follows WORD glyph_indeces[max_char_or_byte2 - min_char_or_byte2 + 1] */
  /* Each entry gives the glyph index that corresponds to each encoding    */
  /* value, 0xFFFF meaning no glyph for that encoding.                     */
}
__attribute__ ((packed)) PcfEncoding;


static int read_metrics(FILE *f, PcfFont *font, const PcfTocEntry *tab)
{
  PcfCompMetrics      met;
  PcfCompMetricsEntry ent;
  unsigned            k;

  /* Only compressed (byte), little-endian  metrics are supported */
  if (tab->format.id != PCF_COMPRESSED_METRICS) return -1;
  if (tab->format.bit || tab->format.byte) return -1;
  /* Read and check the compressed metrics table header */
  fseek(f, tab->offset, SEEK_SET);
  if (fread(&met, sizeof(PcfCompMetrics), 1, f) != 1) return -1;
  font->metrics = (FontMetrics *) malloc(sizeof(FontMetrics) * met.metrics_count);
  if (!font->metrics) return -1;
  if (font->glyph_count & (font->glyph_count != met.metrics_count)) return -1;
  font->glyph_count = met.metrics_count;
  /* Read each compressed metrics entry */
  for (k = 0; k < met.metrics_count; k++)
  {
    if (fread(&ent, sizeof(PcfCompMetricsEntry), 1, f) != 1) return -1;
    font->metrics[k].left_side_bearing  = (int) ent.left_side_bearing  - 0x80;
    font->metrics[k].right_side_bearing = (int) ent.right_side_bearing - 0x80;
    font->metrics[k].character_width    = (int) ent.character_width    - 0x80;
    font->metrics[k].ascent             = (int) ent.ascent             - 0x80;
    font->metrics[k].descent            = (int) ent.descent            - 0x80;
  }
  return 0;
}


static int read_bitmaps(FILE *f, PcfFont *font, const PcfTocEntry *tab)
{
  PcfBitmap bmp;

  /* Only 32-bit padded, little-endian bitmaps are supported */
  if (tab->format.glyph != 2) return -1;
  if (tab->format.bit || tab->format.byte) return -1;
  /* Read bitmap table header */
  fseek(f, tab->offset, SEEK_SET);
  if (fread(&bmp, sizeof(PcfBitmap), 1, f) != 1) return -1;
  if (font->glyph_count & (font->glyph_count != bmp.glyph_count)) return -1;
  font->glyph_count = bmp.glyph_count;
  /* Allocate and read bitmap_offsets[] */
  font->bitmap_offsets = (DWORD *) malloc(sizeof(DWORD) * bmp.glyph_count);
  if (!font->bitmap_offsets) return -1;
  if (fread(font->bitmap_offsets, sizeof(DWORD), bmp.glyph_count, f) != bmp.glyph_count) return -1;
  /* Read bmp.bitmap_sizes[2], the size of DWORD-padded bitmap_data[] */
  fseek(f, 2 * sizeof(DWORD), SEEK_CUR);
  if (fread(&font->bitmap_size, sizeof(DWORD), 1, f) != 1) return -1;
  fseek(f, sizeof(DWORD), SEEK_CUR);
  /* Allocate and read bitmap_data[] */
  font->bitmap_data = (DWORD *) malloc(font->bitmap_size);
  if (!font->bitmap_data) return -1;
  if (fread(font->bitmap_data, font->bitmap_size, 1, f) != 1) return -1;
  return 0;
}


static int read_encoding(FILE *f, PcfFont *font, const PcfTocEntry *tab)
{
  PcfEncoding enc;
  unsigned    numencs;

  /* Only little endian encodings are supported */
  if (tab->format.bit || tab->format.byte) return -1;
  fseek(f, tab->offset, SEEK_SET);
  if (fread(&enc, sizeof(PcfEncoding), 1, f) != 1) return -1;
  if (enc.min_byte1 || enc.max_byte1) return -1; /* double-byte encodings not supported */
  font->default_char = enc.default_char;
  font->min_encoding = enc.min_char_or_byte2;
  font->max_encoding = enc.max_char_or_byte2;
  numencs = enc.max_char_or_byte2 - enc.min_char_or_byte2 + 1;
  /* Allocate and read glyph_indeces[] */
  font->glyph_indeces = (WORD *) malloc(sizeof(WORD) * numencs);
  if (!font->glyph_indeces) return -1;
  if (fread(font->glyph_indeces, sizeof(WORD), numencs, f) != numencs) return -1;
  return 0;
}


#define ABORTLOAD(f, font, res) { fclose(f); pcffont_unload(font); return res; }


int pcffont_load(const char *filename, PcfFont *font)
{
  PcfToc   toc;
  unsigned k;
  long     savepos;
  FILE    *f;

  memset(font, 0, sizeof(PcfFont));
  if ((f = fopen(filename, "rb")) == NULL) return -1;
  if (fread(&toc, sizeof(PcfToc), 1, f) != 1) ABORTLOAD(f, font, -1);
  if (toc.version != PCFVERSION) ABORTLOAD(f, font, -1);
  for (k = 0; k < toc.ntables; k++)
  {
    PcfTocEntry tab;
    if (fread(&tab, sizeof(PcfTocEntry), 1, f) != 1) ABORTLOAD(f, font, -1);
    savepos = ftell(f);
    switch (tab.type)
    {
      case PCF_METRICS      : if (read_metrics(f, font, &tab) < 0)
                                ABORTLOAD(f, font, -1);
                              break;
      case PCF_BITMAPS      : if (read_bitmaps(f, font, &tab) < 0)
                                ABORTLOAD(f, font, -1);
                              break;
      case PCF_BDF_ENCODINGS: if (read_encoding(f, font, &tab) < 0)
                                ABORTLOAD(f, font, -1);
                              break;
      default               : break;
    }
    fseek(f, savepos, SEEK_SET);
  }
  fclose(f);
  return 0;
}


void pcffont_unload(PcfFont *font)
{
  free(font->glyph_indeces);
  free(font->metrics);
  free(font->bitmap_offsets);
  free(font->bitmap_data);
}
