#ifndef __FD32_NLS_H
#define __FD32_NLS_H

#include <unicode.h>

int oemcp_to_utf8 (char *Source, UTF8 *Dest);
int utf8_to_oemcp (UTF8 *Source, int SourceSize, char *Dest, int DestSize);
int oemcp_skipchar(char *Dest);

#endif /* #ifndef __FD32_NLS_H */

