/**************************************************************************
 * FreeDOS32 File System Layer                                            *
 * Wrappers for file system driver functions and JFT support              *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2002-2003, Salvatore Isaja                               *
 *                                                                        *
 * This is "jft.c" - To set up the Job File Table, which links integer    *
 *                   file handles in a process to file identifier of      *
 *                   file system drivers.                                 *
 *                                                                        *
 *                                                                        *
 * This file is part of the FreeDOS32 File System Layer (the SOFTWARE).   *
 *                                                                        *
 * The SOFTWARE is free software; you can redistribute it and/or modify   *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation; either version 2 of the License, or (at  *
 * your option) any later version.                                        *
 *                                                                        *
 * The SOFTWARE is distributed in the hope that it will be useful, but    *
 * WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with the SOFTWARE; see the file GPL.txt;                         *
 * if not, write to the Free Software Foundation, Inc.,                   *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA                *
 **************************************************************************/
#include <ll/i386/hw-data.h>
#include <ll/i386/stdlib.h>
#include <ll/i386/mem.h>
#include <ll/i386/error.h>

#include <kmem.h>
#include <kernel.h>
#include <errors.h>
#include <devices.h>
#include "fspriv.h"


/* Define the __DEBUG__ symbol in order to activate log output */
//#define __DEBUG__
#ifdef __DEBUG__
 #define LOG_PRINTF(s) fd32_log_printf s
#else
 #define LOG_PRINTF(s)
#endif


/* We want to do some output even if the   */
/* console driver does not implement it... */
int fake_console_write(void *Buffer, int Size)
{
  char String[255];
  int  n, Count;

  Count = Size;
  while (Count > 0)
  {
    n = 254;
    if (n > Count) n = Count;
    memcpy(String, Buffer, n);
    String[n] = 0;
    cputs(String);
    Count -= n;
    Buffer += n;
  }
  return Size;
}

/* The fake console request function is used for a fake console device */
/* open with handles 0, 1, 2 when no console driver is installed.      */
static fd32_request_t fake_console_request;
static int fake_console_request(DWORD Function, void *Params)
{
  switch (Function)
  {
    case FD32_WRITE:
    {
      fd32_write_t *X = (fd32_write_t *) Params;
      if (X->Size < sizeof(fd32_write_t)) return FD32_EFORMAT;
      return fake_console_write(X->Buffer, X->BufferBytes);
    }
    case FD32_CLOSE:
    {
      fd32_close_t *X = (fd32_close_t *) Params;
      if (X->Size < sizeof(fd32_close_t)) return FD32_EFORMAT;
      return 0;
    }
  }
  return FD32_EINVAL;
}

/* The dummy request function is used for fake AUX and PRN devices */
/* that are to be open with handles 3, 4.                          */
static fd32_request_t dummy_request;
static int dummy_request(DWORD Function, void *Params)
{
  switch (Function)
  {
    case FD32_CLOSE:
    {
      fd32_close_t *X = (fd32_close_t *) Params;
      if (X->Size < sizeof(fd32_close_t)) return FD32_EFORMAT;
      return 0;
    }
  }
  return FD32_EINVAL;
}


/*
static fd32_request_t con_filter_request;
static int con_filter_request(DWORD Function, void *Param)
{
  if (Function != FD32_READ) return
  fd32_dev_file_t *console_ops = (fd32_dev_file_t *) P;
  return console_ops->read(NULL, Buffer, 1);
}
int fake_console_read(void *P, void *Buffer, int Size)
{
}
*/


/* Allocates and initializes a new Job File Table.                  */
/* Handles 0, 1, 2, 3 and 4 are reserved for stdin, stdout, stderr, */
/* stdaux and stdprn special files respectively.                    */
/* Thus, JftSize must be at least 5.                                */
/* Returns a pointer to the new JFT on success, or NULL on failure. */
void *fd32_init_jft(int JftSize)
{
  int            hDev;
  tJft          *Jft;

  LOG_PRINTF(("Initializing JFT...\n"));
  if (JftSize < 5) return NULL;
  if ((Jft = (struct jft *)mem_get(sizeof(tJft) * JftSize)) == NULL) return NULL;
  memset(Jft, 0, sizeof(tJft) * JftSize);

  /* Prepare JFT entries 0, 1 and 2 with the "con" device */
  if ((hDev = fd32_dev_search("con")) < 0)
  {
    LOG_PRINTF(("\"con\" device not present. Using fake cosole output only.\n"));
    Jft[0].request = fake_console_request;
  }
  else fd32_dev_get(hDev, &Jft[0].request, &Jft[0].DeviceId, NULL, 0);
  /* TODO: Re-enable the blocking con read filter */
  /*
  else if ((Ops = fd32_dev_query(Dev, FD32_DEV_FILE)))
  {
    if (!(Ops->ioctl(0, FD32_SET_NONBLOCKING_IO, 0) > 0))
    {
      fd32_message("\"con\" device performs blocking input. Installing an input filter.\n");
      FakeConOps      = *Ops;
      FakeConOps.P    = Ops;
      FakeConOps.read = fake_console_read;
      Ops             = &FakeConOps;
    }
  }
  else
  {
    fd32_message("\"con\" is not a character device. Using fake cosole output only.\n");
    request = fake_console_request;
  }
  */
  Jft[1] = Jft[2] = Jft[0];

  /* Prepare JFT entry 3 with the "aux" device */
  if ((hDev = fd32_dev_search("aux")) < 0)
  {
    LOG_PRINTF(("\"aux\" device not present. Initializing handle 3 with dummy ops.\n"));
    Jft[3].request = dummy_request;
  }
  else fd32_dev_get(hDev, &Jft[3].request, &Jft[3].DeviceId, NULL, 0);

  /* Prepare JFT entry 4 with the "prn" device */
  if ((hDev = fd32_dev_search("prn")) < 0)
  {
    LOG_PRINTF(("\"prn\" device not present. Initializing handle 4 with dummy ops.\n"));
    Jft[4].request = dummy_request;
  }
  else fd32_dev_get(hDev, &Jft[4].request, &Jft[4].DeviceId, NULL, 0);

  LOG_PRINTF(("JFT initialized.\n"));
  return Jft;
}

void fd32_free_jft(void *p, int jft_size)
{
  int res;

  res = mem_free((DWORD)p, sizeof(struct jft) * jft_size);
  if (res != 1) {
    error("Restore PSP panicing while freeing the JFT\n");
    fd32_abort();
  }
}
