#ifndef __FD32_FSPRIV_H
#define __FD32_FSPRIV_H

#include <filesys.h>

/* FD32 format of the Job File Table                                  */
/* A JFT entry is free (closed handle) if the request member is NULL. */
typedef struct jft
{
  fd32_request_t *request;     /* File system driver request function        */
  void           *DeviceId;    /* Identifier for the open file               */
  DWORD           SearchFlags; /* Only for search handles: search flags      */
  char           *SearchName;  /* Only for search handles: path name to find */
}
tJft;
/* Defined by the FD32 kernel (PSP) */
void *fd32_get_jft(int *JftSize);

/* TODO: Don't know how to place these */
int fake_console_write(void *Buffer, int Size);
//void *fd32_init_jft(int JftSize);

#endif /* #ifndef __FD32_FSPRIV_H */

