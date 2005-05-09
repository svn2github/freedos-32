#ifndef __FD32_NLS_H
#define __FD32_NLS_H

#include <unicode/unicode.h>

int oemcp_to_utf8 (char *Source, char *Dest);
int utf8_to_oemcp (char *Source, int SourceSize, char *Dest, int DestSize);
int oemcp_skipchar(char *Dest);

#endif /* #ifndef __FD32_NLS_H */
