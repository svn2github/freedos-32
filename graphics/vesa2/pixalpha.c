/**************************************************************************
 * FreeDOS32 VESA2 Graphics Library                                       *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2003, Salvatore Isaja, all rights reserved.              *
 *                                                                        *
 * This is "pixalpha.c" - Drawing a pixel with transparency.              *
 *                        In order to take advantage of function inlining *
 *                        this file is included by files which draws with *
 *                        transparency.                                   *
 *                        MMX instructions can be used if available:      *
 *                        just define the __MMX__ symbol.                 *
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
#ifndef __PIXALPHA_C
#define __PIXALPHA_C


/* The actual position of red, green and blue bits is not important. */
/* Here we split a 32-bit color in byte components, mediate them     */
/* according to the alpha value and repack it.                       */
/* The only constraint is that components shall be on bits 23-0.     */
/* This is valid for both MMX and non-MMX implementations.           */


#ifndef __MMX__
  #define GETR(c) ((c >> 16) & 0xFF)
  #define GETG(c) ((c >> 8) & 0xFF)
  #define GETB(c) (c & 0xFF)
#endif


static inline void vesa2_pixel_alpha(DWORD *dest, DWORD source, unsigned alpha)
#ifndef __MMX__
{
  /* Calculate *dest += (alpha * (source - *dest)) >> 8.            */
  /* This is the weighted average of previous color and new color:  */
  /* *dest = *dest * (256 - alpha) / 256 + source * alpha / 256.    */
  /* It should be 255, but with 256 it's faster, with little error. */
  DWORD    c = *dest;
  unsigned r = GETR(c);
  unsigned g = GETG(c);
  unsigned b = GETB(c);
  r += (alpha * (GETR(source) - r)) >> 8;
  g += (alpha * (GETG(source) - g)) >> 8;
  b += (alpha * (GETB(source) - b)) >> 8;
  c = (r << 16) | (g << 8) | b;
  *dest = c;
}
#else
{
  /* Prepare alpha for vector multiply */
  alpha = alpha | (alpha << 8) | (alpha << 16);
  /* Since we'll need byte RGB components, we'll mask other bits out */
  const volatile QWORD mask = 0x000000FF00FF00FF;
  /* Let's calculate *dest += (alpha * (source - *dest)) >> 8 in parallel. */
  /* MMX allows vector multiplies on 4-WORD data, so we need to expand     */
  /* 32-bit colors to 64-bit, where each component takes 16 bits.          */
  /* Instructions order is rearranged to maximize pipeline efficiency.     */
  asm(
  "movd         %1, %%mm0\n" /* %mm0 = source                                  */
  "pxor      %%mm2, %%mm2\n" /* %mm2 = 0                                       */
  "movd         %0, %%mm1\n" /* %mm1 = *dest                                   */
  "punpcklbw %%mm2, %%mm0\n" /* convert source bytes to words                  */
  "movd         %2, %%mm3\n" /* %mm3 = alpha                                   */
  "punpcklbw %%mm2, %%mm1\n" /* convert dest bytes to words                    */
  "psubsw    %%mm1, %%mm0\n" /* %mm0 = source - *dest                          */
  "punpcklbw %%mm2, %%mm3\n" /* convert alpha bytes to words                   */
  "pmullw    %%mm3, %%mm0\n" /* %mm0 = alpha * (source - *dest)                */
  "movq         %3, %%mm4\n" /* %mm4 = mask                                    */
  "psraw        $8, %%mm0\n" /* %mm0 = (alpha * (source - *dest)) >> 8         */
  "paddsw    %%mm0, %%mm1\n" /* %mm1 = *dest + (alpha * (source - *dest)) >> 8 */
  "pand      %%mm4, %%mm1\n" /* mask most out significant byte of each word    */
  "packuswb  %%mm2, %%mm1\n" /* convert back source words to bytes             */
  "movd      %%mm1, %0   \n" /* store *dest                                    */
  : "=m" (*dest)
  : "m" (source), "m" (alpha), "m" (mask)
  : "%mm0", "%mm1", "%mm2", "%mm3", "%mm4");
}
#endif


#endif /* #ifndef __PIXALPHA_C */
