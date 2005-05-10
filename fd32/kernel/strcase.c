/* FD/32 Simple case insensitive string compare
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */
#include <ll/ctype.h>

int strcasecmp(const char *s1, const char *s2)
{
  while (toupper(*s1) == toupper(*s2)) {
    if (*s1 == 0) {
      return 0;
    }
    s1++;
    s2++;
  }

  return *(unsigned const char *)s1 - *(unsigned const char *)s2;
}

int strncasecmp(const char *s1, const char *s2, int n)
{
  if (n == 0) return 0;
  do {
    if (toupper(*s1) != toupper(*s2++)) {
      return *(unsigned const char *)s1 - *(unsigned const char *)(s2 - 1);
    }
    if (*s1++ == 0) break;
    
  } while (--n != 0);

  return 0;
}
