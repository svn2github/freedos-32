/**************************************************************************
 * FreeDOS 32 Wildcards support library                                   *
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

#include <dr-env.h>

#include <unicode.h>
#include <errors.h>

/* This enum is used for return values of internal (static) compare functions */
enum
{
  MATCH,      /* Characters/strings match                                    */
  DONT_MATCH, /* Characters/strings don't match                              */
  MATCH_END   /* Characters match and the end of the string has been reached */
};


/* Compares the first character of the passed strings.                  */
/* The s2 string may contain '?' wildcards.                             */
/* If the characters match, returns MATCH and advance into the strings. */
/* If the characters match and they are NULLs, returns MATCH_END.       */
/* If the characters don't match, returns DONT_MATCH.                   */
static inline int compare_char(char **s1, char **s2, int *ImplicitDot)
{
  DWORD Scalar1, Scalar2;
  int   Res;
  /* Due to the possible implicit dot, we have to treat the case of the   */
  /* end of s1 separately: the trailing implicit dot of s2 may match with */
  /* a '?' wildcard or a '.' in s1.                                       */
  if (**s1 == 0)
  {
    if (**s2 == 0) return MATCH_END;
    if (!*ImplicitDot) return DONT_MATCH;
    if ((**s2 != '?') && (**s2 != '.')) return DONT_MATCH;
    *ImplicitDot = 0;
    ++*s2;
    return MATCH;
  }
  /* Otherwise, if s1 is not ended, compare the characters as usual */
  if (**s2 != '?')
  {
    if ((!(**s1 & 0x80)) || (!(**s2 & 0x80)))
    {
      if (toupper(**s1) != toupper(**s2)) return DONT_MATCH;
      ++*s1;
      ++*s2;
      return MATCH;
    }
    if ((Res = fd32_utf8to32(*s1, &Scalar1)) < 0) return FD32_EUTF8;
    *s1 += Res;
    if ((Res = fd32_utf8to32(*s2, &Scalar2)) < 0) return FD32_EUTF8;
    *s2 += Res;
    if (unicode_toupper(Scalar1) != unicode_toupper(Scalar2)) return DONT_MATCH;
  }
  /* If we have a '?' we skip a character both in s1 and s2 */
  if ((Res = fd32_utf8to32(*s1, &Scalar1)) < 0) return FD32_EUTF8;
  *s1 += Res;
  ++*s2;
  return MATCH;
}


/* Compares the first n characters of the passed strings, including the    */
/* strings NULL terminators. The s2 string may contain '?' wildcards.      */
/* If the characters match, returns MATCH.                                 */
/* If the characters match till the end of the strings, returns MATCH_END. */
/* If the characters don't match, returns DONT_MATCH.                      */
static inline int compare_n_chars(char *s1, char *s2, int n, int *ImplicitDot)
{
  int Res;
  for (;n > 0; n--)
    if ((Res = compare_char(&s1, &s2, ImplicitDot)) != MATCH) return Res;
  return 0;
}


/* Compares two UTF-8 strings, disregarding case and considering a     */
/* trailing implicit dot as required for DOS/Win file names.           */
/* The s2 string may contain any combination of '*' and '?' wildcards. */
/* Returns zero if the strings match, a positive value if they don't   */
/* match, or FD32_EUTF8 if an invalid UTF-8 sequence is found.         */
int utf8_fnameicmp(char *s1, char *s2)
{
  int   Res, ImplicitDot = 1;
  char *s;

  for (s = s1; *s; s++) if (*s == '.') ImplicitDot = 0;
  for (;;)
  {
    if (*s2 == '*')
    {
      /* If an '*' is found in s2, we search an s2 pattern into s1.       */
      /* The s2 pattern is the part of s2 enclosed by the '*' character   */
      /* and the next '*', like in "*pattern*", or between the '*' and    */
      /* the end of s2, like in "*pattern".                               */
      /* k is the s2 pattern length, including the possible 0 terminator. */
      int k = 0;
      /* If this '*' is the last character of s2, the comparison ends */
      if (*(s2 + 1) == 0) return 0;
      /* Determine the s2 pattern */
      for (s2++; s2[k] != '*'; k++) if (s2[k] == 0) { k++; break; }
      /* Search for the s2 pattern in s1, like in strstr */
      for (;;)
      {
        Res = compare_n_chars(s1, s2, k, &ImplicitDot);
        if (Res < 0) return FD32_EUTF8;
        if (Res == MATCH) break;
        if (Res == MATCH_END) return 0;
        if (*s1 == 0) return 1;
        s1++;
      }
      /* If the pattern is found, skip the pattern length in both strings */
      s1 += k;
      s2 += k;
    }
    else
    {
      /* If '*' is not found, a simple comparison char by char is peformed */
      Res = compare_char(&s1, &s2, &ImplicitDot);
      if (Res < 0) return FD32_EUTF8;
      if (Res == DONT_MATCH) return 1;
      if (Res == MATCH_END) return 0;
    }
  }
}
