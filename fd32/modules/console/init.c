/* FD32 Console Handler
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/cons.h>
#include <dr-env.h>

#include "devices.h"
#include "errors.h"

#define CONS_BUFF_SIZE 129  //512 DOS allows max 127 character per console input
#define CONS_DEV_NAME  "con"
#define KEYB_DEV_NAME  "kbd"

char cons_buff[CONS_BUFF_SIZE];
static int cons_count = 0;

static fd32_request_t *keyb_request;
static void *keyb_devid;

/* This is the same as fake_console_write in fd32/filesys/jft.c */
static int write(BYTE *buf, DWORD size)
{
  char string[255];
  int  n, count;

  count = size;
  while (count > 0)
  {
    n = 254;
    if (n > count) n = count;
    memcpy(string, buf, n);
    string[n] = 0;
    fd32_message(string);
    count -= n;
    buf += n;
  }
  return size;
}

static int read(void *id, DWORD n, BYTE *buf)
{
  int done;
  char c;
  fd32_read_t r;

  if (cons_count >= n) {
    /* Data is already present in the console buffer... */
    memcpy(buf, cons_buff, n);
    memcpy(cons_buff, &cons_buff[n], cons_count - n);
    cons_count -= n;
    return n;
  }

  done = 0;
  r.Size = sizeof(fd32_read_t);
  r.DeviceId = keyb_devid;
  r.Buffer = &c;
  r.BufferBytes = 1;

  while (!done) {
    keyb_request(FD32_READ, &r);
    /* Err... Here, the REAL console stuff should be done... */
    if (c == '\b') {
      if (cons_count > 0) {
        int x, y;
        getcursorxy(&x, &y);
        if (x > 0) {
          cputc('\b');
          place(--x, y);
        }
        cons_count--;
      }
      continue;
    }
    if (c == '\n') {
      done = 1;
    }
    if (cons_count < CONS_BUFF_SIZE - 2) {
      if (c == '\n') {
	cons_buff[cons_count++] = 13;
      } else {
	cons_buff[cons_count++] = c;
      }
      cputc(c);
    }
  }
  if (n > cons_count) {
    n = cons_count;
  }
  memcpy(buf, cons_buff, n);

  if (n < cons_count) {
    memcpy(cons_buff, &cons_buff[n], cons_count - n);
  }
  cons_count -= n;
  
  return n;
}

static fd32_request_t console_request;
static int console_request(DWORD function, void *params)
{
  switch (function)
  {
    case FD32_WRITE:
    {
      fd32_write_t *w = (fd32_write_t *) params;
      if (w->Size < sizeof(fd32_write_t)) return FD32_EFORMAT;
      return write(w->Buffer, w->BufferBytes);
    }
    case FD32_READ:
    {
      fd32_read_t *r = (fd32_read_t *) params;
      if (r->Size < sizeof(fd32_read_t)) return FD32_EFORMAT;
      return read(r->DeviceId, r->BufferBytes, r->Buffer);
    }
    case FD32_CLOSE:
    {
      fd32_close_t *c = (fd32_close_t *) params;
      if (c->Size < sizeof(fd32_close_t)) return FD32_EFORMAT;
      return 0;
    }
  }
  return FD32_EINVAL;
}

void console_init(void)
{
  int hdev;

  fd32_message("Searching for keyboard device...\n");
  hdev = fd32_dev_search(KEYB_DEV_NAME);
  if (hdev < 0) {
    fd32_message("Keyboard device not found!!!\n");
    return;
  }
  if (fd32_dev_get(hdev, &keyb_request, &keyb_devid, NULL, 0) < 0) {
    fd32_message("Failed to get keyboard device data!!!\n");
    return;
  }
  fd32_message("Registering console device...\n");
  if (fd32_dev_register(console_request, NULL, CONS_DEV_NAME) < 0) {
    fd32_message("Failed to register!!!\n");
  }
}
