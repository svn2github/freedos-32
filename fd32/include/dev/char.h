#ifndef __FD32_DEV_CHAR_H
#define __FD32_DEV_CHAR_H

#include <ll/i386/hw-data.h>
#include "devices.h"

#define FD32_DEV_CHAR 3

typedef struct fd32_dev_char
{
  fd32_dev_ops_t *Next;
  DWORD           Type;
  void           *P;

  int (*open)      (void *P);
  int (*close)     (void *P);
  int (*seek)     (void *p, int pos, int wence);
  int (*read)      (void *P, DWORD Size, BYTE *Buffer);
  int (*write)     (void *P, DWORD Size, BYTE *Buffer);
  int (*ioctl)     (void *P, DWORD Function, void *Params);
  /* Other functions have to come... */
} 
fd32_dev_char_t;

#endif /* #ifndef __FD32_DEV_CHAR_H */

