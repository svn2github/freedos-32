/**************************************************************************
 * FreeDOS 32 Unicode support library                                     *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2002, Salvatore Isaja                               *
 *                                                                        *
 * This file is part of FreeDOS 32                                        *
 *                                                                        *
 * FreeDOS 32 is free software; you can redistribute it and/or modify it  *
 * under the terms of the GNU General Public License as published by the  *
 * Free Software Foundation; either version 2 of the License, or (at your *
 * option) any later version.                                             *
 *                                                                        *
 * FreeDOS 32 is distributed in the hope that it will be useful, but      *
 * WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with the FreeDOS 32 FAT Driver; see the file COPYING;            *
 * if not, write to the Free Software Foundation, Inc.,                   *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA                *
 **************************************************************************/

#ifndef __FD32_UNICODE_H
#define __FD32_UNICODE_H

/* Unicode typedefs */
typedef BYTE  UTF8;
typedef WORD  UTF16;
typedef DWORD UTF32;

/* UTF-16 management functions */
int fd32_utf16to32(const UTF16 *s, UTF32 *Ch);
int fd32_utf32to16(UTF32 Ch, UTF16 *s);

/* UTF-8 management functions */
int fd32_utf8to32(const UTF8 *s, UTF32 *Ch);
int fd32_utf32to8(UTF32 Ch, UTF8 *s);
int utf8_stricmp (const UTF8 *s1, const UTF8 *s2);
int utf8_strupr  (UTF8 *Dest, const UTF8 *Source);

/* Conversion functions between UTF-8 and UTF-16 */
int fd32_utf8to16(const UTF8 *Utf8, UTF16 *Utf16);
int fd32_utf16to8(const UTF16 *Utf16, UTF8 *Utf8);

/* Case conversion functions */
UTF32 unicode_toupper(UTF32 Ch);

#endif

