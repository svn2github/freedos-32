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
static volatile BYTE *rawqueue = (BYTE *)BDA_KEYB_BUF; /* RAWQUEUE_MAX_SIZE */
/* static volatile int rawqueue_elements; */
static volatile WORD *rawqueue_head = (WORD *)BDA_KEYB_BUFHEAD;
static volatile WORD *rawqueue_tail = (WORD *)BDA_KEYB_BUFTAIL;
/* Level 2 keybuf queue */
static volatile BYTE keyqueue[KEYQUEUE_MAX_SIZE];
/* static volatile int keyqueue_elements; */
static volatile WORD keyqueue_head;
static volatile WORD keyqueue_tail;

/* Note: KEYBOARD BUFFER HEAD and TAIL pointer to the buffer offset relative to 0x0040:0000
 *       So we need to subtract 0x1E (which is BDA_OFFSET(BDA_KEYB_BUF)) to get the index
 */
void rawqueue_put(BYTE code)
{
  if (rawqueue_tail[0]-rawqueue_head[0] == RAWQUEUE_MAX_SIZE) {
    return;
  } /* Note: keyqueue_elements++ is not used, elements determined by keyqueue_tail[0]-keyqueue_head[0] */

  fd32_cli();
  rawqueue_tail[0] = (rawqueue_tail[0]-BDA_OFFSET(BDA_KEYB_BUF) + 1) % RAWQUEUE_MAX_SIZE;
  rawqueue_tail[0] += BDA_OFFSET(BDA_KEYB_BUF);
  rawqueue[rawqueue_tail[0]-BDA_OFFSET(BDA_KEYB_BUF)] = code;
  fd32_sti();

  return;
}

BYTE rawqueue_get(void)
{
  if (rawqueue_tail[0] == rawqueue_head[0]) {
    //    fd32_error("??? No char in queue???\n");
    return 0;
  }
  /* rawqueue_elements--; */
  fd32_cli();
  rawqueue_head[0] = (rawqueue_head[0]-BDA_OFFSET(BDA_KEYB_BUF) + 1) % RAWQUEUE_MAX_SIZE;
  rawqueue_head[0] += BDA_OFFSET(BDA_KEYB_BUF);
  fd32_sti();

  return rawqueue[rawqueue_head[0]-BDA_OFFSET(BDA_KEYB_BUF)];
}

void keyqueue_put(BYTE code)
{
  if (keyqueue_tail-keyqueue_head == KEYQUEUE_MAX_SIZE) {
    return;
  }
  /* Note: keyqueue_elements++ is not used, elements determined by keyqueue_tail[0]-keyqueue_head[0] */
  fd32_cli();
  keyqueue_tail = (keyqueue_tail + 1) % KEYQUEUE_MAX_SIZE;
  keyqueue[keyqueue_tail] = code;
  fd32_sti();

  return;
}

/* Note: Not ignore the maskable interrupts 'cause no shared data manipulation */
BYTE keyqueue_gethead(void)
{
  if (keyqueue_tail == keyqueue_head) {
    /* fd32_error("??? No char in queue???\n"); */
    return 0;
  }
  return keyqueue[(keyqueue_head + 1) % KEYQUEUE_MAX_SIZE];
}

BYTE keyqueue_get(void)
{
  if (keyqueue_tail == keyqueue_head) {
    /* fd32_error("??? No char in queue???\n"); */
    return 0;
  }
  /* keyqueue_elements--; */
  fd32_cli();
  keyqueue_head = (keyqueue_head + 1) % KEYQUEUE_MAX_SIZE;
  fd32_sti();

  return keyqueue[keyqueue_head];
}

void keyb_queue_clear(void)
{
  /* Keyqueue clear */
  keyqueue_head = 0;
  keyqueue_tail = 0;
  /* Rawqueue clear */
  rawqueue_head[0] = BDA_OFFSET(BDA_KEYB_BUF);
  rawqueue_tail[0] = BDA_OFFSET(BDA_KEYB_BUF);
  /* Set the Keyqueue buffer start offsets to 0x0040:0000 */
  ((WORD *)BDA_KEYB_BUFSTART)[0] = BDA_OFFSET(BDA_KEYB_BUF);
  ((WORD *)BDA_KEYB_BUFEND)[0] = BDA_OFFSET(BDA_KEYB_BUF)+KEYQUEUE_MAX_SIZE;
}