/* FD32 Console Handler
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/cons.h>
#include <dr-env.h>

#include "devices.h"
#include "errors.h"

#define CONS_BUFF_SIZE 129   /* DOS allows max 127 chrs per console input */
#define CONS_DEV_NAME  "con"
#define KEYB_DEV_NAME  "kbd"

static char cons_buff[CONS_BUFF_SIZE];
static int cons_count = 0;
static int cons_echo = TRUE;    /* Console ECHO control */

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
    cputs(string);
    count -= n;
    buf += n;
  }
  return size;
}

static int read(BYTE *buf, DWORD n)
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
    if (!cons_echo) {       /* Without echo, return immediately */
      done = 1;
      cons_buff[cons_count++] = c;
    } else if (c == '\b') { /* Err... Here, the REAL console stuff should be done... */
      if (cons_count > 0) {
        int x, y;
        getcursorxy(&x, &y);
        if (x > 0) {
          cputc('\b');
        }
        cons_count--;
      }
    } else if (c == '\r') {
      done = 1;
      /* When doing a file read from console must return a \r\n newline */
      cons_buff[cons_count++] = '\r';
      cons_buff[cons_count++] = '\n';
      cputc('\n');
    } else if (cons_count < CONS_BUFF_SIZE - 2) {
      cons_buff[cons_count++] = c;
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

static int getattr(void *attribute)
{
  *((DWORD *)attribute) = cons_echo;
  return 0;
}

static int setattr(void *attribute)
{
  cons_echo = (*((DWORD *)attribute) != 0);
  return 0;
}

static fd32_request_t console_request;
static int console_request(DWORD function, void *params)
{
  int res = FD32_EINVAL;
  /* This style of converting and setting variables will be well-optimized */
  fd32_read_t *r = (fd32_read_t *) params;
  fd32_write_t *w = (fd32_write_t *) params;
  fd32_close_t *c = (fd32_close_t *) params;
  fd32_setattr_t *sa = (fd32_setattr_t *) params;
  fd32_getattr_t *ga = (fd32_getattr_t *) params;

  switch (function)
  {
    case FD32_WRITE:
      if (w->Size < sizeof(fd32_write_t))
        res = FD32_EFORMAT;
      else
        res = write(w->Buffer, w->BufferBytes);
      break;
    case FD32_READ:
      if (r->Size < sizeof(fd32_read_t))
        res = FD32_EFORMAT;
      else
        res = read(r->Buffer, r->BufferBytes);
      break;
    case FD32_GETATTR:
      /* NOTE: Control the console's ECHO with FD32_SETATTR? */
      if (ga->Size < sizeof(fd32_getattr_t))
        res = FD32_EFORMAT;
      else
        res = getattr(ga->Attr);
      break;
    case FD32_SETATTR:
      /* NOTE: Control the console's ECHO with FD32_SETATTR? */
      if (sa->Size < sizeof(fd32_setattr_t))
        res = FD32_EFORMAT;
      else
        res = setattr(sa->Attr);
      break;
    case FD32_GET_DEV_INFO:
      res = 0x83;
      break;
    case FD32_CLOSE:
      if (c->Size < sizeof(fd32_close_t))
        res = FD32_EFORMAT;
      else
        res = 0;
      break;
    default:
      break;
  }
  return res;
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
