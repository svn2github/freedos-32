/* Keyborad Driver for FD32
 * original by Luca Abeni
 * extended by Hanzac Chen
 *
 * 2004 - 2005
 * This is free software; see GPL.txt
 */

#include <dr-env.h>

#include "key.h"
#include "queues.h"

/* Level 1 keybuf queue */
static volatile BYTE rawqueue[RAWQUEUE_MAX_SIZE];
/* static volatile int rawqueue_elements; */
static volatile WORD rawqueue_head;
static volatile WORD rawqueue_tail;
/* Level 2 keybuf queue */
static volatile BYTE *keyqueue = (BYTE *)BDA_KEYB_BUF; /* KEYQUEUE_MAX_SIZE */
/* static volatile int keyqueue_elements; */
static volatile WORD *keyqueue_head = (WORD *)BDA_KEYB_BUFHEAD;
static volatile WORD *keyqueue_tail = (WORD *)BDA_KEYB_BUFTAIL;

void rawqueue_put(BYTE code)
{
  if (rawqueue_tail-rawqueue_head == RAWQUEUE_MAX_SIZE) {
    return;
  } /* Note: keyqueue_elements++ is not used, elements determined by keyqueue_tail[0]-keyqueue_head[0] */

  fd32_cli();
  rawqueue_tail = (rawqueue_tail + 1) % RAWQUEUE_MAX_SIZE;
  rawqueue[rawqueue_tail] = code;
  fd32_sti();

  return;
}

BYTE rawqueue_get(void)
{
  if (rawqueue_tail == rawqueue_head) {
    //    fd32_error("??? No char in queue???\n");
    return 0;
  }
  /* rawqueue_elements--; */
  fd32_cli();
  rawqueue_head = (rawqueue_head + 1) % RAWQUEUE_MAX_SIZE;
  fd32_sti();

  return rawqueue[rawqueue_head];
}

/* Note: KEYBOARD BUFFER HEAD and TAIL pointer to the buffer offset relative to 0x0040:0000
 *       So we need to subtract 0x1E (which is BDA_OFFSET(BDA_KEYB_BUF)) to get the index
 */
void keyqueue_put(WORD code)
{
  if (keyqueue_tail[0]-keyqueue_head[0] == KEYQUEUE_MAX_SIZE) {
    return;
  }
  /* Note: keyqueue_elements++ is not used, elements determined by keyqueue_tail[0]-keyqueue_head[0] */
  fd32_cli();
  ((WORD *)(keyqueue + keyqueue_tail[0] - BDA_OFFSET(BDA_KEYB_BUF)))[0] = code;
  keyqueue_tail[0] = (keyqueue_tail[0] - BDA_OFFSET(BDA_KEYB_BUF) + 2) % KEYQUEUE_MAX_SIZE;
  keyqueue_tail[0] += BDA_OFFSET(BDA_KEYB_BUF);
  fd32_sti();

  return;
}

/* Note: Not ignore the maskable interrupts 'cause no shared data manipulation */
WORD keyqueue_gethead(void)
{
  if (keyqueue_tail[0] == keyqueue_head[0]) {
    /* fd32_error("??? No char in queue???\n"); */
    return 0;
  }
  return ((WORD *)(keyqueue + (keyqueue_head[0]-BDA_OFFSET(BDA_KEYB_BUF)) % KEYQUEUE_MAX_SIZE))[0];
}

BYTE keyqueue_get(void)
{
  DWORD t = keyqueue_head[0];
  if (keyqueue_tail[0] == keyqueue_head[0]) {
    /* fd32_error("??? No char in queue???\n"); */
    return 0;
  }
  /* keyqueue_elements--; */
  fd32_cli();
  keyqueue_head[0] = (keyqueue_head[0]-BDA_OFFSET(BDA_KEYB_BUF) + 1) % KEYQUEUE_MAX_SIZE;
  keyqueue_head[0] += BDA_OFFSET(BDA_KEYB_BUF);
  fd32_sti();

  return keyqueue[t-BDA_OFFSET(BDA_KEYB_BUF)];
}

int keyqueue_empty(void)
{
  if (keyqueue_tail[0] == keyqueue_head[0]) {
    /* fd32_error("??? No char in queue???\n"); */
    return 1;
  } else {
    return 0;
  }
}

void keyb_queue_clear(void)
{
  /* Rawqueue clear */
  rawqueue_head = 0;
  rawqueue_tail = 0;
  /* Keyqueue clear */
  keyqueue_head[0] = BDA_OFFSET(BDA_KEYB_BUF);
  keyqueue_tail[0] = BDA_OFFSET(BDA_KEYB_BUF);
  /* Set the Keyqueue buffer start offsets to 0x0040:0000 */
  ((WORD *)BDA_KEYB_BUFSTART)[0] = BDA_OFFSET(BDA_KEYB_BUF);
  ((WORD *)BDA_KEYB_BUFEND)[0] = BDA_OFFSET(BDA_KEYB_BUF)+KEYQUEUE_MAX_SIZE;
}
